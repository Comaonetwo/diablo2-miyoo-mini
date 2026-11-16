// SPDX-License-Identifier: GPL-3.0-or-later
#include <gamedata_load.hpp>
#include <dcc.hpp>
#include <world.hpp>
#include <character_store.hpp>
#include <saved_maxima.hpp>
#include <cstdio>
#include <stdexcept>
using namespace d2d::game;using namespace d2d::d2s;
#define CHECK(x) do{if(!(x))throw std::runtime_error("combat feedback line "+std::to_string(__LINE__)+": " #x);}while(0)
int main(int argc,char**argv)try{
 if(argc<2)return 1;setenv("D2D_SERIAL_LOADING","1",1);auto data=load_game_data(argv[1],std::filesystem::path(argv[1])/"LODPatch_114d.exe",3,false);CHECK(data);auto& d=*data;int cases=0;
 {
  d2d::rules::Rng seed{9};auto made=new_character(d,kBarbarian,"ReloadCheck",false,true,seed);Character c;c.name=made.header.name;c.header=made.header;c.character_class=kBarbarian;c.stats=made.stats;c.items=made.items;
  auto hat=d2d::rules::generate_item(d.rules,"cap",10,2,seed);hat.location=item_location::kEquipped;hat.slot=body_location::kHead;hat.props={{kMaxLife,0,14},{kMaxMana,0,9},{kMaxStamina,0,6}};c.items.push_back(hat);
  const auto base=c.stats;World reload(&d,-1,-1);
  for(int attempt=0;attempt<15;++attempt){reload.enter(c);reload.tick({},1000+attempt*80+40,1000+attempt*80);const auto own=reload.fight.own_stats();
   if(own.get(kMaxLife)!=base.get(kMaxLife))fprintf(stderr,"RELOAD %d base HP %.3f -> %.3f (tracked item %.3f)\n",attempt,base.get(kMaxLife)/256.,own.get(kMaxLife)/256.,reload.fight.item_max[0]/256.);
   CHECK(own.get(kMaxLife)==base.get(kMaxLife)&&own.get(kMaxMana)==base.get(kMaxMana)&&own.get(kMaxStamina)==base.get(kMaxStamina));
   CHECK(reload.character.stats.get(kLife)==reload.character.stats.get(kMaxLife));
   const auto saved=d2d::d2s::write_save({},c.header,own,reload.character.items,*d.item_tables);c.stats=d2d::d2s::parse_stats(saved,*d.item_tables);
  }
  reload.fight.bo_life=20*256;reload.character.stats.values[kMaxLife]+=20*256;
  reload.new_game();reload.tick({},3000,2960);CHECK(reload.fight.own_stats().get(kMaxLife)==base.get(kMaxLife));
  for(int cls=0;cls<7;++cls){auto fresh=new_character(d,cls,"SwitchCheck",false,true,seed);Character other;other.header=fresh.header;other.name=fresh.header.name;other.character_class=cls;other.stats=fresh.stats;other.items=fresh.items;other.items.push_back(hat);
   reload.enter(other);reload.tick({},4000+cls*80+40,4000+cls*80);const auto own=reload.fight.own_stats();
   for(auto stat:{kMaxLife,kMaxMana,kMaxStamina})CHECK(own.get(stat)==fresh.stats.get(stat));
  }
  puts("PASS switching through all7 classes clears previous equipment/buff maxima");
  auto invalid=base;invalid.values[kMaxLife]=-256;CharacterStore guard;guard.dir="/dev/null";guard.tables=&*d.item_tables;
  CHECK(guard.save(c.header,invalid,c.items).find("truncation")!=std::string::npos);
  auto bad=base;bad.values[kMaxLife]=3*256;CHECK(repair_saved_maxima(d,c.header,bad)&&bad.get(kMaxLife)==base.get(kMaxLife));
  bad=base;bad.values[kMaxLife]=(1<<21)-11*256;bad.values[kLife]=3*256;CHECK(repair_saved_maxima(d,c.header,bad)&&bad.get(kMaxLife)==base.get(kMaxLife));
  auto custom=base;custom.values[kMaxLife]=custom.values[kLife]=1000*256;CHECK(!repair_saved_maxima(d,c.header,custom)&&custom.get(kMaxLife)==1000*256);
  auto quest=c.header;d2d::rules::quest::set(quest.quests[0],d2d::rules::GoldenBirdQuest::kQuest,0);bad=base;CHECK(repair_saved_maxima(d,quest,bad)&&bad.get(kMaxLife)==base.get(kMaxLife)+20*256);
  puts("PASS15 reload/save cycles preserve base life/mana/stamina with equipment bonuses; full equipped refill, explicit new game, old deficits/wrap, custom maxima and permanent quest bonus");
 }

 for(int cls=0;cls<7;++cls)for(const char* code:{"hp1","hp2","hp3","hp4","hp5","mp1","mp2","mp3","mp4","mp5"}){
  World w(&d,-1,-1);w.character.character_class=cls;auto& s=w.character.stats.values;
  s[kLife]=s[kMana]=256;s[kMaxLife]=s[kMaxMana]=2560000;s[kVit]=s[kEne]=0;
  w.fight.prayer_no_mana_regen=true;w.fight.player_combat.life_regen=0;
  const auto p=d.rules.potions.at(code);const bool life=p.life>0;const int amount=d2d::rules::potion_bonus((life?p.life:p.mana)*256,cls,life);const int rate=amount/p.ticks;
  w.fight.potion(code,1000);CHECK(w.fight.regen.size()==1);CHECK(w.fight.regen[0].until==1000+40*p.ticks);
  auto& value=s[life?kLife:kMana];const auto before=value;
  for(int tick=1;tick<=p.ticks;++tick){w.fight.apply_regen(1000+tick*40,1000+(tick-1)*40);CHECK(value-before==rate*tick);}
  CHECK(w.fight.regen.empty());const auto end=value;w.fight.apply_regen(1000+(p.ticks+1)*40,1000+p.ticks*40);CHECK(value==end);++cases;
  // Repeated doses add remaining amount AND time, without speeding the rate.
  w.fight.potion(code,20000);w.fight.apply_regen(21000,20000);w.fight.potion(code,21000);CHECK(w.fight.regen.size()==1);CHECK(w.fight.regen[0].until==20000+80*p.ticks);
  const int left=p.ticks-25;const int stacked=(rate*left+amount)/(left+p.ticks);CHECK(std::abs(w.fight.regen[0].life+w.fight.regen[0].mana-stacked/40.0)<.00001);
 }
 printf("PASS %d real healing/mana potion rows across7 classes: each25Hz tick, original class bonuses/duration, expiry and repeated doses\n",cases);
 Level area;area.id=37;area.ds1=d2d::ds1::Map(64,64,0,1);area.walk.resize(320*320);
 World w(&d,-1,-1);w.character.character_class=kBarbarian;w.character.stats.values[kLife]=w.character.stats.values[kMaxLife]=256000;w.arrive(&area,21,20,"combat feedback");
 d2d::rules::Rng rng{11};
 for(const char* name:{"fallen4","fallenshaman4","bighead2","vampire1","arach2"}){
  auto m=make_monster(d,d.monsters.row(name),20,20,rng,0);auto src=m.stats;src.to_hit=1<<20;
  int high=src.a1_max;for(auto e:src.elements)if(e.mode=="A1"&&e.type!=3&&e.type!=5)high+=e.max;CHECK(high<60);
  for(int n=0;n<50;++n){Foe foe;foe.fighter={};foe.level=1;foe.alive=true;w.character.stats.values[kLife]=256000;w.fight.bind_incoming(foe,0,-1,1000);
   const auto taken=d2d::rules::monster_blow(foe.fighter,1,d2d::rules::Motion::standing,src,false,rng,false,true);auto result=foe.take(taken,m.id);
   const auto loss=256000-w.character.stats.values[kLife];CHECK(loss==result.life_damage-result.life_heal);CHECK(loss<=high*256);CHECK(foe.damage==foe.settled_damage);
  }
  printf("PASS Normal %s melee damage at most%d HP per unresisted hit; actual receiver commits once\n",name,high);
 }
 // Tainted's lightning inherits its MonStats element once, not a second hit.
 auto m=make_monster(d,d.monsters.row("bighead2"),20,20,rng,0);m.mode="A2";const auto shot=monster_attack_missile(d.missiles.at("bighead2"),m,1,0,1000);
 CHECK(shot.src.elements[0].min==m.stats.elements[0].min&&shot.src.elements[0].max==m.stats.elements[0].max);
 CHECK(shot.src.a2_max==0);printf("PASS Tainted bolt Normal lightning=%d..%d HP, no extra physical damage\n",shot.src.elements[0].min,shot.src.elements[0].max);

 // Cross-platform decoder check against pre-existing release0.2.34 hashes.
 // The adaptive cache may keep extra small directions, never more pixel bytes
 // than the former two-largest-direction allowance.
 const std::pair<const char*,std::uint64_t> assets[]{
 {R"(data\global\CHARS\BA\HD\BAHDLITA1HTH.dcc)",10870143056611625626ull},
 {R"(data\global\CHARS\BA\LG\BALGLITSCHTH.dcc)",114252647298731069ull},
 {R"(data\global\CHARS\SO\HD\SOHDLITA1HTH.dcc)",9039560656028683121ull},
 {R"(data\global\CHARS\SO\LG\SOLGLITSCHTH.dcc)",3391254549349525057ull},
 {R"(data\global\CHARS\NE\HD\NEHDLITA1HTH.dcc)",4580681604952549365ull},
 {R"(data\global\CHARS\NE\LG\NELGLITSCHTH.dcc)",2706958152786383292ull},
 {R"(data\global\monsters\0A\TR\0ATRLITNUHTH.dcc)",12058899751816267154ull},
 {R"(data\global\monsters\L3\TR\L3TRLITWLHTH.dcc)",625923439424440898ull}
 };
 for(auto [path,expected]:assets){auto raw=d.mpqs.read(path);d2d::dcc::Sprite sprite{std::vector<std::byte>(raw)};sprite.cache_equivalent_directions();const auto ceiling=sprite.cache_reservation_bytes();
  std::uint64_t hash=1469598103934665603ull;const auto add=[&](std::uint8_t v){hash=(hash^v)*1099511628211ull;};
  for(unsigned dir=0;dir<sprite.directions();++dir)for(int f=0;f<sprite.frames_per_direction();++f){const auto& frame=sprite.frame(dir,f);
   for(int v:{frame.width,frame.height,frame.x_offset,frame.y_offset,frame.box_left,frame.box_top,frame.box_right,frame.box_bottom})for(int k=0;k<4;++k)add(std::uint8_t(std::uint32_t(v)>>(k*8)));
   for(auto v:frame.pixels)add(v);CHECK(sprite.storage_bytes()<=ceiling);
  }
  CHECK(hash==expected);sprite.release_pixels();for(const auto& f:sprite.frames())CHECK(f.pixels.capacity()==0);
 }
 puts("PASS8 hero/monster DCC golden hashes on all directions/frames, unchanged pixel ceiling and full release");
 return 0;
}catch(const std::exception&e){fprintf(stderr,"%s\n",e.what());return 1;}
