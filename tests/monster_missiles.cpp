// SPDX-License-Identifier: GPL-3.0-or-later
#include <gamedata_load.hpp>
#include <world.hpp>
#include <cstdio>
#include <stdexcept>
using namespace d2d::game;
#define CHECK(x) do{if(!(x))throw std::runtime_error("monster_missiles line "+std::to_string(__LINE__)+": " #x);}while(0)
static d2d::rules::MissileDamage own_damage(const GameData& d,const Missile& shot){
 const auto& r=*shot.info;const d2d::rules::Skill* s=nullptr;
 if(!r.skill.empty())if(auto i=d.skills.by_name.find(r.skill);i!=d.skills.by_name.end())s=d.skills.get(i->second);
 return s?d2d::rules::row_damage(s->etype,s->emin,s->emax,s->emin_lev,s->emax_lev,s->hitshift,s->elen,s->elen_lev,shot.level):d2d::rules::row_damage(r.etype,r.emin,r.emax,r.emin_lev,r.emax_lev,r.hitshift,r.elen,r.elen_lev,shot.level);
}
static void check_poison(const GameData& d,const Missile& shot){
 const auto damage=own_damage(d,shot);CHECK(damage.etype==3&&damage.ehi>0);CHECK(shot.src.poison_min==damage.elo&&shot.src.poison_max==damage.ehi&&shot.src.poison_len==damage.elen);
 for(int resistance:{0,75,100}){d2d::rules::Fighter defender;defender.res[3]=resistance;defender.plr=resistance?50:0;d2d::rules::Rng rng{7};int hits=0;
  for(int n=0;n<100;++n){auto hit=d2d::rules::monster_blow(defender,15,d2d::rules::Motion::standing,shot.src,true,rng,true);if(!hit.hit)continue;++hits;CHECK(hit.poison>=d2d::rules::resisted(damage.elo,resistance)&&hit.poison<=d2d::rules::resisted(damage.ehi,resistance));CHECK(hit.poison_ticks==d2d::rules::resisted(damage.elen,defender.plr));}CHECK(hits>0);
 }
}
int main(int argc,char**argv)try{
 if(argc<2)return 0;setenv("D2D_SERIAL_LOADING","1",1);auto data=load_game_data(argv[1],std::filesystem::path(argv[1])/"LODPatch_114d.exe",3,false);CHECK(data);auto& d=*data;d2d::rules::Rng rng{3};
 Level a;a.id=37;a.ds1=d2d::ds1::Map(64,64,0,1);a.walk.resize(320*320);Crowd crowd;int poison_shots=0,other_shots=0;
 for(std::size_t type=0;type<d.monsters.types.size();++type){const auto& info=d.monsters.types[type];if(d.mon_npc[type].hc_idx>=410)continue;
  for(int diff=0;diff<3;++diff)for(auto pair:{std::pair{info.miss_a1,"A1"},std::pair{info.miss_a2,"A2"},std::pair{info.miss_c,"SC"},std::pair{info.miss_s1,"S1"}}){
   const auto it=d.missiles.find(pair.first);if(it==d.missiles.end()||!it->second.skill.empty())continue;
   auto m=make_monster(d,int(type),20,20,rng,diff);m.mode=pair.second;auto shot=monster_attack_missile(it->second,m,1,0,1000,pair.second==std::string_view("S1"));
   if(it->second.etype==3){check_poison(d,shot);++poison_shots;}
   else if(it->second.etype>=0){const auto damage=own_damage(d,shot);CHECK(std::ranges::any_of(shot.src.elements,[&](const auto& e){return e.type==damage.etype&&e.mode=="A2"&&e.min>=(damage.elo>>8)&&e.max>=(damage.ehi>>8);}));++other_shots;}
  }
 }
 for(const char* name:{"andariel","radament","unraveler1"})for(int diff=0;diff<3;++diff){
  auto m=make_monster(d,d.monsters.row(name),20,20,rng,diff);auto stats=m.stats;stats.to_hit=1<<20;
  auto e=std::ranges::find_if(stats.elements,[](const auto&e){return e.type==3;});CHECK(e!=stats.elements.end());e->pct=100;e->mode="A1";
  d2d::rules::Fighter defender;bool hit=false;for(int n=0;n<64&&!hit;++n){const auto taken=d2d::rules::monster_blow(defender,15,d2d::rules::Motion::standing,stats,false,rng);if(!taken.hit)continue;hit=true;CHECK(taken.poison>=e->min*10&&taken.poison<=e->max*10&&taken.poison_ticks==e->dur*2);}CHECK(hit);
 }
 puts("PASS original MonStats melee poison x10 rate / x2 duration preserved for Andariel, Radament and mummies on3 difficulties");
 CHECK(poison_shots>20&&other_shots>10);printf("PASS classic monster attack missile rows: %d poison / %d other element cases across3 difficulties\n",poison_shots,other_shots);
 for(const auto& name:{"radament","sandmaggot1","tentaclehead1"}){const auto type=d.monsters.row(name);CHECK(type>=0);auto m=make_monster(d,type,20,20,rng,0);auto& info=d.monsters.types[std::size_t(type)];const auto& row=d.missiles.at(info.miss_a2.empty()?info.miss_a1:info.miss_a2);m.mode=info.miss_a2.empty()?"A1":"A2";
  // A no-SrcDamage shot must not acquire an unrelated owner's poison/fire.
  m.stats.poison_min=9000;m.stats.poison_max=10000;m.stats.poison_len=9999;m.stats.attack_fire_min=m.stats.attack_fire_max=999;
  auto shot=monster_attack_missile(row,m,1,0,1000);check_poison(d,shot);CHECK(shot.src.attack_fire_min==0&&shot.src.attack_fire_max==0);printf("PASS %s own poison, no inherited SrcDamage=0 attack bonus\n",name);
 }
 const auto cast=[&](const char* name,const char* skillname,const char* mode,int diff){
  auto m=make_monster(d,d.monsters.row(name),20,20,rng,diff);const auto id=d.skills.by_name.at(skillname);m.mode=mode;m.unit.mode_ms=1000;m.mode_until=10000;m.skill=id;m.skill_frame=3;m.skill_x=110;m.skill_y=100;
  Foe foe;foe.x=22;foe.y=20;foe.level=15;std::vector<Missile> shots;
  for(unsigned now=2000;shots.empty()&&now<2400;now+=40)monster_update(d,a,m,{&foe,1},rng,now,.04f,crowd,shots);
  CHECK(!shots.empty());return shots;
 };
 for(int diff=0;diff<3;++diff){
  auto shots=cast("mephisto","PrimePoisonNova","A2",diff);for(const auto& shot:shots)check_poison(d,shot);printf("PASS Mephisto poison nova difficulty=%d level=%d count=%zu\n",diff,shots[0].level,shots.size());
  auto orbs=cast("mephisto","MephistoMissile","A2",diff);CHECK(orbs[0].src.a2_min>0&&orbs[0].src.a2_max>=orbs[0].src.a2_min);if(diff==0)CHECK(orbs[0].src.a2_min==44&&orbs[0].src.a2_max==48);printf("PASS Mephisto orb difficulty=%d physical=%d..%d\n",diff,orbs[0].src.a2_min,orbs[0].src.a2_max);
  auto bolts=cast("mephisto","PrimeBolt","A2",diff);CHECK(std::ranges::any_of(bolts[0].src.elements,[](const auto&e){return e.type==1&&e.max>0;}));printf("PASS Mephisto charged bolt uses listed Charged Strike damage difficulty=%d\n",diff);
  auto beams=cast("diablo","DiabLight","SC",diff);CHECK(beams[0].src.a2_min>0&&std::ranges::any_of(beams[0].src.elements,[](const auto&e){return e.type==1&&e.max>0;}));printf("PASS Diablo lightning difficulty=%d physical=%d..%d plus lightning\n",diff,beams[0].src.a2_min,beams[0].src.a2_max);
 }
 World w(&d,-1,-1);w.character.character_class=0;w.character.stats.values[d2d::d2s::kLife]=w.character.stats.values[d2d::d2s::kMaxLife]=256000;w.arrive(&a,21,20,"poison routing test");
 Monster mummy;mummy.stats.level=30;mummy.unit.x=21;mummy.unit.y=20;w.fight.boss_missile(mummy,"corpsepoisoncloud",0,0,std::make_shared<std::vector<int>>(),1000,30);CHECK(w.fight.pending.size()==1);check_poison(d,w.fight.pending[0]);CHECK(w.fight.pending[0].row);
 puts("PASS corpse cloud keeps exact poison payload and existing continuous-cloud handler");
 // A real hostile missile reaches the same Foe poison fields used by player,
 // mercenary and summons, with no second duration/rate conversion.
 auto shots=cast("andariel","AndyPoisonBolt","A1",0);auto shot=shots[0];const auto expected=own_damage(d,shot);shot.x=21;shot.y=20;shot.velocity_x=shot.velocity_y=0;shot.born=1000;shot.dies=5000;
 for(int victim=0;victim<3;++victim){Foe foe;foe.x=21;foe.y=20;foe.level=15;foe.fighter.res[3]=victim*25;bool landed=false;for(int trial=0;trial<64&&!landed;++trial){std::vector<Missile> ms{shot};ms[0].struck=std::make_shared<std::vector<int>>();missiles_update(a,ms,{&foe,1},rng,2000,[](float,float){return std::vector<std::array<int,4>>{};},[](Missile&){return false;},[](int){return true;});landed=foe.poison>0;}CHECK(landed&&foe.poison>=d2d::rules::resisted(expected.elo,foe.fighter.res[3])&&foe.poison<=d2d::rules::resisted(expected.ehi,foe.fighter.res[3])&&foe.poison_ticks==expected.elen);}
 puts("PASS actual hostile missile collision delivers original poison rate/length with0/25/50 resistance");
 w.fight.pending.clear();w.fight.monsters.clear();w.fight.regen.clear();w.fight.missiles.clear();bool poisoned=false;unsigned start=0;
 for(int n=0;n<16&&!poisoned;++n){start=2000+n*40;auto s=shot;s.born=start;s.dies=start+2000;s.struck=std::make_shared<std::vector<int>>();w.fight.missiles={s};w.tick({},start,start-40);poisoned=std::ranges::any_of(w.fight.regen,[](const auto& r){return r.poison;});}
 CHECK(poisoned);w.fight.missiles.clear();const auto before=w.character.stats.get(d2d::d2s::kLife);
 for(unsigned n=1;n<=25;++n)w.tick({},start+n*40,start+(n-1)*40);
 const auto loss=before-w.character.stats.get(d2d::d2s::kLife);CHECK(loss>=2*256&&loss<=8*256);printf("PASS actual player poison tick: %.3f HP lost in1 second, no instant kill\n",loss/256.0);
 return 0;
}catch(const std::exception&e){fprintf(stderr,"%s\n",e.what());return 1;}
