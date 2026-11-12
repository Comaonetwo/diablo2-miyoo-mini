// SPDX-License-Identifier: GPL-3.0-or-later
#include <world.hpp>
#include <replication.hpp>
#include "../src/inventory_arrange.hpp"
#include <gamedata_load.hpp>
#include <d2s_write.hpp>
#include <item_text.hpp>
#include <txt.hpp>
#include <map>
#include <cstdio>
#include <chrono>
#include <stdexcept>
using namespace d2d::game;using namespace d2d::d2s;namespace rule=d2d::rules;
#define CHECK(x) do{if(!(x))throw std::runtime_error("item effects line "+std::to_string(__LINE__));}while(false)
static Item worn(const char* code,int slot,std::vector<ItemProp> props){Item i;i.code=code;i.slot=slot;i.location=1;i.identified=true;i.quality=2;i.props=std::move(props);return i;}
static void state_group_health_cases(GameData& data){
 World w(&data,-1,-1);auto& f=w.fight;auto& c=w.character;c.character_class=6;c.stats.values[kLevel]=60;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;
 const auto* fade=f.skill_named("Fade");const auto* speed=f.skill_named("Quickness");if(!speed)speed=f.skill_named("Burst of Speed");const auto* frozen=f.skill_named("Frozen Armor");const auto* shiver=f.skill_named("Shiver Armor");const auto* chill=f.skill_named("Chilling Armor");const auto* venom=f.skill_named("Venom");CHECK(fade&&speed&&frozen&&shiver&&chill&&venom);
 CHECK(f.buff_conflicts(*fade,speed->id)&&f.buff_conflicts(*frozen,shiver->id)&&f.buff_conflicts(*shiver,chill->id)&&!f.buff_conflicts(*fade,venom->id));
 f.start_state(*fade,10,1000);f.start_state(*venom,10,1000);f.start_state(*speed,10,1100);CHECK(f.self_states.size()==2&&f.state_of(fade->aurastate)==nullptr&&f.state_of(speed->aurastate)&&f.state_of(venom->aurastate));
 f.start_state(*frozen,10,1200);f.start_state(*shiver,10,1300);f.start_state(*chill,10,1400);CHECK(f.self_states.size()==3&&!f.state_of(frozen->aurastate)&&!f.state_of(shiver->aurastate)&&f.state_of(chill->aurastate));
 const auto* master=f.skill_named("Shadow Master");CHECK(master);f.update_fighters(2000);f.summon(*master,20,20,20,2000);CHECK(f.pets.size()==1);auto& pet=f.pets[0];f.shadow_give(pet,fade->id,10);f.shadow_give(pet,speed->id,10);pet.cast=fade->id;f.shadow_cast(pet,2100,{});pet.cast=speed->id;f.shadow_cast(pet,2200,{});CHECK(pet.buffs.size()==1&&pet.buffs[0].skill==speed->id&&f.pet_buff_stat(pet,109)==0);
 bool sampled=false;for(unsigned seed=1;seed<=64&&!sampled;++seed){Fight::Pet sample;sample.monster=make_monster(data,data.monsters.row(master->summon),20,20,f.rng,0);sample.monster.stats.level=60;sample.base_combat.emplace();sample.base_combat->life=100;sample.base_combat->life_pct=7;sample.base_combat->ar=100;f.rng=rule::Rng{seed};f.equip_shadow(sample,*master,20);const auto& sum=sample.shadow_gear->sum;
  if(sum[kMaxLife]>0){sampled=true;const int base=100+int(sum[kMaxLife]+sum[216]*60/8);CHECK(sample.base_combat->life==base&&sample.monster.stats.hit_points==base*(107+sum[kMaxLifePercent])/100&&sample.monster.hit_points==sample.monster.stats.hit_points);}
 }CHECK(sampled);
 std::puts("PASS state groups and Shadow life: actual States.txt exclusivity for three Sorceress armors and Fade/Burst on hero/Shadow, independent Venom retained; actual generated Shadow gear flat/per-level life in HP units and percent ordering");
}

static void compact_pack_cases(GameData& data){
 auto& t=data.rules;t.item_info["qa1"].width=t.item_info["qa1"].height=1;
 const auto make=[&](int n){std::vector<Item> bag;for(int k=0;k<n;++k){Item i;i.code="qa1";i.uid=unsigned(k+10);i.location=item_location::kStored;i.panel=item_panel::kInventory;bag.push_back(i);}return bag;};
 for(int n:{4,12,20,35,40}){auto bag=make(n);auto worn_item=worn("cap",1,{});worn_item.column=7;bag.push_back(worn_item);CHECK(d2mini::arrange_inventory(t,bag));std::array<bool,40> used{};int width=0,height=0;
  for(int k=0;k<n;++k){const auto& i=bag[std::size_t(k)];CHECK(i.uid==unsigned(k+10)&&i.column>=0&&i.column<10&&i.row>=0&&i.row<4&&!used[std::size_t(i.row*10+i.column)]);used[std::size_t(i.row*10+i.column)]=true;width=std::max(width,i.column+1);height=std::max(height,i.row+1);}CHECK(bag.back().column==7&&bag.back().slot==1);if(n==4)CHECK(width==2&&height==2);if(n==20)CHECK(width==5&&height==4);
 }
 auto invalid=make(41);invalid[0].column=9;CHECK(!d2mini::arrange_inventory(t,invalid)&&invalid[0].column==9);
 std::puts("PASS compact inventory: square small piles, twenty items in left5x4 leaving full-height pickup room, mixed equipment untouched, occupancy/identity and transactional full failure");
}
static void reddit_firestorm_sound_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(320*320);w.level=&arena;w.player.x=w.player.y=20;
 auto& f=w.fight;auto& c=w.character;c.character_class=5;c.stats.values[kLevel]=6;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;c.stats.values[kMana]=c.stats.values[kMaxMana]=256000;
 const auto* fire=f.skill_named("Firestorm");CHECK(fire&&fire->start_sound>0);const auto& ids=data.skills.class_ids[5];const auto learned=std::ranges::find(ids,fire->id);CHECK(learned!=ids.end());c.stats.skills[std::size_t(learned-ids.begin())]=1;const auto flame=data.missiles.find("firestorm");CHECK(flame!=data.missiles.end());
 auto mob=make_monster(data,data.monsters.row("quillrat1"),20,20,f.rng,0);mob.id=101;mob.hit_points=mob.stats.hit_points=1000;f.monsters={mob};
 for(int i=0;i<3;++i){Missile m;m.info=&flame->second;m.skill=fire->id;m.level=1;m.friendly=true;m.x=m.y=20;m.born=0;m.dies=600;f.missiles.push_back(m);}
 f.missile_tick(40);CHECK(f.monsters[0].hit_points==1000&&f.monsters[0].dot_acc>0);
 for(int i=2;i<=12;++i)f.missile_tick(unsigned(i*40));CHECK(f.monsters[0].hit_points<1000&&f.monsters[0].hit_points>=996);
 CHECK(!f.skill_missile_hits(f.missiles[0],0,500));
 f.monsters[0]=mob;f.monsters[0].boss_res[2]=100-data.monsters.types[std::size_t(mob.type)].diff[0].res[2];
 for(int i=13;i<=24;++i)f.missile_tick(unsigned(i*40));CHECK(f.monsters[0].hit_points==1000);
 f.cues.due.clear();CHECK(f.cast_missile(fire->id,22,20,1000));CHECK(std::ranges::any_of(f.cues.due,[&](const auto& cue){return cue.sound==fire->start_sound;}));
 f.pmode=-1;c.stats.values[kMana]=0;f.cues.due.clear();CHECK(!f.cast_missile(fire->id,22,20,2000));CHECK(std::ranges::none_of(f.cues.due,[&](const auto& cue){return cue.sound==fire->start_sound;}));
 for(auto name:{"Firestorm","Fire Bolt","Ice Bolt","Charged Bolt"}){const auto* skill=f.skill_named(name);CHECK(skill&&skill->start_sound>0);const auto& sound=data.sounds[std::size_t(skill->start_sound)];CHECK(!sound.file.empty()&&!sound.loop);auto wav=data.mpqs.try_read(std::string(R"(data\global\sfx\)")+sound.file);if(!wav)wav=data.mpqs.try_read(std::string(R"(data\local\sfx\)")+sound.file);CHECK(wav&&wav->size()>44);}
 const auto bolt=data.missiles.find("firebolt");CHECK(bolt!=data.missiles.end()&&bolt->second.hit_sound>0);f.monsters={mob};f.cues.due.clear();Missile shot;shot.info=&bolt->second;shot.skill=f.skill_named("Fire Bolt")->id;shot.level=1;shot.friendly=true;f.strike(shot,0,3000);CHECK(std::ranges::any_of(f.cues.due,[&](const auto& cue){return cue.sound==bolt->second.hit_sound;}));
 std::puts("PASS Reddit regression: level-one Firestorm retains fractional damage over time, overlapping flames, fire immunity; cast sound only on success; original Druid/Sorc sample files and FireBolt impact cue");
}


static void breath_and_action_sound_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(320*320);w.level=&arena;w.player.x=w.player.y=20;
 auto& f=w.fight;auto& c=w.character;c.stats.values[kLevel]=30;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;c.stats.values[kMana]=c.stats.values[kMaxMana]=256000;f.skill_level=[](int){return 1;};
 auto mob=make_monster(data,data.monsters.row("quillrat1"),20,20,f.rng,0);mob.id=101;mob.hit_points=mob.stats.hit_points=1000;
 for(auto name:{"Inferno","Arctic Blast","Inferno Sentry"}){
  const auto* skill=f.skill_named(name);CHECK(skill);const auto* row=f.skill_missile(*skill,true);if(std::string_view(name)=="Inferno Sentry")row=&data.missiles.at("inferno sentry 1");CHECK(row);
  f.monsters={mob};f.monsters[0].boss_res={};const int element=skill->etype==2?4:2;f.monsters[0].boss_res[std::size_t(element)]=-data.monsters.types[std::size_t(mob.type)].diff[0].res[std::size_t(element)];
  Missile shot;shot.info=row;shot.skill=skill->id;shot.level=1;shot.friendly=true;
  f.strike(shot,0,1000);if(skill->etype==2){CHECK(f.monsters[0].hit_points==1000);CHECK(f.monsters[0].chill_until>1000);}
  for(int i=1;i<100;++i)f.strike(shot,0,1000+unsigned(i)*40);
  const int lost=1000-f.monsters[0].hit_points;CHECK(lost>10&&lost<400&&f.monsters[0].dot_acc>=0&&f.monsters[0].dot_acc<1);
  f.monsters[0]=mob;f.monsters[0].boss_res[std::size_t(element)]=100-data.monsters.types[std::size_t(mob.type)].diff[0].res[std::size_t(element)];
  for(int i=0;i<100;++i)f.strike(shot,0,6000+unsigned(i)*40);
  CHECK(f.monsters[0].hit_points==1000&&f.monsters[0].dot_acc==0&&f.monsters[0].chill_until==0);
  if(skill->srvdofunc==ServerDoFunction::kBreath){CHECK(row->travel_sound>0);f.cues.due.clear();f.fire(*skill,11000,Fight::Caster{.x=20,.y=20,.level=1,.to_x=24,.to_y=20});CHECK(std::ranges::any_of(f.cues.due,[&](const auto& cue){return cue.sound==row->travel_sound;}));}
 }
 for(auto name:{"Fire Trauma","Shock Field","Telekinesis"}){
  const auto* skill=f.skill_named(name);CHECK(skill&&skill->action_sound>0);f.cues.due.clear();Fight::Caster cast{.x=20,.y=20,.level=1,.to_x=24,.to_y=20};
  if(f.missile_skill(*skill))f.fire(*skill,12000,cast);else f.spot(*skill,12000,cast);
  CHECK(std::ranges::any_of(f.cues.due,[&](const auto& cue){return cue.sound==skill->action_sound;}));
 }
 CHECK(f.skill_named("Find Item")->start_sound>0&&f.skill_named("Jab")->start_sound>0);
 for(auto name:{"Tiger Strike","Cobra Strike"}){const auto* skill=f.skill_named(name);CHECK(skill&&skill->progress_sound>0);f.cues.due.clear();f.charge(*skill,13000);CHECK(std::ranges::any_of(f.cues.due,[&](const auto& cue){return cue.sound==skill->progress_sound;}));}
 // Default (ordinary projectile) rounding remains unchanged; stream-only precision.
 d2d::rules::MissileDamage tiny{.etype=0,.elo=128,.ehi=128};d2d::rules::Target target;d2d::rules::Rng rng{3};double carry=0;
 CHECK(d2d::rules::missile_blow(tiny,target,{},rng).damage==0);
 CHECK(d2d::rules::missile_blow(tiny,target,{},rng,{.hit=true},&carry).damage==0&&carry==.5);
 CHECK(d2d::rules::missile_blow(tiny,target,{},rng,{.hit=true},&carry).damage==1&&carry==0);
 std::puts("PASS related skill regression: level-one Inferno/Arctic Blast/Inferno Sentry fractional damage and elemental immunity, Arctic chill below one HP, finite breath cues, Assassin bomb/web and Telekinesis action sounds, class start fallback, ordinary projectile rounding unchanged");
}

static void attack_panel_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(320*320);w.level=&arena;w.player.x=w.player.y=20;
 auto& f=w.fight;auto& c=w.character;c.stats.values[kLevel]=40;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;f.skill_level=[](int){return 10;};
 // Buckler has zero intrinsic block: class block and its Smite damage still apply.
 c.character_class=3;c.stats.values[kLevel]=1;c.stats.values[kStr]=25;c.stats.values[kDex]=20;
 Item buckler;buckler.code="buc";buckler.location=item_location::kEquipped;buckler.slot=5;buckler.quality=2;buckler.identified=true;buckler.max_durability=buckler.durability=12;c.items={buckler};
 const auto actual_shield=f.player_fighter();CHECK(data.rules.item_base.at("buc").block==0&&actual_shield.smite_lo>0&&actual_shield.smite_hi>=actual_shield.smite_lo&&actual_shield.block>0);c.items.clear();c.stats.values[kLevel]=40;
 f.player_combat=rule::simple_fighter(100,100,1000000,0);f.player_combat.phys_lo=f.player_combat.phys_hi=25600;f.player_combat.ar_base=1000000;
 f.player_combat.smite_lo=10;f.player_combat.smite_hi=20;f.player_combat.smite_pct=50;
 auto* smite=f.skill_named("Smite");CHECK(smite);const auto sm=f.panel_attack(*smite,10);const auto sw=f.skill_swing(*smite,10,f.calc_env(),true);
 CHECK(sm.damage&&sm.attack_rating==0&&sm.min==10*(150+sw.ed_pct)/100&&sm.max==20*(150+sw.ed_pct)/100);
 const auto* holy=f.skill_named("Holy Shield");const auto* orders=f.skill_named("Battle Orders");CHECK(holy&&orders);f.self_states={{holy->id,10,99999}};const auto holy_line=f.panel_attack(*smite,10);CHECK(holy_line.min>sm.min);f.self_states.push_back({orders->id,10,99999});CHECK(f.panel_attack(*smite,10).min==holy_line.min);f.self_states.clear();
 f.player_combat.elem[0]={999,999};CHECK(f.panel_attack(*smite,10).min==sm.min);f.player_combat.elem={};
 f.pf_kick=f.player_combat;f.pf_kick.kick_lo=10;f.pf_kick.kick_hi=20;f.pf_kick.kick_pct=100;
 auto* talon=f.skill_named("Dragon Talon");CHECK(talon);const auto kick=f.panel_attack(*talon,10);CHECK(kick.damage&&kick.min>0&&kick.attack_rating>0);
 f.player_combat.phys_lo=f.player_combat.phys_hi=2560000;CHECK(f.panel_attack(*talon,10).min==kick.min);f.pf_kick.kick_lo+=10;CHECK(f.panel_attack(*talon,10).min>kick.min);
 f.dual_wield=true;f.player_offhand=rule::simple_fighter(9,19,123,0);auto* attack=f.skill_named("Attack");CHECK(attack);auto both=f.panel_attack(*attack,1);CHECK(both.dual&&both.off_min<both.min&&both.off_rating>0);
 View sent,received;sent.level=&data.town;sent.has_character=false;sent.attack_lines[0]=both;ViewEncoder encoder;CHECK(apply_view(data,encode_view(data,sent,encoder),received));CHECK(received.attack_lines[0].dual&&received.attack_lines[0].off_min==both.off_min&&received.attack_lines[0].off_rating==both.off_rating);
 for(auto name:{"Vengeance","Blessed Hammer","Double Swing","Double Throw","Frenzy","Hunger","Dragon Claw","Blade Fury","Poison Javelin","Freezing Arrow","Charged Strike","Rabies","Fire Wall","Inferno"}){const auto* skill=f.skill_named(name);CHECK(skill);CHECK(f.panel_attack(*skill,10).damage);}
 auto* wall=f.skill_named("Fire Wall");CHECK(f.panel_attack(*wall,10).per_second);
 // Hunger calc1 is applied once, not once by skill_swing and again by hit.
 f.dual_wield=false;f.player_combat=rule::simple_fighter(100,100,1000000,0);f.player_combat.phys_lo=f.player_combat.phys_hi=25600;f.player_combat.ar_base=1000000;
 auto* hunger=f.skill_named("Hunger");CHECK(hunger);auto target=make_monster(data,data.monsters.row("quillrat1"),20.5f,20,f.rng,0);target.hit_points=target.stats.hit_points=10000;target.boss_res={};f.monsters={target};f.attack_mon=0;f.swing_skill=hunger->id;f.rng=rule::Rng{3};
 const auto panel=f.panel_attack(*hunger,10);const auto seed=f.rng.low;CHECK(panel.min==25&&f.rng.low==seed);f.hit(1000);CHECK(10000-f.monsters[0].hit_points==25);
 std::puts("PASS selected attack panel: shield-only Smite, boot-only kicks, separate hand damage/rating and View roundtrip, spell/throw/poison displays, damage-per-second marker; Hunger single reduction actual hit");
}

static void missile_damage_function_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(320*320);w.level=&arena;w.player.x=w.player.y=20;
 auto& f=w.fight;auto& c=w.character;c.character_class=3;c.stats.values[kLevel]=40;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;
 const auto enemy=[&](const char* code){auto m=make_monster(data,data.monsters.row(code),21,20,f.rng,0);m.id=900;m.hit_points=m.stats.hit_points=1000000;m.boss_res={};return m;};
 const auto cast=[&](const char* name,const char* missile,int level){const auto* skill=f.skill_named(name);CHECK(skill);auto it=data.missiles.find(missile);CHECK(it!=data.missiles.end());Missile m;m.info=&it->second;m.skill=skill->id;m.level=level;m.friendly=true;return m;};
 for(auto [name,missile,fn]:{std::tuple{"Ice Blast","iceblast",4},std::tuple{"Ice Arrow","icearrow",2},std::tuple{"Blades of Ice","bladesoficecubes",10}}){
  f.monsters={enemy("fallen1")};auto m=cast(name,missile,10);CHECK(m.info->dmg_func==fn);f.player_combat=rule::simple_fighter(100,100,1000000,0);f.player_combat.ar_base=1000000;f.rng=rule::Rng{3};f.strike(m,0,1000);CHECK(f.monsters[0].freeze_until>1000);
 }
 for(auto [name,missile,fn]:{std::tuple{"War Cry","warcry",7},std::tuple{"Twister","twister",9},std::tuple{"Shock Wave","shockwave",7}}){
  f.monsters={enemy("fallen1")};auto m=cast(name,missile,10);CHECK(m.info->dmg_func==fn);f.strike(m,0,2000);CHECK(f.monsters[0].stun_until>2000);
 }
 auto hammer=cast("Blessed Hammer","blessedhammer",10);CHECK(hammer.info->dmg_func==5);
 const auto hit=[&](const char* kind,Missile& m){f.monsters={enemy(kind)};f.rng=rule::Rng{123};const int hp=f.monsters[0].hit_points;f.strike(m,0,3000);return hp-f.monsters[0].hit_points;};
 const int plain=hit("quillrat1",hammer),undead=hit("zombie1",hammer),demon=hit("fallen1",hammer);CHECK(plain>0&&undead>=plain*3/2-1&&demon>=plain*3/2-1);
 f.pending.clear();const auto* conc=f.skill_named("Concentration");const auto* blessed=f.skill_named("Blessed Hammer");CHECK(conc&&blessed);
 f.aura_sources={{conc->id,10,false,0,20}};f.fire(*blessed,4000,Fight::Caster{20,20,10,-1,21,20,false,false});CHECK(!f.pending.empty()&&f.pending.back().spell_pct>0);const int pct=f.pending.back().spell_pct;f.aura_sources.clear();CHECK(f.pending.back().spell_pct==pct);
 auto magic=cast("Magic Arrow","magicarrow",10);CHECK(magic.info->dmg_func==1&&magic.info->dmg_calc=="dl12");auto ordinary=*magic.info;ordinary.dmg_func=0;auto no_conversion=magic;no_conversion.info=&ordinary;
 const auto against_immune=[&](Missile& m){f.monsters={enemy("quillrat1")};f.monsters[0].boss_res[0]=100-data.monsters.types[std::size_t(f.monsters[0].type)].diff[0].res[0];f.player_combat=rule::simple_fighter(1000,1000,1000000,0);f.player_combat.phys_lo=f.player_combat.phys_hi=256000;f.player_combat.ar_base=1000000;f.rng=rule::Rng{3};const int hp=f.monsters[0].hit_points;f.strike(m,0,5000);return hp-f.monsters[0].hit_points;};
 {const int converted=against_immune(magic),ordinary_damage=against_immune(no_conversion);CHECK(converted>ordinary_damage+50);}
 auto bolt=cast("Lightning Bolt","lightningjavelin",10);CHECK(bolt.info->dmg_func==12&&against_immune(bolt)>500);
 CHECK(std::ranges::any_of(data.monsters.types,[](const auto& type){return type.small;}));CHECK(std::ranges::any_of(data.monsters.types,[](const auto& type){return type.large;}));
 auto boulder=cast("Molten Boulder","moltenboulder",10);CHECK(boulder.info->dmg_func==14&&boulder.info->dmg_param[1]==50);
 std::puts("PASS missile damage functions: actual IceBlast/IceArrow/ice cubes freeze, WarCry/ShockWave/Twister stun, Hammer undead/demon bonus and cast-time Concentration, MagicArrow/LightningBolt physical conversion, original boulder size metadata");
}

static void animation_and_motion_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(320*320);w.level=&arena;w.player.x=w.player.y=20;
 auto& f=w.fight;auto& c=w.character;c.character_class=4;c.stats.values[kLevel]=40;c.stats.values[kLife]=c.stats.values[kMaxLife]=25600;
 for(const char* name:{"icebreaksmall","icebreakmedium","icebreaklarge","icebreaksmoke"}){const auto it=data.missiles.find(name);CHECK(it!=data.missiles.end()&&!it->second.cel_file.empty()&&it->second.anim_len>0);}
 auto monster=make_monster(data,data.monsters.row("fallen1"),20,20,f.rng,0);monster.id=99;monster.hit_points=1;monster.freeze_until=2000;
 CHECK(hurt(data,monster,100,1000)&&monster.corpse_used);const auto seed=f.rng.low;
 f.shatter(monster,1000);CHECK(monster.shatter_visual&&f.pending.size()==9&&f.rng.low==seed);
 for(const auto& m:f.pending)CHECK(m.visual_only&&m.dies>1000&&m.info->name.starts_with("icebreak"));
 f.shatter(monster,1000);CHECK(f.pending.size()==9);
 {const auto first=f.pending.front();f.missiles={first};f.pending.clear();f.missile_tick(first.dies);CHECK(f.pending.size()==1&&f.pending.front().visual_only&&f.pending.front().info->name=="icebreaksmallmelt");f.missiles.clear();f.pending.assign(9,first);}
 for(int i=0;i<100;++i){monster.shatter_visual=false;f.shatter(monster,1000);}CHECK(f.pending.size()==128&&f.rng.low==seed);f.pending.clear();
 const auto* whirl=f.skill_named("Whirlwind");const auto* leap_attack=f.skill_named("Leap Attack");CHECK(whirl&&leap_attack);
 f.swing_skill=whirl->id;f.pmode=kModeS1;CHECK(f.uninterruptible());f.apply_player_damage(200,2000);CHECK(!f.dead()&&f.death_pending&&c.stats.values[kLife]==0);
 CHECK(w.deal(cmd::Move{25,25,true})&&!w.player.walking&&w.player.x==20);f.finish_uninterruptible(2050);CHECK(!f.dead());
 f.pmode_until=2100;f.seq={};f.player_modes(false,2101,0);CHECK(f.dead()&&!f.death_pending);f.finish_uninterruptible(2200);CHECK(f.pmode==kModeDT);
 f.pmode=kModeS1;f.swing_skill=leap_attack->id;f.smove.fly=true;c.stats.values[kLife]=25600;CHECK(f.uninterruptible());f.apply_player_damage(1,3000);CHECK(c.stats.values[kLife]==25344&&!f.death_pending);
 f.smove.fly=false;CHECK(!f.uninterruptible());f.leap=Fight::Leap{20,20,25,25,3000,3500,0,1};CHECK(f.uninterruptible());f.leap.reset();CHECK(!f.uninterruptible());
 f.pmode=-1;f.apply_player_damage(1000,3600);CHECK(f.dead()&&!f.death_pending);
 for(const char* name:{"Shadow Warrior","Shadow Master"}){const auto* skill=f.skill_named(name);CHECK(skill);Fight::Pet pet;pet.monster=make_monster(data,data.monsters.row(name==std::string_view("Shadow Warrior")?"shadowwarrior":"shadowmaster"),20,20,f.rng,0);pet.monster.stats.level=40;f.equip_shadow(pet,*skill,20);CHECK(pet.shadow_gear&&pet.shadow_gear->appearance==data.look_of(pet.shadow_gear->items));}
 // The in-process client goes through the View codec too. Appearance,
 // animation pace and state overlays must survive that boundary.
 View sent,received;sent.level=&data.town;sent.has_character=false;
 View::Pet shown;shown.id=123;shown.rate=1.25f;shown.mode="A1";shown.states={"fade","enchant"};shown.gfx=data.look_of({worn("cap",1,{})});sent.pets.push_back(shown);
 ViewEncoder enc;CHECK(apply_view(data,encode_view(data,sent,enc),received));CHECK(received.pets.size()==1&&received.pets[0].id==123&&received.pets[0].rate==1.25f&&received.pets[0].states==shown.states&&received.pets[0].gfx==shown.gfx);
 // A monster's protected leap also keeps its death/credit until landing.
 auto jumper=make_monster(data,data.monsters.row("fallen1"),20,20,f.rng,0);jumper.hit_points=jumper.stats.hit_points=100;jumper.mode="SQ";jumper.leaping=true;
 CHECK(!hurt(data,jumper,20,4000)&&jumper.hit_points==80&&jumper.mode=="SQ");
 CHECK(!hurt(data,jumper,500,4010,false)&&jumper.alive()&&jumper.deferred_death&&!jumper.deferred_credit&&jumper.mode=="SQ");
 jumper.leaping=false;CHECK(hurt(data,jumper,1,4100)&&!jumper.alive()&&!jumper.deferred_death&&jumper.mode=="DT");
 jumper=make_monster(data,data.monsters.row("fallen1"),20,20,f.rng,0);jumper.id=444;jumper.leaping=true;jumper.hit_points=5;jumper.poison_rate=1000;jumper.poison_until=99999;jumper.poison_actor=2;jumper.poison_pet=999;jumper.poison_from_item=true;f.monsters={jumper};
 f.monster_dots(5000,.1f);CHECK(f.monsters[0].deferred_death&&f.monsters[0].deferred_actor==2&&f.monsters[0].deferred_pet==999&&f.monsters[0].deferred_proc);
 f.monsters.clear();
 // Vile mothers must animate their wind-up before creating a child.
 const auto mother=std::ranges::find(data.monsters.types,std::string("VileMother"),&rule::MonType::ai_name);CHECK(mother!=data.monsters.types.end());
 const int type=int(mother-data.monsters.types.begin());std::vector<Monster> pack;std::vector<Monster> born;std::vector<Missile> bolts;
 Foe foe{24,20,40,true,false,rule::simple_fighter(1,1,1,0)};foe.size=2;
 bool started=false;
 for(unsigned attempt=0;attempt<128&&!started;++attempt){auto mom=make_monster(data,type,20,20,f.rng,0);mom.id=300;mom.seed=rule::Rng{attempt+1};mom.force_sight=true;mom.aware=true;mom.mode="NU";mom.next_act=0;pack={mom};born.clear();
  monster_update(data,arena,pack[0],std::span(&foe,1),f.rng,5000,0,{},bolts,pack,&born);
  if(pack[0].nesting){started=true;CHECK(born.empty()&&pack[0].uninterruptible());const auto end=pack[0].mode_until;CHECK(end>5000);
   CHECK(!hurt(data,pack[0],10000000,5010)&&pack[0].deferred_death);
   const bool died=monster_update(data,arena,pack[0],std::span(&foe,1),f.rng,end,0,{},bolts,pack,&born);
   CHECK(died&&!pack[0].alive()&&born.size()==1&&!pack[0].nesting);
  }
 }CHECK(started);
 std::puts("PASS animations/motion: authentic ice art, one-shot bounded noncombat shards/RNG isolation; generated Shadow appearance; temporary Whirlwind/Leap protection, damage, deferred death and input release");
}

static void cold_curse_enchant_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(320*320);w.level=&arena;w.player.x=w.player.y=20;
 auto& f=w.fight;auto& c=w.character;c.character_class=1;c.stats.values[kLevel]=60;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;c.stats.values[kMana]=c.stats.values[kMaxMana]=256000;
 const auto enemy=[&](float x,float y,int id){auto m=make_monster(data,data.monsters.row("fallen1"),x,y,f.rng,0);m.id=id;m.hit_points=m.stats.hit_points=100000;m.stats.level=60;m.next_act=1000000;return m;};
 f.monsters={enemy(21,20,501)};f.update_fighters(1000);
 // Actual 1.14 differs from the old decompile: <20%, with deadcol clear.
 int yes=0,no=0;const int type=f.monsters[0].type;const bool deadcol=data.monsters.types[std::size_t(type)].dead_collision;data.monsters.types[std::size_t(type)].dead_collision=false;
 for(int n=0;n<100;++n){auto& m=f.monsters[0];m.seed=rule::Rng{unsigned(n)};f.land(0,{.hit=true,.chill_ticks=100},true,1000);rule::Rng expected{unsigned(n)};CHECK(m.cold_shatter==(expected(100)<20));yes+=m.cold_shatter;no+=!m.cold_shatter;}CHECK(yes>0&&no>0);
 f.monsters[0].cold_shatter=true;f.monsters[0].chill_until=5000;f.killed(0,2000,false);CHECK(f.monsters[0].corpse_used);f.monsters[0].corpse_used=false;f.killed(0,5000,false);CHECK(!f.monsters[0].corpse_used);
 data.monsters.types[std::size_t(type)].dead_collision=true;f.land(0,{.hit=true,.chill_ticks=100},true,2000);CHECK(!f.monsters[0].cold_shatter);data.monsters.types[std::size_t(type)].dead_collision=deadcol;
 // Dedicated Taunt: eligible target, nearest fallback, caster locking and A1.
 const auto* taunt=f.skill_named("Taunt");const auto* attract=f.skill_named("Attract");const auto* amplify=f.skill_named("Amplify Damage");const auto* enchant=f.skill_named("Enchant");const auto* fade=f.skill_named("Fade");CHECK(taunt&&attract&&amplify&&enchant&&fade);
 f.monsters={enemy(22,20,501),enemy(21,20,502)};CHECK(swaps_ai(data,f.monsters[1]));f.attack_mon=-1;f.spot(*taunt,3000,{20,20,5,-1,22,20});CHECK(f.monsters[1].special==12&&f.monsters[1].set_kind==4&&f.monsters[1].taunt_actor==0&&f.monsters[0].special!=12);
 f.monsters[1].next_act=0;f.world(true,3040,0,{});CHECK(f.monsters[1].special==12&&f.monsters[1].unit.walking);
 auto& lured=f.monsters[1];lured.unit.x=20.5f;lured.unit.y=20;lured.unit.walking=false;lured.steps.clear();lured.mode="NU";lured.next_act=0;lured.ai_state=1;f.world(true,3100,0,{});CHECK(lured.mode=="A1");
 const auto taunt_end=lured.cry.until;f.monster_states(taunt_end);CHECK(lured.special==0&&lured.set_kind==0&&lured.cry.skill<0);
 f.monsters={enemy(21,20,501)};f.attack_mon=0;f.spot(*attract,4000,{20,20,5,0,21,20});CHECK(f.monsters[0].align==1&&f.monsters[0].curse.skill==attract->id);const auto until=f.monsters[0].curse.until;
 f.spot(*amplify,4100,{20,20,20,0,21,20});CHECK(f.monsters[0].curse.skill==attract->id&&f.monsters[0].curse.until==until);
 f.monsters[0].align=0;f.monsters[0].curse={};f.monsters[0].boss=rule::Boss::unique;f.spot(*attract,4200,{20,20,5,0,21,20});CHECK(f.monsters[0].align==0&&f.monsters[0].curse.skill<0);
 // Enchant carries cast-time values to the chosen ally, not the receiver's synergies.
 Fight::Pet pet;pet.monster=enemy(23,20,800);pet.where=&arena;pet.skill=f.skill_named("Valkyrie")->id;pet.monster.stats.to_hit=100;f.pets={pet};
 w.merc=enemy(25,20,900);f.merc_st.level=60;f.merc_st.life=100000;f.merc_st.attack_rating=100;
 const auto* warmth=f.skill_named("Warmth");CHECK(warmth);const auto& ids=data.skills.class_ids[1];const auto warm_index=std::ranges::find(ids,warmth->id);CHECK(warm_index!=ids.end());c.stats.skills[std::size_t(warm_index-ids.begin())]=20;f.update_fighters(5000);
 f.aim_enchant(23,20);CHECK(f.enchant_actor==2&&f.enchant_id==800);f.enchant(*enchant,10,f.enchant_actor,f.enchant_id,5000);CHECK(f.pets[0].buffs.size()==1&&f.pets[0].buffs[0].fixed.has_value());const auto fixed=*f.pets[0].buffs[0].fixed;CHECK(f.pet_buff_stat(f.pets[0],kFireMinDamage)>0&&f.pet_buff_stat(f.pets[0],kToHitPercent)>0&&f.self_states.empty());
 auto stats=f.pet_attack_stats(f.pets[0]);CHECK(stats.attack_fire_max>0&&stats.to_hit>100);rule::Target target;target.level=60;target.res[0]=100;rule::Rng roll{2};bool hit=false;for(int n=0;n<30;++n){auto b=rule::pet_monster_blow(stats,target,roll);if(b.hit){hit=true;CHECK(b.damage>0);}}CHECK(hit);
 c.stats.skills[std::size_t(warm_index-ids.begin())]=0;f.update_fighters(5100);f.pet_buffs(f.pets[0]);CHECK(*f.pets[0].buffs[0].fixed==fixed);f.enchant(*enchant,1,2,800,5100);CHECK(f.pets[0].buffs[0].level==10&&*f.pets[0].buffs[0].fixed==fixed);
 f.aim_enchant(25,20);CHECK(f.enchant_actor==1);f.enchant(*enchant,10,1,-1,5200);CHECK(f.merc_buffs.size()==1&&f.merc_buffs[0].fixed.has_value()&&f.merc_fighter().elem[0].second>0);
 f.aim_enchant(18,18);CHECK(f.enchant_actor==0);f.enchant(*enchant,10,0,-1,5300);f.update_fighters(5300);CHECK(f.player_combat.elem[0].second>0&&f.self_states[0].fixed.has_value());
 // Fade's curse resistance also applies to summons, without imposing a
 // one-second minimum after reduction or changing noncurse marks.
 f.pets[0].buffs.clear();f.shadow_give(f.pets[0],fade->id,10);f.pets[0].buffs.push_back({fade->id,10,50000});f.pet_buffs(f.pets[0]);const int resist=f.pet_foe(f.pets[0]).fighter.curse_resistance;CHECK(resist>0&&resist<100);
 Foe cursed;cursed.cursed={amplify->id,1};f.pet_conditions(f.pets[0],cursed,6000,0);const int duration=rule::eval_calc(data.skills,amplify->auralen,f.calc_env(),amplify->id,1);CHECK(f.pets[0].monster.curse.until==6000+unsigned(rule::curse_duration(std::max(duration,25),resist))*40);
 f.pets[0].buff_stats[109]=100;const auto previous=f.pets[0].monster.curse;f.pet_conditions(f.pets[0],cursed,6100,0);CHECK(f.pets[0].monster.curse.until==previous.until);
 // Actual cast action frame keeps the selected ally even if it moves.
 f.self_states.clear();f.pets[0].buffs.clear();f.pets[0].buff_stats.clear();f.pets[0].monster.unit.x=23;f.pets[0].monster.unit.y=20;f.pmode=-1;
 const auto enchant_index=std::ranges::find(ids,enchant->id);CHECK(enchant_index!=ids.end());c.stats.skills[std::size_t(enchant_index-ids.begin())]=10;f.update_fighters(7000);CHECK(f.cast(enchant->id,7000));f.aim_enchant(23,20);f.pets[0].monster.unit.x=26;f.player_modes(false,7000+f.player_anim(kModeSC).action_ms()+1,0);CHECK(f.pets[0].buffs.size()==1&&f.pets[0].buffs[0].skill==enchant->id&&f.self_states.empty());
 // A summon aura kill must not trigger the hero's OnKill item spell.
 const auto* holy=f.skill_named("Holy Fire");const auto* nova=f.skill_named("Nova");CHECK(holy&&nova);c.items={worn("cap",1,{{196,(nova->id<<6)|1,100}})};f.update_fighters(8000);f.pets[0].monster.unit.x=20;f.pets[0].aura=holy->id;f.pets[0].aura_level=20;f.pets[0].aura_next=0;f.monsters={enemy(21,20,505)};f.monsters[0].hit_points=1;f.item_casts.clear();f.pet_aura(f.pets[0],8000);CHECK(!f.monsters[0].alive()&&f.item_casts.empty()&&f.damage_actor==0);
 std::puts("PASS cold/curse/Enchant: native20% chilled corpse shatter/expiry/deadcol, dedicated Taunt nearest/caster/A1 and boss restrictions, nonreplaceable Attract, Enchant hero/merc/summon targeting and fixed caster stats/AR/fire/melee/weak refresh, Fade duration/immunity");
}

static void shadow_passive_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);w.level=&arena;w.player.x=w.player.y=20;
 auto& f=w.fight;auto& c=w.character;c.character_class=6;c.stats.values[kLevel]=60;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;
 c.items={worn("ktr",4,{}),worn("ktr",5,{})};f.update_fighters(1000);
 const auto* warrior=f.skill_named("Shadow Warrior");const auto* master=f.skill_named("Shadow Master");const auto* mastery=f.skill_named("Claw Mastery");const auto* block=f.skill_named("Weapon Block");CHECK(warrior&&master&&mastery&&block);
 f.summon(*warrior,20,21,20,1000);CHECK(f.pets.size()==1);auto& pet=f.pets[0];CHECK(pet.shadow_gear);auto& gear=*pet.shadow_gear;
 CHECK(f.shadow_env(pet).level(mastery->id)==10&&f.shadow_env(pet).level(block->id)==10);
 const auto expected=[&](const rule::Skill& skill,int slot,int lvl){return rule::eval_calc(data.skills,skill.passive_calc[std::size_t(slot)],f.shadow_env(pet),skill.id,lvl);};
 for(bool off:{false,true}){const auto hand=off?1:0;const auto fighter=f.shadow_fighter(pet,false,off);CHECK(fighter.ar_pct==gear.hand[hand].ar_pct+expected(*mastery,0,10));CHECK(fighter.phys_pct==gear.hand[hand].phys_pct+expected(*mastery,1,10));CHECK(fighter.mastery_crit==gear.hand[hand].mastery_crit+expected(*mastery,2,10));CHECK(fighter.weapon_block==gear.hand[hand].weapon_block+expected(*block,0,10));CHECK(fighter.attack_rating==std::max(int(std::int64_t(fighter.ar_base)*(100+fighter.ar_pct)/100),1));}
 const auto kick=f.shadow_fighter(pet,true);CHECK(kick.ar_pct==gear.hand[2].ar_pct&&kick.phys_pct==gear.hand[2].phys_pct&&kick.mastery_crit==gear.hand[2].mastery_crit);
 const auto before=gear.mastery;f.shadow_give(pet,mastery->id,10);CHECK(gear.mastery==before);f.shadow_give(pet,mastery->id,20);CHECK(gear.mastery[0][1]==expected(*mastery,1,20)&&gear.mastery[0][1]>before[0][1]);f.shadow_give(pet,mastery->id,0);CHECK(gear.mastery[0][0]==0&&gear.mastery[0][1]==0&&gear.mastery[0][2]==0&&gear.mastery[0][3]>0);f.shadow_give(pet,mastery->id,10);
 auto defender=f.pet_foe(pet).fighter;CHECK(defender.weapon_block==gear.hand[0].weapon_block+expected(*block,0,10));
 rule::MonStats attack;attack.level=60;attack.to_hit=100000;attack.a1_min=attack.a1_max=100;rule::Rng rolls{42};int standing=0,walking=0,missiles=0;
 for(int n=0;n<200;++n){standing+=rule::monster_blow(defender,60,rule::Motion::standing,attack,false,rolls).blocked;walking+=rule::monster_blow(defender,60,rule::Motion::walking,attack,false,rolls).blocked;missiles+=rule::monster_blow(defender,60,rule::Motion::standing,attack,false,rolls,true).blocked;}CHECK(standing>0&&missiles>0&&walking==0);
 // Eligibility is the Shadow's private weapon type and condition, not the hero's.
 gear.items[1].code="crs";f.refresh_shadow_masteries(pet);CHECK(gear.mastery[0][1]>0&&gear.mastery[1][1]==0&&gear.mastery[0][3]==0);gear.items[0].code="crs";f.refresh_shadow_masteries(pet);CHECK((gear.mastery==decltype(gear.mastery){}));
 gear.items[0].code=gear.items[1].code="ktr";gear.items[1].max_durability=20;gear.items[1].durability=0;gear.items[1].flags|=0x100;f.refresh_shadow_masteries(pet);CHECK(gear.mastery[0][1]>0&&gear.mastery[1][1]==0&&gear.mastery[0][3]==0);gear.items[1].durability=20;gear.items[1].flags&=~0x100u;f.refresh_shadow_masteries(pet);CHECK(gear.mastery==before);
 c.items={worn("crs",4,{})};f.update_fighters(2000);CHECK(gear.mastery==before&&f.shadow_fighter(pet).phys_pct==gear.hand[0].phys_pct+before[0][1]);
 const auto& ids=data.skills.class_ids[6];const auto index=std::ranges::find(ids,mastery->id);CHECK(index!=ids.end());
 c.stats.skills[std::size_t(index-ids.begin())]=12;f.update_fighters(2500);f.pets.clear();f.summon(*warrior,20,21,20,2500);
 CHECK(f.pets.size()==1&&f.shadow_env(f.pets[0]).level(mastery->id)==16);
 CHECK(f.pets[0].shadow_gear->mastery[0][1]==rule::eval_calc(data.skills,mastery->passive_calc[1],f.shadow_env(f.pets[0]),mastery->id,16)); // generated own claw, despite owner sword
 f.pets.clear();f.summon(*master,20,21,20,3000);CHECK(f.pets.size()==1&&f.pets[0].shadow_gear);CHECK(f.shadow_env(f.pets[0]).level(mastery->id)==0&&f.shadow_env(f.pets[0]).level(block->id)==0);CHECK((f.pets[0].shadow_gear->mastery==decltype(gear.mastery){}));
 std::puts("PASS Shadow learned passives: actual Warrior level10 Claw Mastery AR/damage/critical on each own claw, kick exclusion, two-claw stationary/missile block, sword/broken eligibility, level removal/update/no accumulation, private gear and no Master borrowing");
}

static void shared_trap_cap_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);w.level=&arena;w.player.x=w.player.y=20;
 auto& f=w.fight;auto& c=w.character;c.character_class=6;c.stats.values[kLevel]=60;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;f.update_fighters(1000);
 const auto* master=f.skill_named("Shadow Master");const auto* trap=f.skill_named("Wake of Fire Sentry");const auto* other=f.skill_named("Lightning Sentry");const auto* blade=f.skill_named("Blade Sentinel");CHECK(master&&trap&&other&&blade);
 f.summon(*master,5,20,20,1000);const int parent=f.pets[0].monster.id;const auto profile=f.summon_attack(f.pets[0]);
 const auto count=[&](const auto& skill){return std::ranges::count_if(f.pets,[&](const auto& pet){return pet.monster.alive()&&data.skills.get(pet.skill)->pettype==skill.pettype;});};
 for(int n=0;n<5;++n)f.summon(*trap,5,21+float(n)*.2f,20,1100);CHECK(count(*trap)==5);const int first=f.pets[1].monster.id;
 {Fight::ActorScope actor(f.damage_actor,2),source(f.damage_pet_id,parent);Fight::SummonScope cast(f.casting_summon,profile);f.summon(*other,5,23,20,1200);}
 CHECK(count(*trap)==5&&f.pets.back().parent_pet==parent&&f.pets.back().cast_source==profile);
 const auto dead=std::ranges::find(f.pets,first,[](const auto& pet){return pet.monster.id;});CHECK(dead!=f.pets.end()&&!dead->monster.alive()&&dead->death_pending);
 // Fill with Shadow-owned traps, then a hero cast must replace one too.
 std::erase_if(f.pets,[&](const auto& p){return p.monster.id!=parent;});
 {Fight::ActorScope actor(f.damage_actor,2),source(f.damage_pet_id,parent);Fight::SummonScope cast(f.casting_summon,profile);for(int n=0;n<5;++n)f.summon(*trap,5,21+float(n)*.2f,20,1300);}
 const int shadow_trap=f.pets[1].monster.id;f.summon(*other,5,23,20,1400);CHECK(count(*trap)==5);
 const auto replaced=std::ranges::find(f.pets,shadow_trap,[](const auto& p){return p.monster.id;});CHECK(replaced!=f.pets.end()&&!replaced->monster.alive()&&replaced->death_pending);
 // Blade Sentinel uses the same native assassintrap family too.
 {Fight::ActorScope actor(f.damage_actor,2),source(f.damage_pet_id,parent);Fight::SummonScope cast(f.casting_summon,profile);f.summon_one(*blade,data.monsters.row(blade->summon),5,f.actor_env(),23,20,1500);}
 CHECK(blade->pettype==trap->pettype&&count(*trap)==5&&count(*blade)==5&&f.pets.back().monster.stats.level==60);
 CHECK(std::ranges::any_of(f.pets,[&](const auto& p){return p.monster.id==parent&&p.monster.alive();}));
 std::puts("PASS shared trap caps: hero/Shadow replace each other's oldest trap, common five-slot sentry family across skills, delayed death preserved, own child profile retained, shared Blade Sentinel family and native caster level");
}

static void summon_death_cases(GameData& data){
 World w(&data,-1,-1);Level arena,elsewhere;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);w.level=&arena;w.player.x=w.player.y=30;
 auto& f=w.fight;auto& c=w.character;c.character_class=2;c.stats.values[kLevel]=60;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;c.stats.values[kMana]=c.stats.values[kMaxMana]=0;
 c.items={worn("cap",1,{{kPassiveFireMastery,0,1000},{kPassiveLightningMastery,0,1000}})};f.update_fighters(1000);
 const auto* fire=f.skill_named("FireGolem");const auto* clay=f.skill_named("Clay Golem");const auto* master=f.skill_named("Shadow Master");const auto* warrior=f.skill_named("Shadow Warrior");const auto* nova=f.skill_named("Nova");CHECK(fire&&clay&&master&&warrior&&nova);
 CHECK(master->summon_umod==42&&warrior->summon_umod==42);
 const int zombie=data.monsters.row("zombie1");CHECK(zombie>=0);
 const auto enemy=[&](int id,float x=20.8f){auto m=make_monster(data,zombie,x,20,f.rng,0);m.id=id;m.stats.hit_points=m.hit_points=5000;m.stats.level=10;m.killable=true;m.mode="NU";for(std::size_t k=0;k<6;++k)m.boss_res[k]=-data.monsters.types[std::size_t(zombie)].diff[0].res[k];m.stats.armor_class=0;return m;};
 const auto reset=[&](const rule::Skill& skill){f.pets.clear();f.monsters={enemy(901),enemy(902,21.4f)};f.pending.clear();f.missiles.clear();f.item_casts.clear();w.level=&arena;std::ranges::fill(arena.walk,0);f.summon(skill,5,20,20,1000);CHECK(f.pets.size()==1);};
 for(int path=0;path<3;++path){reset(*fire);auto& pet=f.pets[0];CHECK(pet.death_mod==31);const int id=pet.monster.id;
  if(path==0)f.pet_hurt(pet,pet.monster.hit_points+1,1000);
  else if(path==1){rule::IncomingChannels raw{};raw[0]=100000000;f.receive_damage(2,id,raw,1000);}
  else{pet.until=1000;f.pets_turn(1000,0,{});}
  CHECK(!f.pets[0].monster.alive()&&f.pets[0].death_started&&f.pets[0].death_pending&&f.pets[0].death_at==1160);
  f.pets[0].monster.mode_until=1001;f.pets_turn(1159,0,{});CHECK(f.monsters[0].hit_points==5000&&f.pets.size()==1);
  const auto life=c.stats.get(kLife);f.pets_turn(1160,0,{});CHECK(f.monsters[0].hit_points==4800&&f.monsters[1].hit_points==5000&&c.stats.get(kLife)==life);
  CHECK(std::ranges::count_if(f.pending,[](const auto& m){return m.visual_only&&m.info&&m.info->name=="monstercorpseexplode";})==1);
  f.pets_turn(2000,0,{});CHECK(f.monsters[0].hit_points==4800&&f.pets.empty());
 }
 // Each channel resolves against the actual victim; hero mastery cannot scale it.
 for(const auto [physical,fire_res,damage]:{std::tuple{50,25,125},std::tuple{100,0,100},std::tuple{0,100,100},std::tuple{100,100,0},std::tuple{-50,-50,300}}){reset(*fire);f.monsters[0].boss_res[0]+=physical;f.monsters[0].boss_res[2]+=fire_res;f.pet_hurt(f.pets[0],1000000,1000);f.pet_death_event(f.pets[0],1160);CHECK(f.monsters[0].hit_points==5000-damage);}
 for(int barrier:{1,4,0x800}){reset(*fire);arena.walk[100*320+102]=std::uint16_t(barrier);f.pet_hurt(f.pets[0],1000000,1000);f.pet_death_event(f.pets[0],1160);CHECK(f.monsters[0].hit_points==5000);}
 reset(*fire);f.monsters[0].align=2;auto ally=f.pets[0];ally.monster.id+=100;ally.idle=true;const int ally_hp=ally.monster.hit_points;f.pets.push_back(ally);f.pet_hurt(f.pets[0],1000000,1000);f.pet_death_event(f.pets[0],1160);CHECK(f.monsters[0].hit_points==5000&&f.pets[1].monster.hit_points==ally_hp);
 // Replacing a golem releases its slot but preserves the old delayed blast/location.
 reset(*fire);const int old=f.pets[0].monster.id;f.summon(*clay,5,25,20,1100);CHECK(f.pets.size()==2&&std::ranges::count_if(f.pets,[](const auto& p){return p.monster.alive();})==1);
 CHECK(f.pets[0].monster.id==old&&f.pets[0].death_pending);std::swap(f.pets[0],f.pets[1]);const auto old_pet=std::ranges::find(f.pets,old,[](const auto& p){return p.monster.id;});CHECK(old_pet!=f.pets.end());f.pet_death_event(*old_pet,1259);CHECK(f.monsters[0].hit_points==5000);f.pet_death_event(*old_pet,1260);CHECK(f.monsters[0].hit_points==4800);
 reset(*fire);f.pet_hurt(f.pets[0],1000000,1000);w.level=&elsewhere;f.pet_death_event(f.pets[0],1160);w.level=&arena;f.pet_death_event(f.pets[0],2000);CHECK(f.monsters[0].hit_points==5000&&!f.pets[0].death_pending);
 // Wake of Fire really has a nonzero native trap-death blast; the other
 // two sumumod32 rows have zero base elemental damage in original data.
 const auto* wake=f.skill_named("Wake of Fire Sentry");CHECK(wake&&wake->summon_umod==32);
 reset(*wake);auto& trap=f.pets[0];const auto& type=data.monsters.types[std::size_t(trap.monster.type)];trap.monster.stats.level=1;
 const float radius=float(type.diff[0].aip[2])/5;CHECK(radius>0&&type.diff[0].elements[0].max>type.diff[0].elements[0].min);
 f.monsters[0].unit.x=20+radius-.2f;f.monsters[1].unit.x=20+radius+.2f;f.monsters.push_back(enemy(903,20+radius*.5f));f.pet_die(trap,1000);f.pet_death_event(trap,1159);CHECK(f.monsters[0].hit_points==5000);f.pet_death_event(trap,1160);
 const int damage=5000-f.monsters[0].hit_points;CHECK(damage>=type.diff[0].elements[0].min&&damage<type.diff[0].elements[0].max&&f.monsters[1].hit_points==5000&&f.monsters[2].hit_points==f.monsters[0].hit_points&&f.pending.empty());
 reset(*wake);f.monsters[0].boss_res[2]+=100;f.pet_die(f.pets[0],1000);f.pet_death_event(f.pets[0],1160);CHECK(f.monsters[0].hit_points==5000);
 reset(*wake);f.pets[0].shots=1;f.pets[0].aura_next=0;f.trap_turn(f.pets[0],1000);CHECK(f.pets[0].death_started&&f.pets[0].death_pending);f.pending.clear();f.pet_death_event(f.pets[0],1160);CHECK(f.monsters[0].hit_points<5000);
 reset(*wake);const int cap=rule::eval_calc(data.skills,wake->petmax,f.calc_env(),wake->id,5);CHECK(cap==5);
 const int first_trap=f.pets[0].monster.id;for(int n=1;n<=cap;++n)f.summon(*wake,5,20+float(n)*.2f,20,1100);
 CHECK(f.pets.size()==std::size_t(cap+1)&&std::ranges::count_if(f.pets,[](const auto& p){return p.monster.alive();})==cap);
 const auto expired=std::ranges::find(f.pets,first_trap,[](const auto& p){return p.monster.id;});CHECK(expired!=f.pets.end()&&expired->death_pending);f.pet_death_event(*expired,1260);CHECK(f.monsters[0].hit_points<5000);
 for(const char* name:{"Inferno Sentry","Death Sentry"}){const auto* skill=f.skill_named(name);CHECK(skill&&skill->summon_umod==32);reset(*skill);f.pet_die(f.pets[0],1000);f.pet_death_event(f.pets[0],1160);CHECK(f.monsters[0].hit_points==5000&&f.pending.empty());}
 // Both Shadows have intrinsic death Nova, independent of an authored
 // item death spell. Its level follows owner's Shadow Warrior, even for Master.
 c.character_class=6;const auto& ids=data.skills.class_ids[6];const auto wi=std::ranges::find(ids,warrior->id);CHECK(wi!=ids.end());
 for(int learned:{0,20,40})for(const auto* skill:{warrior,master}){
  c.stats.skills.fill(0);c.stats.skills[std::size_t(wi-ids.begin())]=std::uint8_t(learned);f.update_fighters(1000);reset(*skill);auto& shadow=f.pets[0];const int id=shadow.monster.id;
  shadow.shadow_gear->procs={{f.OnDeath,nova->id,3,100,0}};f.pet_hurt(shadow,10000000,1000);CHECK(f.item_casts.size()==1&&shadow.death_pending);f.run_item_casts(1000);CHECK(f.pending.size()==64);f.pending.clear();
  f.pet_death_event(shadow,1159);CHECK(f.pending.empty());f.pet_death_event(shadow,1160);CHECK(f.pending.size()==64);
  const int level=std::min(learned/2+1,15);const auto shared=f.pending[0].struck;const auto profile=f.pending[0].summon_attack;CHECK(shared&&profile);
  for(const auto& shot:f.pending)CHECK(shot.level==level&&shot.owner==id&&shot.item_actor==2&&shot.friendly&&shot.skill==nova->id&&shot.struck==shared&&shot.summon_attack==profile&&shot.dies==1160+13*40);
  CHECK(profile->stats[kPassiveLightningMastery]!=f.psum[kPassiveLightningMastery]);
  const auto before=f.monsters[0].hit_points;{Fight::SummonScope owner(f.casting_summon,profile);const auto damage=rule::missile_damage(data.skills,*nova,f.actor_env(),level);CHECK(damage.ehi>0);f.strike(f.pending[0],0,1160);CHECK(before-f.monsters[0].hit_points>=damage.elo/256&&before-f.monsters[0].hit_points<=damage.ehi/256+1);}
  f.pet_death_event(shadow,2000);CHECK(f.pending.size()==64);
 }
 reset(*master);f.pets[0].shadow_gear->procs={{f.OnDeath,nova->id,3,100,0}};f.summon(*master,5,25,20,1100);CHECK(f.pets.size()==2&&f.pets[0].death_pending&&f.item_casts.size()==1);f.run_item_casts(1100);CHECK(f.pending.size()==64);f.pending.clear();f.pet_death_event(f.pets[0],1260);CHECK(f.pending.size()==64&&f.pets[1].monster.alive());
 f.new_game(0);CHECK(f.pets.empty()&&f.pending.empty()&&f.item_casts.empty());
 std::puts("PASS summon deaths: native Fire Golem two channels/radius/barriers/delay, hurt/incoming/expiry/cap paths, allies and level isolation, actual trap32 damage/exhaustion/zero rows, both Shadow intrinsic Nova levels/shared own profile plus item death event, one-shot cleanup and new-game cancellation");
}

static void shadow_finisher_cases(GameData& data){
 World w(&data,-1,-1);Level arena,elsewhere;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);w.level=&arena;w.player.x=w.player.y=20;
 auto& f=w.fight;auto& c=w.character;c.character_class=6;c.stats.values[kLevel]=60;c.stats.values[kStr]=c.stats.values[kDex]=200;c.stats.values[kLife]=128000;c.stats.values[kMaxLife]=256000;c.stats.values[kMana]=0;
 const auto* master=f.skill_named("Shadow Master");const auto* claw=data.skills.get(260);const auto* talon=data.skills.get(255);const auto* tail=data.skills.get(270);const auto* flight=data.skills.get(275);const auto* tiger=data.skills.get(254);const auto* cobra=data.skills.get(265);const auto* thunder=data.skills.get(269);CHECK(master&&claw&&talon&&tail&&flight&&tiger&&cobra&&thunder);
 f.update_fighters(1000);f.summon(*master,17,21,20,1000);auto& pet=f.pets[0];const int id=pet.monster.id;auto& gear=*pet.shadow_gear;
 gear.hand[0]=rule::simple_fighter(10,10,100000);gear.hand[1]=rule::simple_fighter(100,100,100000);gear.hand[2]=rule::simple_fighter(10,10,100000);gear.hand[2].kick_lo=gear.hand[2].kick_hi=10;
 const auto* nova=f.skill_named("Nova");const auto* frost=f.skill_named("Frost Nova");CHECK(nova&&frost);gear.items={worn("ces",4,{{198,(nova->id<<6)|1,100}}),worn("btl",5,{{198,(frost->id<<6)|1,100}})};f.refresh_shadow_procs(pet);
 auto enemy=[&](int key,float x=21.1f){auto m=make_monster(data,data.monsters.row("zombie1"),x,20,f.rng,0);m.id=key;m.stats.level=20;m.hit_points=m.stats.hit_points=100000;m.stats.armor_class=0;m.boss_res={};return m;};
 const auto hitseed=[](bool lastkick=false){for(unsigned seed=1;seed<1000;++seed){rule::Rng probe{seed};if(lastkick)probe(100);if(probe(100)<95)return seed;}return 1u;};
 const auto missseed=[](){for(unsigned seed=1;seed<1000;++seed){rule::Rng probe{seed};if(probe(100)>=5)return seed;}return 1u;};
 const auto reset=[&](const rule::Skill& skill,int lvl){f.monsters={enemy(901)};f.pending.clear();f.missiles.clear();f.item_casts.clear();pet.combo={};pet.charges.clear();pet.monster.mode="A1";pet.monster.hit_points=pet.monster.stats.hit_points;pet.monster.freeze_until=pet.monster.stun_until=0;pet.where=&arena;pet.monster.unit.x=21;pet.monster.unit.y=20;pet.target=0;pet.cast=skill.id;f.shadow_give(pet,skill.id,lvl);f.world_now=2000;};
 reset(*claw,1);f.rng=rule::Rng{hitseed()};f.shadow_cast(pet,2000,{});const int first=100000-f.monsters[0].hit_points;CHECK(first>0&&pet.combo.remaining==1&&pet.combo.next>2000&&f.item_casts.size()==1&&f.item_casts[0].skill==nova->id);
 const auto due=pet.combo.next;const int hp=f.monsters[0].hit_points;CHECK(f.shadow_combo_tick(pet,due-1)&&f.monsters[0].hit_points==hp);f.rng=rule::Rng{hitseed()};CHECK(f.shadow_combo_tick(pet,due));CHECK(pet.combo.skill<0&&hp-f.monsters[0].hit_points>first&&f.item_casts.size()==2&&f.item_casts.back().skill==frost->id);const int done=f.monsters[0].hit_points;CHECK(!f.shadow_combo_tick(pet,due+1000)&&f.monsters[0].hit_points==done);
 // A miss retains charges for the second claw; it has its own roll.
 reset(*claw,1);pet.charges={{tiger->id,6,3,100000}};f.shadow_give(pet,tiger->id,6);f.monsters[0].stats.armor_class=1000000000;gear.hand[0].ar_base=1;f.rng=rule::Rng{missseed()};f.shadow_cast(pet,2000,{});CHECK(pet.charges.size()==1&&f.monsters[0].hit_points==100000);f.monsters[0].stats.armor_class=0;f.rng=rule::Rng{hitseed()};f.shadow_combo_tick(pet,pet.combo.next);CHECK(pet.charges.empty()&&f.monsters[0].hit_points<100000);gear.hand[0].ar_base=100000;
 // Target IDs survive vector reorder, but death, range, hit recovery,
 // freeze and changing levels terminate the remaining hits.
 reset(*claw,1);f.rng=rule::Rng{hitseed()};f.shadow_cast(pet,2000,{});f.monsters.push_back(enemy(902,22));std::swap(f.monsters[0],f.monsters[1]);f.rng=rule::Rng{hitseed()};f.shadow_combo_tick(pet,pet.combo.next);CHECK(f.monsters[0].hit_points==100000&&f.monsters[1].hit_points<100000-first);
 for(int variant=0;variant<5;++variant){reset(*claw,1);f.rng=rule::Rng{hitseed()};f.shadow_cast(pet,2000,{});const int before=f.monsters[0].hit_points;
  if(variant==0){f.monsters[0].hit_points=0;f.monsters[0].mode="DT";}else if(variant==1)f.monsters[0].unit.x=100;else if(variant==2)pet.monster.mode="GH";else if(variant==3)pet.monster.freeze_until=100000;else f.pets_cross(&arena,&elsewhere,0,0);
  f.shadow_combo_tick(pet,3000);CHECK(pet.combo.skill<0&&(variant==0||f.monsters[0].hit_points==before));
 }
 reset(*talon,18);w.player.x=30;const int kicks=rule::eval_calc(data.skills,talon->calc[0],f.shadow_env(pet),talon->id,18);CHECK(kicks>=3&&kicks<=32);f.rng=rule::Rng{hitseed()};f.shadow_cast(pet,2000,{});CHECK(pet.combo.remaining==kicks-1&&f.monsters[0].mode!="KB");int landed=1;
 while(pet.combo.skill>=0){const auto next=pet.combo.next;const bool last=pet.combo.remaining==1;f.rng=rule::Rng{hitseed(last)};f.shadow_combo_tick(pet,next);++landed;if(!last)CHECK(f.monsters[0].mode!="KB");}
 CHECK(landed==kicks&&f.monsters[0].hit_points<100000);if(f.monsters[0].mode=="KB")CHECK(f.monsters[0].leap_to_x>f.monsters[0].unit.x);w.player.x=20;
 // Dragon Flight misses no longer consume charges unconditionally.
 reset(*flight,1);pet.charges={{tiger->id,6,3,100000}};gear.hand[2].ar_base=1;f.monsters[0].stats.armor_class=1000000000;f.rng=rule::Rng{missseed()};f.shadow_cast(pet,2000,{});CHECK(pet.charges.size()==1);gear.hand[2].ar_base=100000;
 // Cobra's life steal heals this Shadow, not the hero. Own current skill
 // levels and elemental releases also remain independent of hero stats.
 reset(*data.skills.get(0),1);f.shadow_give(pet,cobra->id,10);pet.charges={{cobra->id,10,3,100000}};pet.monster.hit_points=100;const auto hero_hp=c.stats.get(kLife);f.rng=rule::Rng{hitseed()};f.shadow_hit(pet,*data.skills.get(0),1,0,4,true,2000);CHECK(pet.monster.hit_points>100&&c.stats.get(kLife)==hero_hp&&c.stats.get(kMana)==0&&pet.charges.empty());
 // The ordinary AI swing path (cast=-1), not only explicit skills, is a finisher.
 reset(*data.skills.get(0),1);pet.cast=-1;pet.charges={{tiger->id,6,3,100000}};set_mode(data,pet.monster,"A1",1000);pet.monster.struck=false;f.rng=rule::Rng{hitseed()};const auto end=pet.monster.mode_until;f.pets_turn(end+1,0,{});CHECK(pet.charges.empty());
 // The actual Shadow Dragon Tail path emits fire from its own stats.
 reset(*tail,5);const float radius=float(rule::eval_calc(data.skills,tail->aurarange,f.shadow_env(pet),tail->id,5))/5;f.monsters.push_back(enemy(903,21.1f+radius*.5f));f.psum[kPassiveFireMastery]=1000;f.rng=rule::Rng{hitseed()};f.shadow_cast(pet,2000,{});CHECK(f.monsters[1].hit_points<100000&&f.monsters[1].hit_points>99000);
 reset(*data.skills.get(0),1);f.shadow_give(pet,thunder->id,8);pet.charges={{thunder->id,3,3,100000}};f.charges={{tiger->id,20,3,100000}};f.psum[kPassiveLightningMastery]=1000;f.rng=rule::Rng{hitseed()};f.shadow_hit(pet,*data.skills.get(0),1,0,4,true,2000);CHECK(pet.charges.empty()&&f.charges.size()==1&&(!f.pending.empty()||!f.missiles.empty()));
 for(const auto* list:{&f.pending,&f.missiles})for(const auto& missile:*list)CHECK(missile.item_actor==2&&missile.owner==id&&missile.summon_attack&&missile.summon_attack->stats[kPassiveLightningMastery]!=f.psum[kPassiveLightningMastery]);
 CHECK(!f.calculating_shadow&&!f.casting_summon&&f.damage_actor==0&&f.damage_pet_id==-1);
 std::printf("PASS Shadow finishers: distinct timed claw hands/events, %d Talon kicks, last-kick knockback policy, miss/charge retention, stable target/cancel conditions, Flight miss, own Cobra healing and charge release source/level isolation\n",kicks);
}
static void dragon_tail_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);w.level=&arena;auto& f=w.fight;auto& c=w.character;c.character_class=6;c.stats.values[kLevel]=60;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;f.update_fighters(1000);
 const auto* tail=data.skills.get(270);CHECK(tail);const int lvl=5,range=rule::eval_calc(data.skills,tail->aurarange,f.calc_env(),tail->id,lvl);CHECK(range>0);
 const auto enemy=[&](int id,float x,int physres,int fireres){auto m=make_monster(data,data.monsters.row("zombie1"),x,20,f.rng,0);m.id=id;m.hit_points=m.stats.hit_points=100000;const auto& res=data.monsters.types[std::size_t(m.type)].diff[0].res;m.boss_res[0]=physres-res[0];m.boss_res[2]=fireres-res[2];return m;};
 for(int resist:{0,50,100,-50}){f.monsters={enemy(911,20,resist,0),enemy(912,20+float(range)/5-.1f,0,0),enemy(913,20+float(range)/5+.5f,0,0)};f.psum[kPassiveFireMastery]=50;f.psum[kPassiveFirePierce]=0;
  const int percent=rule::eval_calc(data.skills,tail->calc[0],f.calc_env(),tail->id,lvl)+50;const int expected=rule::resisted(100,resist)*percent/100;f.dragon_tail(*tail,0,100,2000,lvl);CHECK(100000-f.monsters[1].hit_points==expected&&f.monsters[2].hit_points==100000);
 }
 f.monsters={enemy(914,20,0,100),enemy(915,20.1f,0,0)};f.dragon_tail(*tail,0,100,2000,lvl);CHECK(f.monsters[0].hit_points==100000&&f.monsters[1].hit_points<100000);
 std::puts("PASS Dragon Tail: resisted physical explosion base, no explosion from physical immunity, fire immunity by receiver, native subtile radius and fire mastery");
}
static void shadow_affix_child_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);w.level=&arena;w.player.x=w.player.y=20;
 auto& f=w.fight;auto& c=w.character;c.character_class=6;c.stats.values[kLevel]=60;c.stats.values[kStr]=c.stats.values[kDex]=200;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;c.stats.values[kMana]=0;c.items={worn("cap",1,{{kPassiveFireMastery,0,500}})};f.update_fighters(1000);
 const auto* master=f.skill_named("Shadow Master");const auto* hydra=f.skill_named("Hydra");CHECK(master&&hydra);int tested=0;std::map<int,int> distinct;
 const std::map<std::string,int> events{{"hit-skill",198},{"att-skill",195},{"gethit-skill",201},{"kill-skill",196},{"death-skill",197}};
 for(const auto* list:{&data.rules.prefixes,&data.rules.suffixes})for(const auto& affix:*list)if(affix.spawnable&&affix.frequency>0)for(const auto& mod:affix.mods){
  const auto event=events.find(mod.code);if(event==events.end())continue;const int id=std::atoi(mod.param.c_str());const auto* skill=data.skills.get(id);CHECK(skill);distinct[id]++;++tested;
  f.pets.clear();f.pending.clear();f.item_casts.clear();f.summon(*master,17,21,20,1000);const int parent=f.pets[0].monster.id;
  for(auto& item:f.pets[0].shadow_gear->items)item.props.clear();f.pets[0].shadow_gear->items.push_back(worn("cap",1,{{event->second,(id<<6)|std::clamp(mod.max,1,63),100}}));f.refresh_shadow_procs(f.pets[0]);
  auto enemy=make_monster(data,data.monsters.row("zombie1"),22,20,f.rng,0);enemy.id=801;enemy.hit_points=enemy.stats.hit_points=100000;f.monsters={enemy};
  std::vector<int> hero_heads;if(skill==hydra){f.summon(*hydra,3,24,20,1100);for(const auto& pet:f.pets)if(pet.skill==hydra->id&&pet.parent_pet<0)hero_heads.push_back(pet.monster.id);CHECK(!hero_heads.empty());}
  f.item_event(event->second,0,1200,2,parent);CHECK(!f.item_casts.empty());f.run_item_casts(1200);
  if(skill==hydra){
   const auto at=std::ranges::find_if(f.pets,[&](const Fight::Pet& pet){return pet.skill==hydra->id&&pet.parent_pet==parent;});CHECK(at!=f.pets.end()&&at->cast_source);const auto snapshot=at->cast_source;CHECK(snapshot->level==60&&snapshot->stats[kPassiveFireMastery]<f.psum[kPassiveFireMastery]);
   for(int old:hero_heads)CHECK(std::ranges::any_of(f.pets,[&](const Fight::Pet& pet){return pet.monster.id==old;}));
   auto& head=*at;head.target=0;head.cast=head.ranged;{Fight::ActorScope actor(f.damage_actor,2),source(f.damage_pet_id,head.monster.id);f.pet_cast(head,1300,{});}CHECK(!f.pending.empty());for(const auto& shot:f.pending)CHECK(shot.summon_attack==snapshot&&shot.item_actor==2);const auto fired=f.pending.front();const int hp=f.monsters[0].hit_points;for(int n=0;n<20;++n)f.strike(fired,0,1400+n*40);CHECK(f.monsters[0].hit_points<hp);
  }else if(skill->srvdofunc==rule::ServerDoFunction::kCurse)CHECK(f.monsters[0].curse.skill==id);
  else {CHECK(!f.pending.empty());for(const auto& shot:f.pending)CHECK(shot.summon_attack&&shot.item_actor==2&&shot.from_item);}
 }
 CHECK(tested>=19&&distinct.size()>=9&&distinct.contains(hydra->id));
 // A Shadow's deferred trap placement carries its own source, including
 // weapon damage for Blade Sentinel, through the actual next-turn queue.
 for(const char* name:{"Blade Sentinel","Lightning Sentry"}){
  const auto* skill=f.skill_named(name);CHECK(skill);f.pets.clear();f.pending.clear();f.item_casts.clear();f.pet_summons.clear();f.summon(*master,17,21,20,2000);const int parent=f.pets[0].monster.id;f.shadow_give(f.pets[0],skill->id,4);f.pets[0].cast=skill->id;f.pets[0].target=0;f.shadow_cast(f.pets[0],2100,{});
  CHECK(f.pet_summons.size()==1&&f.pet_summons[0].parent==parent&&f.pet_summons[0].source);const auto context=f.pet_summons[0].source;f.pets_turn(2200,0,{});
  const auto child=std::ranges::find_if(f.pets,[&](const Fight::Pet& pet){return pet.parent_pet==parent&&pet.skill==skill->id;});CHECK(child!=f.pets.end()&&child->cast_source==context);
 }
 CHECK(c.stats.get(kMana)==0&&!f.casting_summon&&f.damage_actor==0&&f.damage_pet_id==-1);
 const auto parent=std::ranges::find_if(f.pets,[](const Fight::Pet& p){return p.mirror;});CHECK(parent!=f.pets.end());const auto* nova=f.skill_named("Nova");CHECK(nova);{Fight::ActorScope actor(f.damage_actor,2),source(f.damage_pet_id,parent->monster.id);Fight::SummonScope context(f.casting_summon,f.summon_attack(*parent));f.fire(*nova,2300,{21,20,1,0,22,20,false,true});}CHECK(!f.pending.empty());f.new_game(0);CHECK(f.pending.empty()&&f.pet_summons.empty()&&f.pets.empty()&&f.missiles.empty()&&!f.casting_summon);
 std::printf("PASS Shadow active affix catalog and children: %d active spell affix rows / %zu skills including Hydra Shield; separate hero/Shadow summons, own child caster profile and actual deferred Blade Sentinel/Lightning Sentry placement\n",tested,distinct.size());
}
static void shadow_item_projectile_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);w.level=&arena;w.player.x=w.player.y=10;
 auto& f=w.fight;auto& c=w.character;c.character_class=6;c.stats.values[kLevel]=60;c.stats.values[kStr]=c.stats.values[kDex]=200;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;c.stats.values[kMana]=0;f.update_fighters(1000);
 const auto* master=f.skill_named("Shadow Master");const auto* nova=f.skill_named("Nova");const auto* frost=f.skill_named("Frost Nova");const auto* amp=f.skill_named("Amplify Damage");const auto* venom=f.skill_named("Venom");const auto* blade=f.skill_named("Blade Fury");const auto* blast=data.skills.get(251);const auto* shock=data.skills.get(256);CHECK(master&&nova&&frost&&amp&&venom&&blade&&blast&&shock);
 f.summon(*master,1,21,20,1000);CHECK(f.pets.size()==1&&f.pets[0].shadow_gear);auto& pet=f.pets[0];const int source_id=pet.monster.id;
 auto enemy=[&]{auto m=make_monster(data,data.monsters.row("zombie1"),21.1f,20,f.rng,0);m.id=601;m.hit_points=m.stats.hit_points=100000;m.stats.armor_class=0;m.stats.level=20;m.boss_res={};return m;};f.monsters={enemy()};pet.target=0;pet.cast=0;
 // Use authored legal spell properties at 100% to exercise the real combat
 // events deterministically; normal random affix generation is covered separately.
 auto props=[&](std::vector<ItemProp> weapon,std::vector<ItemProp> armor){for(auto& item:pet.shadow_gear->items){if(item.slot==4)item.props=weapon;else if(item.slot==5)item.props.clear();}pet.shadow_gear->items.push_back(worn("cap",1,std::move(armor)));f.refresh_shadow_procs(pet);};
 props({{195,(nova->id<<6)|3,100},{198,(amp->id<<6)|2,100}},{{201,(venom->id<<6)|5,100},{196,(frost->id<<6)|3,100},{197,(nova->id<<6)|4,100}});
 const auto hero_hp=c.stats.get(kLife),hero_mana=c.stats.get(kMana);for(int n=0;n<60;++n){f.item_casts.clear();f.shadow_cast(pet,1100+n*40,{});if(std::ranges::any_of(f.item_casts,[&](const auto& cast){return cast.skill==amp->id;}))break;}
 CHECK(std::ranges::any_of(f.item_casts,[&](const auto& cast){return cast.skill==nova->id;})&&std::ranges::any_of(f.item_casts,[&](const auto& cast){return cast.skill==amp->id;}));
 for(const auto& cast:f.item_casts)CHECK(cast.actor==2&&cast.pet==source_id&&cast.x==pet.monster.unit.x&&cast.summon);
 f.run_item_casts(4000);CHECK(f.monsters[0].curse.skill==amp->id&&!f.pending.empty()&&f.self_states.empty()&&c.stats.get(kMana)==hero_mana&&c.stats.get(kLife)==hero_hp);
 const auto shared=f.pending[0].summon_attack;CHECK(shared);for(const auto& shot:f.pending)CHECK(shot.item_actor==2&&shot.owner==source_id&&shot.from_item&&shot.summon_attack==shared);
 f.pending.clear();f.item_casts.clear();f.receive_damage(2,source_id,{256,0,0,0,0},4100);CHECK(std::ranges::any_of(f.item_casts,[&](const auto& cast){return cast.skill==venom->id;}));f.run_item_casts(4100);CHECK(f.pet_buff_stat(pet,kSkillPoisonOverrideLength)>0&&f.self_states.empty()&&c.stats.get(kMana)==0);
 // A triggered teleport moves the owning summon and cannot move the hero.
 const auto* teleport=f.skill_named("Teleport");CHECK(teleport);pet.shadow_gear->items.push_back(worn("rin",6,{{201,(teleport->id<<6)|1,100}}));f.refresh_shadow_procs(pet);f.monsters[0].unit.x=f.monsters[0].unit.y=30;f.item_event(Fight::OnStruck,0,4150,2,source_id);f.run_item_casts(4150);CHECK(w.player.x==10&&w.player.y==10&&pet.monster.unit.x>28&&pet.monster.unit.y>28);pet.monster.unit.x=21;pet.monster.unit.y=20;pet.shadow_gear->items.pop_back();f.refresh_shadow_procs(pet);
 // Kills dispatch the summon's own OnKill, never the hero's equipment.
 f.monsters={enemy()};f.monsters[0].hit_points=1;f.item_casts.clear();rule::Blow lethal;lethal.hit=true;lethal.phys=lethal.damage=100;f.pet_melee_land(pet,0,lethal,4200);CHECK(std::ranges::any_of(f.item_casts,[&](const auto& cast){return cast.skill==frost->id&&cast.actor==2&&cast.pet==source_id;}));
 f.item_casts.clear();f.pending.clear();f.monsters={enemy()};pet.buffs.clear();f.pet_buffs(pet);f.shadow_give(pet,blade->id,3);f.shadow_give(pet,blast->id,3);f.shadow_give(pet,shock->id,6);
 // A cast has one independent snapshot, including learned synergies, weapon
 // damage and hand-specific item events. A later owner/hero change is irrelevant.
 for(auto& item:pet.shadow_gear->items)item.props.clear();f.refresh_shadow_procs(pet);
 const auto* weaken=f.skill_named("Weaken");CHECK(weaken);pet.monster.curse={weaken->id,1,100000};f.world_now=4900;pet.cast=0;bool weakened_hit=false;
 for(int n=0;n<50&&!weakened_hit;++n){f.monsters={enemy()};const auto saved=f.rng;auto predicted_rng=saved;const auto predicted=rule::player_blow(f.shadow_fighter(pet),f.target_of(0),pet.monster.stats.level,predicted_rng);if(!predicted.hit){f.rng.next();continue;}f.shadow_cast(pet,4900,{});CHECK(f.monsters[0].hit_points==100000-predicted.damage);weakened_hit=true;}CHECK(weakened_hit);pet.monster.curse={};
 pet.cast=blade->id;f.shadow_cast(pet,5000,{});CHECK(!f.pending.empty());auto shot=f.pending.front();CHECK(shot.summon_attack&&!shot.from_item&&shot.summon_attack->level==60);
 {Fight::SummonScope scope(f.casting_summon,shot.summon_attack);const auto env=f.actor_env();CHECK(env.level(blast->id)==3&&env.base_level(shock->id)==6&&env.clvl==60&&env.stat(kLevel)==60&&env.rng);}
 f.pending.clear();const auto base_enemy=enemy();f.monsters={base_enemy};f.rng=rule::Rng{44};for(int n=0;n<20;++n)f.strike(shot,0,5100+n*40);const int expected=f.monsters[0].hit_points;CHECK(expected<100000);
 c.items={worn("crs",4,{{kMinDamage,0,50000},{kMaxDamage,0,50000},{kPoisonMinDamage,0,50000},{kPoisonMaxDamage,0,50000},{kPoisonLength,0,1000}})};c.stats.values[kStr]=c.stats.values[kDex]=5000;f.update_fighters(6000);
 f.pets[0].shadow_gear=std::make_shared<Fight::Pet::ShadowGear>(*pet.shadow_gear);f.pets[0].shadow_gear->hand[0]=rule::simple_fighter(50000,50000,100000);f.monsters={base_enemy};f.rng=rule::Rng{44};for(int n=0;n<20;++n)f.strike(shot,0,6100+n*40);CHECK(f.monsters[0].hit_points==expected);
 f.pets.clear();f.monsters={base_enemy};f.rng=rule::Rng{44};for(int n=0;n<20;++n)f.strike(shot,0,7100+n*40);CHECK(f.monsters[0].hit_points==expected);
 // A removed/replaced summon cannot receive a queued normal buff. Its
 // already queued death explosion remains at the old corpse, with old data.
 f.summon(*master,1,23,22,8000);auto& fresh=f.pets[0];CHECK(fresh.monster.id!=source_id);fresh.shadow_gear->items.push_back(worn("cap",1,{{197,(nova->id<<6)|4,100},{201,(venom->id<<6)|5,100},{198,(blade->id<<6)|3,100}}));f.refresh_shadow_procs(fresh);f.monsters={enemy()};
 f.item_event(Fight::OnStruck,0,8100,2,fresh.monster.id);f.pet_hurt(fresh,fresh.monster.hit_points+10000,8100);const auto deaths=std::ranges::count_if(f.item_casts,[](const auto& cast){return cast.death;});CHECK(deaths==1);f.pet_hurt(fresh,10,8101);CHECK(std::ranges::count_if(f.item_casts,[](const auto& cast){return cast.death;})==1);
 f.pets.clear();f.run_item_casts(8200);CHECK(f.pending.size()==64&&f.self_states.empty());for(const auto& missile:f.pending)CHECK(missile.x==23&&missile.y==22&&missile.from_item);
 // A weapon skill triggered by an item cannot trigger itself again on a
 // later impact. Child missiles retain the same guard and cast profile.
 f.pending.clear();f.summon(*master,1,21,20,9000);auto& guarded=f.pets[0];guarded.shadow_gear->items.push_back(worn("cap",1,{{198,(blade->id<<6)|3,100}}));f.refresh_shadow_procs(guarded);f.item_event(Fight::OnHit,0,9000,2,guarded.monster.id);f.run_item_casts(9000);CHECK(!f.pending.empty());auto generated=f.pending[0];CHECK(generated.from_item);f.pending.clear();f.item_casts.clear();f.monsters={enemy()};for(int n=0;n<30;++n)f.strike(generated,0,9100+n*40);CHECK(f.item_casts.empty());
 {Fight::ActorScope actor(f.damage_actor,2),id(f.damage_pet_id,guarded.monster.id),depth(f.item_proc_depth,1);Fight::SummonScope scope(f.casting_summon,f.summon_attack(guarded));f.fire(*blast,10000,{21,20,3,0,22,20,false,true});}
 CHECK(!f.pending.empty());auto shell=f.pending.back();f.pending.clear();f.burst(shell,10100);CHECK(!f.pending.empty());for(const auto& child:f.pending)CHECK(child.from_item&&child.summon_attack==shell.summon_attack&&child.owner==guarded.monster.id);
 f.pending.clear();f.item_casts.clear();f.monsters={enemy()};f.monsters[0].hit_points=1;rule::Blow poison;poison.hit=true;poison.poison=1000;poison.poison_ticks=25;{Fight::ActorScope actor(f.damage_actor,2),id(f.damage_pet_id,guarded.monster.id),depth(f.item_proc_depth,1);f.land(0,poison,false,11000);}CHECK(f.monsters[0].poison_from_item);guarded.shadow_gear->items.push_back(worn("rin",6,{{196,(nova->id<<6)|3,100}}));f.refresh_shadow_procs(guarded);f.monster_dots(11040,.04f);CHECK(!f.monsters[0].alive()&&f.item_casts.empty());
 // A queued effect belongs to its original level as well as its owner.
 f.item_event(Fight::OnHit,0,12000,2,guarded.monster.id);CHECK(!f.item_casts.empty());Level elsewhere;w.level=&elsewhere;f.run_item_casts(12000);CHECK(f.pending.empty());w.level=&arena;
 CHECK(c.stats.get(kMana)==0&&f.damage_actor==0&&f.damage_pet_id==-1&&f.item_proc_depth==0&&!f.casting_summon);
 std::puts("PASS Shadow item events and projectiles: native skills on attack/hit/kill/struck/death, own position/buffs/no hero resource use, stable-ID replacement, death once, shared weapon/synergy snapshots through gear swap/removal/children, recursion guard and actor scopes");
}
static void contagion_equipment_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);w.level=&arena;w.player.x=w.player.y=20;
 auto& f=w.fight;auto& c=w.character;c.character_class=5;c.stats.values[kLevel]=60;c.stats.values[kStr]=c.stats.values[kDex]=200;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;
 const auto* rabies=f.skill_named("Rabies");CHECK(rabies);
 auto zombie=[&](int id,float x,int res){auto m=make_monster(data,data.monsters.row("zombie1"),x,20,f.rng,0);m.id=id;m.hit_points=m.stats.hit_points=100000;m.boss_res[5]=res-data.monsters.types[std::size_t(m.type)].diff[0].res[5];return m;};
 // Pierce is read from the original caster at the contact, not frozen at
 // the first bite. It cannot break an immunity by itself.
 for(const auto scenario:std::array<std::array<int,4>,4>{{{50,0,30,80},{50,30,0,50},{100,0,99,0},{-90,0,30,200}}}){
  c.items={worn("cap",1,{{kPassivePoisonPierce,0,scenario[1]}})};c.stats.values[kLife]=256000;f.update_fighters(1000);f.rabies_strains.clear();f.rabies_wake=0;
  f.monsters={zombie(401,21,100),zombie(402,22,scenario[0])};CHECK(f.start_rabies(0,*rabies,10,1000));auto& strain=f.rabies_strains.at(f.monsters[0].rabies_strain);strain.low=strain.high=256;
  c.items={worn("cap",1,{{kPassivePoisonPierce,0,scenario[2]}})};f.update_fighters(1100);// A dead caster still exists and retains its current gear stats.
  if(scenario==std::array<int,4>{50,0,30,80}){c.stats.values[kLife]=0;f.set_pmode(kModeDT,1100);}
  const auto now=f.monsters[0].rabies_next;const int remaining=int((strain.until-now)/40);f.rabies_tick(now);CHECK(f.monsters[1].rabies_strain);
  const int total=remaining*scenario[3]/100;CHECK(std::abs(f.monsters[1].poison_rate*remaining*40-total)<.0001);CHECK((total==0)==(f.monsters[1].poison_until==0));
 }
 c.stats.values[kLife]=256000;f.set_pmode(kModeNU,20000);c.character_class=6;c.stats.skills.fill(0);c.items={worn("cap",1,{{kMaxLife,0,500000},{kMinDamage,0,5000},{kToHit,0,50000}}),worn("qui",3,{{kDex,0,5000}})};f.update_fighters(20000);
 const auto* master=f.skill_named("Shadow Master");const auto* warrior=f.skill_named("Shadow Warrior");CHECK(master&&warrior&&!data.shadow_equipment.empty());
 for(const auto* skill:{warrior,master})for(int lvl:{1,5,6,9,11,13,17}){
  f.pets.clear();const auto owner_count=c.items.size();f.summon(*skill,lvl,21,20,20000);CHECK(f.pets.size()==1);auto& pet=f.pets[0];CHECK(pet.shadow_gear);const auto& gear=*pet.shadow_gear;
  std::array<bool,11> occupied{};for(const auto& item:gear.items){CHECK(item.slot>0&&item.slot<=10&&!occupied[std::size_t(item.slot)]);occupied[std::size_t(item.slot)]=true;CHECK(item.ilvl==std::clamp(skill->par[4]+(lvl-1)*skill->par[5],1,99));CHECK(rule::item_local_stat(item,kMinDamage)<5000&&rule::item_local_stat(item,kToHit)<50000);}
  CHECK(occupied[1]&&occupied[3]&&occupied[4]&&occupied[5]&&c.items.size()==owner_count);
  if(skill==warrior){CHECK(gear.items.size()==4);for(const auto& item:gear.items)CHECK(item.quality==4);}
  else {CHECK(occupied[2]==(lvl>=17)&&occupied[7]==(lvl>=13)&&occupied[6]==(lvl>=9)&&occupied[10]==(lvl>=5));if(lvl>=17)for(const auto& item:gear.items)CHECK(item.quality==6);}
  const auto defended=f.pet_foe(pet).fighter;CHECK(defended.dr_flat==gear.hand[0].dr_flat&&defended.mdr==gear.hand[0].mdr&&defended.defense>=0);
  CHECK(pet.base_combat&&pet.base_combat->life_pct==skill->par[0]*(lvl-1)+gear.sum[kMaxLifePercent]);
  const auto right=f.shadow_fighter(pet),left=f.shadow_fighter(pet,false,true),kick=f.shadow_fighter(pet,true);CHECK(right.min>0&&left.min>0&&kick.kick_lo>0);
  c.stats.values[kStr]=c.stats.values[kDex]=5000;c.items.push_back(worn("crs",4,{{kMinDamage,0,50000},{kMaxDamage,0,50000},{kPoisonMinDamage,0,50000},{kPoisonMaxDamage,0,50000},{kPoisonLength,0,1000}}));f.update_fighters(21000);
  const auto after=f.shadow_fighter(pet);CHECK(after.phys_lo==right.phys_lo&&after.phys_hi==right.phys_hi&&after.ar_base==right.ar_base&&after.elem==right.elem&&f.shadow_fighter(pet,true).kick_lo==kick.kick_lo);
  pet.allied_stats={{kDamagePercent,200},{kToHitPercent,100}};const auto stronger=f.shadow_fighter(pet);CHECK(stronger.phys_pct==right.phys_pct+200&&stronger.ar_pct==right.ar_pct+100);pet.allied_stats.clear();
  f.monsters={zombie(403,21.1f,0)};pet.target=0;pet.cast=0;for(int n=0;n<60&&f.monsters[0].hit_points==100000;++n)f.shadow_cast(pet,22000+n*40,{});CHECK(f.monsters[0].hit_points<100000);
  c.items.resize(owner_count);c.stats.values[kStr]=c.stats.values[kDex]=200;
 }
 std::puts("PASS contagion equipment and private Shadows: current caster poison pierce/removal/immunity/negative cap/dead owner; actual MonEquip thresholds and qualities, own level-scaled life, hand/kick independence from hero gear, allied bonuses and landed attacks");
}
static void rabies_shadow_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);w.level=&arena;w.player.x=w.player.y=20;auto& f=w.fight;auto& c=w.character;
 c.character_class=5;c.stats.values[kLevel]=50;c.stats.values[kStr]=c.stats.values[kDex]=200;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;c.stats.values[kMana]=256000;
 const auto* rabies=f.skill_named("Rabies");const auto* wolf=data.skills.get(223);CHECK(rabies&&wolf&&wolf->srvdofunc==rule::ServerDoFunction::kShapeShift);const auto& druid=data.skills.class_ids[5];auto at=std::ranges::find(druid,rabies->id);CHECK(at!=druid.end());c.stats.skills[std::size_t(at-druid.begin())]=10;
 f.start_state(*wolf,1,1000);f.update_fighters(1000);const auto& maker=data.missiles.at("rabiesplague");const auto& child=data.missiles.at(maker.sub);CHECK(maker.srv_do==30&&child.hit_func==53);
 std::printf("RABIES DATA interval=%d scatter=%d child=%s vel=%d range=%d\n",maker.param1,maker.param2,child.name.c_str(),child.vel,child.range);
 const auto zombie=[&](int id,float x,float y,int poisonres=0){auto m=make_monster(data,data.monsters.row("zombie1"),x,y,f.rng,0);m.id=id;m.hit_points=m.stats.hit_points=100000;m.stats.armor_class=0;m.boss_res[5]=poisonres-data.monsters.types[std::size_t(m.type)].diff[0].res[5];return m;};
 f.monsters={zombie(201,21,20,100),zombie(202,40,40),zombie(203,45,45),zombie(204,21.1f,20)};f.monsters[3].align=2;
 f.swing_skill=rabies->id;f.attack_mon=0;std::uint32_t bite=0;for(int n=0;n<30&&!bite;++n){const auto now=2000+n*40;f.hit(now);if(f.monsters[0].rabies_strain)bite=now;}CHECK(bite&&f.monsters[0].poison_until==0); // immune carrier is still infected
 const auto strain=f.monsters[0].rabies_strain;const auto info=f.rabies_strains.at(strain);CHECK(info.until==bite+std::uint32_t(rule::elem_length(data.skills,*rabies,f.calc_env(),10))*40);
 CHECK(!f.start_rabies(0,*rabies,20,bite+1)&&f.rabies_strains.at(strain).until==info.until);CHECK(!f.start_rabies(3,*rabies,10,bite));
 f.monsters[1].unit.x=21.05f;f.monsters[1].unit.y=20;const auto pulse=f.monsters[0].rabies_next;f.rabies_tick(pulse-1);CHECK(!f.monsters[1].rabies_strain);f.rabies_tick(pulse);
 CHECK(f.monsters[1].rabies_strain==strain&&f.monsters[1].poison_until==info.until&&f.monsters[1].poison_rate>0&&!f.monsters[3].rabies_strain);
 // The first carrier may die: the infected secondary carrier keeps spreading,
 // but all descendants share the original deadline (no infinite chain).
 f.monsters[0].hit_points=0;f.monsters[0].mode="DT";f.monsters[2].unit.x=21.1f;f.monsters[2].unit.y=20;auto second=f.monsters[1].rabies_next;f.rabies_tick(second);
 CHECK(f.monsters[2].rabies_strain==strain&&f.monsters[2].poison_until==info.until);const int hp=f.monsters[2].hit_points;f.monster_dots(second+40,.04f);CHECK(f.monsters[2].hit_points<hp||f.monsters[2].dot_acc>0);
 f.monster_dots(info.until,.04f);CHECK(f.rabies_strains.empty()&&!f.monsters[1].rabies_strain&&!f.monsters[2].rabies_strain);
 // Geometry is bounded: no spreading through a blocked scene; a sparse
 // sorted grid is reused after the first pulse rather than full pair scans.
 f.monsters={zombie(210,21,20),zombie(211,22,20)};CHECK(f.start_rabies(0,*rabies,10,info.until+1000));auto when=f.monsters[0].rabies_next;std::fill(arena.walk.begin(),arena.walk.end(),4);f.rabies_tick(when);CHECK(!f.monsters[1].rabies_strain);std::fill(arena.walk.begin(),arena.walk.end(),0);f.rabies_tick(when+info.interval);CHECK(f.monsters[1].rabies_strain);
 // Poison kill events retain their mercenary source instead of switching to
 // the hero on the later periodic-damage tick.
 f.rabies_strains.clear();f.monsters={zombie(220,21,20)};f.monsters[0].hit_points=1;w.merc.emplace();w.merc->hit_points=1000;f.merc_st.life=1000;f.merc_st.level=50;const auto* nova=f.skill_named("Nova");CHECK(nova);c.items.clear();c.merc_items={worn("cap",1,{{196,(nova->id<<6)|3,100}})};f.refresh_item_effects();f.item_casts.clear();
 rule::Blow poison;poison.hit=true;poison.poison=1000;poison.poison_ticks=25;{Fight::ActorScope owner(f.damage_actor,1);f.land(0,poison,false,300000);}CHECK(f.monsters[0].poison_actor==1);f.monster_dots(300040,.04f);CHECK(!f.monsters[0].alive()&&std::ranges::any_of(f.item_casts,[](const auto& event){return event.actor==1;})&&f.damage_actor==0);
 // Actual Shadow self-cast Venom modifies its attack, not the hero's, and
 // the owner's active Venom is not silently copied a second time.
 c.merc_items.clear();f.refresh_item_effects();c.character_class=6;c.stats.skills.fill(0);const auto* venom=f.skill_named("Venom");const auto* shadow_skill=f.skill_named("Shadow Master");CHECK(venom&&shadow_skill);const auto& assassin=data.skills.class_ids[6];at=std::ranges::find(assassin,venom->id);CHECK(at!=assassin.end());c.stats.skills[std::size_t(at-assassin.begin())]=20;
 c.items={worn("crs",4,{})};f.self_states.clear();f.update_fighters(400000);f.pets.clear();f.summon(*shadow_skill,5,21,20,400000);CHECK(f.pets.size()==1);auto& shadow=f.pets[0];shadow.target=0;shadow.cast=venom->id;f.shadow_give(shadow,venom->id,5);
 f.monsters={zombie(230,21.1f,20)};const auto hero_before=f.player_combat.elem[3];f.shadow_cast(shadow,400000,{});CHECK(f.pet_buff_stat(shadow,kSkillPoisonOverrideLength)>0&&f.player_combat.elem[3]==hero_before);
 auto own=f.shadow_fighter(shadow);CHECK(own.poison_len==10&&own.elem[3].second>0);const auto own_poison=own.elem[3];
 shadow.cast=0;for(int n=0;n<30&&!f.monsters[0].poison_until;++n)f.shadow_cast(shadow,400040+n*40,{});CHECK(f.monsters[0].poison_until>0&&f.monsters[0].poison_actor==2&&f.monsters[0].poison_pet==shadow.monster.id);
 f.start_state(*venom,20,401000);f.update_fighters(401000);auto copied=f.shadow_fighter(shadow);CHECK(copied.elem[3]==own_poison&&f.player_combat.elem[3].second>own_poison.second);
 shadow.buffs.clear();f.pet_buffs(shadow);copied=f.shadow_fighter(shadow);CHECK(copied.elem[3].second==0&&copied.poison.override_length==0);
 std::puts("PASS Rabies and Shadow Venom: actual Druid bite, immune carrier, secondary spread/shared deadline/dead root/walls/expiry, merc poison kill source, real Shadow self-cast/attack and owner buff independence");
}
static void poison_item_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.teleport=1;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);w.level=&arena;w.player.x=w.player.y=20;
 auto& f=w.fight;auto& c=w.character;auto& t=data.rules;rule::Rng rng{72};c.character_class=4;c.stats.values[kLevel]=50;c.stats.values[kStr]=c.stats.values[kDex]=300;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;c.stats.values[kMana]=c.stats.values[kMaxMana]=256000;
 const auto gem_stat=[&](const char* code,int stat){int result=0;for(const auto& p:t.gem_props.at(code)[0])if(p.stat==stat)result+=p.value;return result;};
 CHECK(gem_stat("gpg",kPoisonCount)==1&&gem_stat("r07",kPoisonCount)==1);const int emerald_length=gem_stat("gpg",kPoisonLength),tal_length=gem_stat("r07",kPoisonLength);CHECK(emerald_length>0&&tal_length>0);
 const auto weapon=[&](const char* code,std::initializer_list<const char*> gems,int slot){auto a=rule::generate_item(t,code,90,2,rng);a.location=item_location::kEquipped;a.slot=slot;a.socketed=true;a.sockets=int(gems.size());for(const char* code:gems){std::optional<Item> gem=rule::generate_item(t,code,90,2,rng);CHECK(rule::insert_socket(t,a,gem)&&!gem);}return a;};
 auto emeralds=weapon("crs",{"gpg","gpg"},4);c.items={emeralds};f.update_fighters(1000);
 CHECK(f.player_combat.poison.count==2&&f.player_combat.poison_len==emerald_length&&f.player_combat.elem[3].second==2*gem_stat("gpg",kPoisonMaxDamage)*emerald_length/256);
 auto mixed=weapon("crs",{"gpg","r07"},4);c.items={mixed};f.update_fighters(1040);CHECK(f.player_combat.poison_len==(emerald_length+tal_length)/2&&f.player_combat.poison.count==2);
 // Generate two actual legal charm suffixes with different durations.
 std::vector<Item> charms;const auto charm_type=t.item_info.at("cm1").type;
 for(std::size_t n=0;n<t.suffixes.size()&&charms.size()<2;++n){const auto& suffix=t.suffixes[n];if(!suffix.spawnable||suffix.frequency<=0||!std::ranges::any_of(suffix.itypes,[&](const auto& type){return rule::type_is(t,charm_type,type);}))continue;
  if(!std::ranges::any_of(suffix.mods,[](const auto& mod){return mod.code=="dmg-pois";}))continue;
  rule::ItemRollOptions forced;forced.suffixes={int(n)};auto charm=rule::generate_item(t,"cm1",90,4,rng,nullptr,nullptr,false,0,&forced);const int length=rule::item_local_stat(charm,kPoisonLength);if(length<=0||(!charms.empty()&&length==rule::item_local_stat(charms[0],kPoisonLength)))continue;
  CHECK(rule::item_local_stat(charm,kPoisonCount)>0);charm.location=item_location::kStored;charm.panel=item_panel::kInventory;charm.column=int(charms.size());charms.push_back(charm);
 }
 CHECK(charms.size()==2);c.items={mixed,charms[0],charms[1]};f.update_fighters(1080);const auto all=f.player_combat;
 const int length_sum=emerald_length+tal_length+rule::item_local_stat(charms[0],kPoisonLength)+rule::item_local_stat(charms[1],kPoisonLength);
 const int sources=2+rule::item_local_stat(charms[0],kPoisonCount)+rule::item_local_stat(charms[1],kPoisonCount);CHECK(all.poison.count==sources&&all.poison_len==length_sum/sources);
 // Count has no Save Bits; original item decode must recreate one source
 // per poison list, while gem sources are reconstructed by Gems.txt.
 CHECK(data.item_tables&&data.item_tables->stats[kPoisonCount].save_bits==0);Header header;header.name="PoisonTest";header.level=50;header.status=0x20;
 const auto imported=parse_items(write_save({},header,c.stats,c.items,*data.item_tables),*data.item_tables);CHECK(imported.size()==3&&rule::item_local_stat(imported[1],kPoisonCount)==1&&rule::item_local_stat(imported[2],kPoisonCount)==1);c.items=imported;f.update_fighters(1120);CHECK(f.player_combat.poison_len==all.poison_len&&f.player_combat.elem[3]==all.elem[3]);
 // The other weapon contributes neither its poison rate, duration nor count.
 auto off=weapon("crs",{"r07"},5);c.items={mixed,off,charms[0]};f.update_fighters(1160);
 CHECK(f.dual_wield&&f.player_combat.poison.count==2+rule::item_local_stat(charms[0],kPoisonCount)&&f.player_offhand.poison.count==1+rule::item_local_stat(charms[0],kPoisonCount));
 CHECK(f.player_combat.poison_len==(emerald_length+tal_length+rule::item_local_stat(charms[0],kPoisonLength))/f.player_combat.poison.count);
 CHECK(f.player_offhand.poison_len==(tal_length+rule::item_local_stat(charms[0],kPoisonLength))/f.player_offhand.poison.count);
 c.items[1].slot=11;f.update_fighters(1200);CHECK(f.player_combat.poison.count==2+rule::item_local_stat(charms[0],kPoisonCount));
 // A real Venom state carries stat101, not a longer item poison duration.
 const auto* venom=f.skill_named("Venom");CHECK(venom);c.character_class=6;c.stats.skills.fill(0);const auto& assassin=data.skills.class_ids[6];auto at=std::ranges::find(assassin,venom->id);CHECK(at!=assassin.end());c.stats.skills[std::size_t(at-assassin.begin())]=5;
 c.items={mixed,charms[0],charms[1]};f.start_state(*venom,5,2000);f.update_fighters(2000);const auto ends=f.self_states.back().until;
 const int override_length=int(f.psum[kSkillPoisonOverrideLength]);CHECK(override_length>0&&f.player_combat.poison_len==override_length&&f.player_combat.poison.count==sources);
 f.update_fighters(ends);CHECK(f.player_combat.poison.override_length==0&&f.player_combat.poison_len==length_sum/sources);
 // Mercenary equipment and an authored item-cast buff use the same rules.
 w.merc.emplace();w.merc->hit_points=1000;w.merc->mode="NU";f.merc_st.level=50;f.merc_st.life=1000;c.merc_items={mixed};f.world_now=3000;
 CHECK(f.merc_fighter().poison_len==(emerald_length+tal_length)/2);f.add_merc_buff(*venom,5,3000);CHECK(f.merc_fighter().poison_len==override_length);f.merc_buffs.clear();
 // Poison Dagger combines native skill rate/length with item poison, then
 // actually lands on a monster and feeds its periodic damage processor.
 c.character_class=2;c.stats.skills.fill(0);const auto* dagger=f.skill_named("Poison Dagger");CHECK(dagger);const auto& necro=data.skills.class_ids[2];at=std::ranges::find(necro,dagger->id);CHECK(at!=necro.end());c.stats.skills[std::size_t(at-necro.begin())]=10;
 auto knife=weapon("kri",{"gpg","r07"},4);c.items={knife,charms[0]};c.header.left_skill=dagger->id;f.update_fighters(4000);auto combined=f.player_combat;const auto base=combined.poison;
 const int skill_length=rule::elem_length(data.skills,*dagger,f.calc_env(),10);f.skill_element(combined,*dagger);
 CHECK(combined.poison_len==(skill_length+base.length)/base.count&&combined.elem[3].second>f.player_combat.elem[3].second);
 CHECK(c.panel.attack[0].damage&&c.panel.attack[0].max>=combined.elem[3].second);
 auto zombie=make_monster(data,data.monsters.row("zombie1"),21,20,f.rng,0);zombie.id=200;zombie.hit_points=zombie.stats.hit_points=100000;zombie.stats.armor_class=0;zombie.boss_res[5]=-data.monsters.types[std::size_t(zombie.type)].diff[0].res[5];f.monsters={zombie};f.swing_skill=dagger->id;f.attack_mon=0;
 std::uint32_t landed=0;for(int n=0;n<30&&!landed;++n){const auto now=5000+n*40;f.hit(now);if(f.monsters[0].poison_until)landed=now;}CHECK(landed&&f.monsters[0].poison_until==landed+std::uint32_t(combined.poison_len)*40);
 CHECK(f.monsters[0].poison_rate>0);const auto hp=f.monsters[0].hit_points;const double loss=f.monsters[0].poison_rate*40;f.monster_dots(landed+40,.04f);CHECK(std::abs((hp-f.monsters[0].hit_points)+f.monsters[0].dot_acc-loss)<.001);
 const auto stop=f.monsters[0].poison_until;const auto after=f.monsters[0].hit_points;f.monster_dots(stop,.04f);CHECK(f.monsters[0].hit_points==after);
 // Venom on a non-Assassin via a real item oskill still gives Dagger's
 // own length + override, without item-length averaging.
 c.items.push_back(worn("cap",1,{{97,venom->id,5}}));f.start_state(*venom,5,6000);f.update_fighters(6000);combined=f.player_combat;f.skill_element(combined,*dagger);CHECK(combined.poison_len==skill_length+override_length);
 std::printf("PASS real poison items: Emerald/Tal source counts and averaging, two native charm suffixes, save reconstruction, hand ownership, Venom expiry/merc, Poison Dagger actual hit+DoT and oskill Venom interaction (override %d)\n",override_length);
}
static void socket_durability_cases(GameData& data, const std::filesystem::path& directory){
 World w(&data,-1,-1);auto& f=w.fight;auto& c=w.character;auto& t=data.rules;rule::Rng rng{42};
 if(std::getenv("D2_SOCKET_AUDIT")){
  d2d::mpq::Stack files;files.push_installer(directory/"LODPatch_114d.exe");files.push(directory/"d2exp.mpq");files.push(directory/"d2data.mpq");const auto bytes=files.read(R"(data\global\excel\gems.txt)");d2d::txt::Table gems(bytes);std::map<int,int> funcs;
  for(std::size_t row=0;row<gems.size();++row)for(const char* context:{"weaponMod","helmMod","shieldMod"})for(int n=1;n<=3;++n){const auto pre=std::string(context)+std::to_string(n);const auto code=std::string(gems.get(row,pre+"Code"));if(code.empty())continue;
   const auto f=t.properties.find(rule::lower_case(code));CHECK(f!=t.properties.end());
   for(const auto& function:f->second){++funcs[function.func];if(function.func==2||function.func==4||function.func==8||function.func==20)std::printf("SOCKET_AUDIT code=%s context=%s property=%s func=%d stat=%d min=%s max=%s param=%s\n",std::string(gems.get(row,"code")).c_str(),context,code.c_str(),function.func,function.stat,std::string(gems.get(row,pre+"Min")).c_str(),std::string(gems.get(row,pre+"Max")).c_str(),std::string(gems.get(row,pre+"Param")).c_str());}
  }
  for(const auto& [func,count]:funcs)std::printf("SOCKET_FUNCTION %d %d\n",func,count);
 }
 auto sword=rule::generate_item(t,"crs",90,2,rng);sword.location=item_location::kEquipped;sword.slot=4;sword.socketed=true;sword.sockets=1;CHECK(sword.max_durability>0);
 auto rune=rule::generate_item(t,"r33",90,2,rng);std::optional<Item> held=rune;CHECK(rule::insert_socket(t,sword,held)&&!held&&sword.socketed_items.size()==1);
 c.items={sword};const auto props=gear_props(data,c.items);CHECK(std::ranges::any_of(props,[](const auto& p){return p.stat==kIndestructible&&p.value>0;}));
 const int before=c.items[0].durability;for(int k=0;k<1000;++k)f.wear(0);
 std::fprintf(stderr,"Zod wear proof: durability %d -> %d, gear indestructible present\n",before,c.items[0].durability);CHECK(c.items[0].durability==before&&!rule::broken(c.items[0]));
 CHECK(rule::indestructible(t,c.items[0])&&rule::repair_cost(t,c.items[0],"charsi",{})==0);
 const auto lines=item_lines(data,c.items[0],90);CHECK(std::ranges::none_of(lines,[](const auto& line){return line.text.find("Durability:")!=std::string::npos;}));
 auto plain=sword;plain.socketed_items.clear();c.items={plain};for(int k=0;k<1000;++k)f.wear(0);CHECK(c.items[0].durability<before);
 auto spear=rule::generate_item(t,"spr",90,2,rng);spear.socketed=true;spear.sockets=1;held=rune;CHECK(rule::insert_socket(t,spear,held));spear.location=item_location::kEquipped;spear.slot=4;c.items={spear};const auto* impale=f.skill_named("Impale");CHECK(impale);const auto durability=spear.durability;c.character_class=0;const auto& amazon=data.skills.class_ids[0];const auto impale_at=std::ranges::find(amazon,impale->id);CHECK(impale_at!=amazon.end());c.stats.skills[std::size_t(impale_at-amazon.begin())]=20;
 for(int k=0;k<100;++k)f.impale_wear(*impale);CHECK(c.items[0].durability==durability&&!rule::broken(c.items[0]));c.items[0].socketed_items.clear();for(int k=0;k<100;++k)f.impale_wear(*impale);CHECK(c.items[0].durability<durability);
 auto eth=sword;eth.ethereal=true;c.items={eth};for(int k=0;k<1000;++k)f.wear(0);CHECK(c.items[0].durability==before&&rule::repair_cost(t,c.items[0],"charsi",{})==0);
 Header h;h.name="ZodTest";h.level=90;h.status=0x20;Stats st;st.values[kLevel]=90;CHECK(data.item_tables);const auto parsed=parse_items(write_save({},h,st,c.items,*data.item_tables),*data.item_tables);CHECK(parsed.size()==1&&parsed[0].socketed_items.size()==1&&rule::indestructible(t,parsed[0]));
 rule::CubeRecipe recipe;recipe.outputs[0].kind=rule::cube_out::kUseItem;recipe.outputs[0].flags=rule::cube_out::kUnsocket;rule::CubeMatch match;match.source[0]=0;
 auto clean=rule::cube_make(t,recipe,match,{&sword},90,true,0,rng,nullptr);CHECK(clean.made&&clean.items.size()==1&&!rule::indestructible(t,clean.items[0]));c.items=clean.items;for(int k=0;k<1000;++k)f.wear(0);CHECK(c.items[0].durability<before);
 // Pul's actual armor bonus used a skipped func2, then the defense sum
 // independently omitted socket-local enhanced defense for hero and merc.
 for(const char* code:{"cap","buc"}){
  auto armor=rule::generate_item(t,code,90,2,rng);armor.socketed=true;armor.sockets=1;armor.slot=std::string_view(code)=="cap"?1:5;armor.location=item_location::kEquipped;const auto raw=armor.defense;
  held=rule::generate_item(t,"r21",90,2,rng);CHECK(rule::insert_socket(t,armor,held));CHECK(rule::item_local_stat(t,armor,kArmorPercent)==30&&armor.defense==std::max(raw+1,t.item_base.at(code).maxac+1));
  c.items={armor};c.stats.values[kDex]=0;c.stats.values[kLevel]=90;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;f.update_fighters(1000);
  const int expected=armor.defense*130/100;CHECK(c.panel.defense==expected&&f.player_combat.defense==expected);
  c.merc_items={armor};CHECK(f.merc_gear().item_def==expected);
  const auto imported=parse_items(write_save({},h,st,{armor},*data.item_tables),*data.item_tables);CHECK(imported.size()==1&&imported[0].defense==armor.defense&&rule::item_local_stat(t,imported[0],kArmorPercent)==30);
 }
 // Shael's func8 differs by host context: attack, hit recovery or block.
 for(const char* code:{"crs","cap","buc"}){
  auto host=rule::generate_item(t,code,90,2,rng);host.socketed=true;host.sockets=1;host.slot=std::string_view(code)=="crs"?4:std::string_view(code)=="cap"?1:5;host.location=item_location::kEquipped;
  held=rule::generate_item(t,"r13",90,2,rng);CHECK(rule::insert_socket(t,host,held));c.items={host};c.merc_items={host};f.update_fighters(2000);
  const int stat=host.slot==4?kFasterAttackRate:host.slot==1?kFasterGetHitRate:kFasterBlockRate;CHECK(rule::item_local_stat(t,host,stat)==20&&f.merc_gear().sum[std::size_t(stat)]==20);
  CHECK(f.player_combat.ias==(host.slot==4?20:0)&&f.player_combat.fhr==(host.slot==1?20:0)&&f.player_combat.fbr==(host.slot==5?20:0));
  if(host.slot==4)CHECK(rule::attack_ticks(16,256,f.player_combat.ias,0)<rule::attack_ticks(16,256,0,0));
 }
 std::puts("PASS socket durability: actual Zod in normal/ethereal weapons, Impale, descriptions, save reload, unsocket removal; Pul armor/shield defense on hero and merc; Shael attack/recovery/block contexts");
}
static void summon_level_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;w.level=&arena;auto& f=w.fight;auto& c=w.character;
 c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;c.character_class=2;
 int cases=0;
 for(int diff=0;diff<3;++diff){c.header.difficulty={0,0,0};c.header.difficulty[std::size_t(diff)]=0x80;
  for(int owner:{1,20,80})for(int level:{1,5,20}){
   c.stats.values[kLevel]=owner;
   for(const char* name:{"Raise Skeleton","Raise Skeletal Mage","Clay Golem","BloodGolem","FireGolem","IronGolem","Valkyrie","Dopplezon","Shadow Warrior","Shadow Master","Raven","Summon Spirit Wolf","Summon Fenris","Summon Grizzly","Oak Sage","Charged Bolt Sentry","Wake of Fire Sentry","Lightning Sentry","Inferno Sentry","Death Sentry","Blade Sentinel"}){
    const auto* skill=f.skill_named(name);CHECK(skill);const int type=data.monsters.row(skill->summon);CHECK(type>=0);f.pets.clear();
    f.summon_one(*skill,type,level,f.calc_env(),20,20,1000);CHECK(f.pets.size()==1);
    const bool explicit_level=skill->srvdofunc==rule::ServerDoFunction::kRaven||skill->srvdofunc==rule::ServerDoFunction::kDruidSummon;
    const int expected=(skill->srvdofunc==rule::ServerDoFunction::kMirrorImage||skill->srvdofunc==rule::ServerDoFunction::kBladeSentinel)?owner:explicit_level?std::max(rule::eval_calc(data.skills,skill->calc[1],f.calc_env(),skill->id,level),1):std::min(owner,level+3*owner/4);
    CHECK(f.pets[0].monster.stats.level==expected);
    if(skill->srvdofunc==rule::ServerDoFunction::kRaven)CHECK(f.pets[0].hits==skill->par[4]+(level-1)*skill->par[5]&&f.pets[0].ctrl[0]==f.pets[0].hits);
    ++cases;
   }
  }
 }
 w.merc.emplace();w.merc->hit_points=100;f.merc_st.level=12;c.stats.values[kLevel]=80;
 {Fight::ActorScope merc(f.damage_actor,1);const auto* skill=f.skill_named("Valkyrie");f.pets.clear();f.summon(*skill,1,20,20,2000);CHECK(f.pets.size()==1&&f.pets[0].merc_owner&&f.pets[0].monster.stats.level==10);}
 // A learned Summon Resist must reach the actual summon, not just the
 // owner's otherwise-unused passive stat. Unrelated Druid pets get none.
 const auto* resist=f.skill_named("Summon Resist");CHECK(resist);const auto& ids=data.skills.class_ids[2];const auto at=std::ranges::find(ids,resist->id);CHECK(at!=ids.end());c.stats.skills[std::size_t(at-ids.begin())]=5;f.update_fighters(3000);
 const int bonus=int(f.psum[kPassiveSummonResist]);CHECK(bonus>0);
 for(const char* name:{"Raise Skeleton","Raise Skeletal Mage","Clay Golem","BloodGolem","FireGolem","IronGolem","Valkyrie","Summon Spirit Wolf"}){
  const auto* skill=f.skill_named(name);const int type=data.monsters.row(skill->summon);f.pets.clear();f.summon_one(*skill,type,5,f.calc_env(),20,20,3000);auto with=f.pets[0].res;
  auto env=f.calc_env();env.stat=[&f](int stat){return stat==kPassiveSummonResist?0:int(f.psum[std::size_t(stat)]);};f.pets.clear();f.summon_one(*skill,type,5,env,20,20,3000);const auto without=f.pets[0].res;
  const bool eligible=skill->srvdofunc==rule::ServerDoFunction::kRaiseSkeleton||skill->srvdofunc==rule::ServerDoFunction::kGolem||skill->srvdofunc==rule::ServerDoFunction::kIronGolem;
  for(int k=0;k<4;++k){bool absorbed=false;const int absorb=k==0?142:k==1?144:148;
   if(k<3)for(std::size_t p=0;p<skill->passive_stat.size();++p)if(skill->passive_stat[p]==absorb&&rule::eval_calc(data.skills,skill->passive_calc[p],f.calc_env(),skill->id,5)>0)absorbed=true;
   if(k<3)for(std::size_t p=0;p<skill->aurastat.size();++p)if(skill->aurastat[p]==absorb&&rule::eval_calc(data.skills,skill->aura_calc[p],f.calc_env(),skill->id,5)>0)absorbed=true;
   if(with[std::size_t(k)]-without[std::size_t(k)]!=(eligible&&!absorbed?bonus:0))std::fprintf(stderr,"Summon resist %s channel=%d with=%d without=%d bonus=%d absorbed=%d\n",name,k,with[std::size_t(k)],without[std::size_t(k)],bonus,absorbed);
   CHECK(with[std::size_t(k)]-without[std::size_t(k)]==(eligible&&!absorbed?bonus:0));
  }
  const auto receiver=f.pet_foe(f.pets[0]);for(int k=0;k<4;++k)CHECK(receiver.fighter.res[std::size_t(k)]==without[std::size_t(k)]);
 }
 // The actual Fire Golem aura's absorption must survive pet creation and
 // the receiver conversion, and heal through the real combat commit path.
 const auto* fire=f.skill_named("FireGolem");CHECK(fire);f.pets.clear();f.summon(*fire,5,20,20,4000);CHECK(f.pets.size()==1);
 int native_absorb=0;for(std::size_t k=0;k<fire->aurastat.size();++k)if(fire->aurastat[k]==142)native_absorb+=rule::eval_calc(data.skills,fire->aura_calc[k],f.calc_env(),fire->id,5);
 for(std::size_t k=0;k<fire->passive_stat.size();++k)if(fire->passive_stat[k]==142)native_absorb+=rule::eval_calc(data.skills,fire->passive_calc[k],f.calc_env(),fire->id,5);
 CHECK(native_absorb>0&&f.pet_foe(f.pets[0]).fighter.absorb_pct[0]==std::min(native_absorb,40));
 auto& golem=f.pets[0];golem.monster.id=198;golem.monster.stats.hit_points=2000;golem.monster.hit_points=1000;golem.monster.hp_fraction=0;golem.res[0]=0;golem.monster.mode="NU";
 const auto expected=rule::incoming_damage(f.pet_foe(golem).fighter,{0,100*256,0,0,0});CHECK(expected.life_heal>0);
 f.receive_damage(2,198,{0,100*256,0,0,0},4040);CHECK(golem.monster.hit_points*256+golem.monster.hp_fraction==1000*256+expected.life_heal-expected.life_damage);
 std::printf("PASS summon base restoration: %d real-data level cases, three difficulties, hero/merc owners, Raven attack growth, learned Summon Resist, exclusions and Fire Golem absorption combat\n",cases);
}
static void real_replenish_cases(GameData& data){
 const auto& t=data.rules;const auto at=std::ranges::find(t.uniques,std::string("Titan's Revenge"),&rule::Special::index);CHECK(at!=t.uniques.end());rule::ItemRollOptions forced;forced.named=int(at-t.uniques.begin());rule::Rng rng{42};auto titan=rule::generate_item(t,at->code,90,7,rng,nullptr,nullptr,false,0,&forced);
 CHECK(titan.unique_id==forced.named&&rule::item_local_stat(titan,253)>0&&rule::total_max_stack(t,titan)>t.item_base.at(titan.code).max_stack);auto ordinary=titan;ordinary.quantity=0;ordinary.ethereal=false;rule::Store vendor;vendor.npc_id="charsi";Stats gold;gold.values[kGold]=1000;CHECK(rule::repair_cost(t,ordinary,vendor.npc_id,{})==1&&rule::store_repair(t,vendor,ordinary,gold)&&ordinary.quantity==rule::total_max_stack(t,ordinary));
 titan.id=17;titan.location=item_location::kEquipped;titan.slot=4;titan.ethereal=true;titan.quantity=0;
 World w(&data,-1,-1);auto& c=w.character;c.items={titan};w.fight.out_of_ammo(0);CHECK(c.items.size()==1&&c.items[0].quantity==0&&rule::broken(c.items[0]));
 rule::ItemRecovery recovery;const auto step=[&](std::uint32_t now){recovery.begin();for(auto& i:c.items)recovery.update(t,i,now);recovery.end();};const auto delay=std::uint32_t(2500/rule::item_local_stat(titan,253)+1)*40;
 step(1000);step(1000+delay-1);CHECK(c.items[0].quantity==0);step(1000+delay);CHECK(c.items[0].quantity==1&&!rule::broken(c.items[0])&&w.fight.ammo_index()==0);CHECK(c.items[0].ethereal&&rule::repair_cost(t,c.items[0],"charsi",{})==0);
 c.items[0].quantity=rule::total_max_stack(t,titan)-1;step(1000+delay+5000);CHECK(c.items[0].quantity==rule::total_max_stack(t,titan));
 // A real rare self-repair property, generated through Properties.txt.
 auto helm=rule::generate_item(t,"cap",30,2,rng);helm.id=18;helm.ethereal=true;const auto property=std::ranges::find_if(t.properties,[](const auto& row){return std::ranges::any_of(row.second,[](const auto& f){return f.stat==252;});});CHECK(property!=t.properties.end());
 rule::apply_mod(t,{property->first,"",20,20},helm.props,rng);CHECK(rule::item_local_stat(helm,252)==20);helm.durability=helm.max_durability-2;c.items={helm};recovery={};step(0);step(5040);CHECK(c.items[0].durability==helm.max_durability-1);step(10080);CHECK(c.items[0].durability==helm.max_durability);
 CHECK(data.item_tables);Header h;h.name="Replenish";h.level=30;h.status=0x20;Stats stats;stats.values[kLevel]=30;auto saved=c.items;saved.push_back(titan);saved.back().quantity=rule::total_max_stack(t,titan);const auto loaded=parse_items(write_save({},h,stats,saved,*data.item_tables),*data.item_tables);CHECK(loaded.size()==2&&rule::total_max_stack(t,loaded[1])==saved[1].quantity&&rule::item_local_stat(loaded[1],253)>0);
 std::puts("PASS real item replenish: Titan's Revenge extended ethereal stack, empty weapon resumes throwing, maximum clamp, native repair property and item save roundtrip");
}
static void explicit_cast_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.teleport=1;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);w.level=&arena;w.player.x=w.player.y=20;auto& f=w.fight;auto& c=w.character;c.character_class=1;c.stats.values[kLevel]=30;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;c.stats.values[kMana]=0;
 const auto* spell=f.skill_named("Teleport");CHECK(spell);const auto& ids=data.skills.class_ids[1];const auto at=std::ranges::find(ids,spell->id);CHECK(at!=ids.end());c.stats.skills[std::size_t(at-ids.begin())]=1;
 auto staff=worn("sst",4,{{kChargedSkill,(spell->id<<6)|3,(20<<8)|2}});staff.id=11;auto ring=worn("rin",6,{{kChargedSkill,(spell->id<<6)|7,(30<<8)|5}});ring.id=12;c.items={staff,ring};
 for(int k=0;k<2;++k){f.pmode=-1;w.apply(cmd::UseSkill{spell->id,22,20,-1,false,11,3},1000+1000*k);CHECK(f.pmode==kModeSC&&f.charged_cast_skill==spell->id&&f.charged_cast_level==3&&f.skill_level(spell->id)==3&&(c.items[0].props[0].value&255)==1-k&&c.stats.get(kMana)==0);}
 f.pmode=-1;w.apply(cmd::UseSkill{spell->id,22,20,-1,false,11,3},3000);CHECK(f.pmode==-1&&(c.items[1].props[0].value&255)==5&&c.stats.get(kMana)==0);
 c.stats.values[kMana]=100000;w.apply(cmd::UseSkill{spell->id,22,20},4000);CHECK(f.pmode==kModeSC&&f.charged_cast_skill==-1&&c.stats.get(kMana)<100000&&(c.items[1].props[0].value&255)==5);
 std::puts("PASS explicit cast world path: learned Teleport and two charged sources, selected level through animation, zero mana, depletion no fallback, ordinary mana restored");
}
static void ammo_fill_cases(GameData& data){
 rule::Store store;store.tab=3;store.npc_id="akara";store.perm={"aqv","cqv","key"};auto offer=worn("aqv",0,{});offer.location=item_location::kStored;offer.panel=item_panel::kInventory;offer.quantity=100;store.tabs[3]={offer};
 const auto& t=data.rules;const auto& base=t.item_base.at("aqv");CHECK(t.types.at(t.item_info.at("aqv").type).quiver);const auto vendor=t.npc_prices.at("akara");
 CHECK(rule::item_price(t,offer,store.npc_id,false,store.header)==int(rule::price_reduced((std::int64_t(base.cost)*offer.quantity/1024)*vendor.buy/1024,0)));
 Stats stats;stats.values[kLevel]=30;stats.values[kGold]=100000;auto stack=offer;stack.location=item_location::kEquipped;stack.slot=5;stack.quantity=base.max_stack-7;std::vector<Item> bag{stack};
 auto single=offer;single.quantity=1;const int unit=std::max(rule::item_price(t,single,store.npc_id,false,store.header),1);const auto before=stats.get(kGold);
 CHECK(rule::store_fill(t,store,0,bag,stats,4)==7&&bag.size()==1&&bag[0].quantity==base.max_stack&&stats.get(kGold)==before-7*unit&&store.tabs[3][0].quantity==100);
 bag[0].quantity=base.max_stack-9;stats.values[kGold]=2*unit;CHECK(rule::store_fill(t,store,0,bag,stats,4)==2&&bag[0].quantity==base.max_stack-7&&stats.get(kGold)==0&&store.notice==rule::ShopNoGold);
 stats.values[kGold]=0;const int count=bag[0].quantity;CHECK(rule::store_fill(t,store,0,bag,stats,4)==0&&bag[0].quantity==count);
 // A full backpack can still refill an equipped quiver; without a partial stack it must refuse atomically.
 stats.values[kGold]=100000;for(int cell=0;cell<40;++cell){auto i=worn("rin",0,{});i.location=item_location::kStored;i.panel=item_panel::kInventory;i.column=cell%10;i.row=cell/10;bag.push_back(i);}
 CHECK(rule::store_fill(t,store,0,bag,stats,4)==7&&bag[0].quantity==base.max_stack&&bag.size()==41);const auto gold=stats.get(kGold);CHECK(rule::store_fill(t,store,0,bag,stats,4)==0&&store.notice==rule::ShopNoRoom&&stats.get(kGold)==gold);
 bag.clear();CHECK(rule::store_fill(t,store,0,bag,stats,4)==base.max_stack&&bag.size()==1&&bag[0].quantity==base.max_stack&&bag[0].location==item_location::kStored);
 auto key=worn("key",0,{});key.quantity=1;key.location=item_location::kStored;key.panel=item_panel::kInventory;auto keys=key;keys.quantity=t.item_base.at("key").max_stack-2;bag={keys};store.tabs[3]={key};CHECK(rule::store_fill(t,store,0,bag,stats,4)==2&&bag.size()==1&&bag[0].quantity==t.item_base.at("key").max_stack);
 std::puts("PASS ammunition fill: real quiver prices, equipped-first partial quantity, affordability, no-gold/no-room atomic failures, full backpack refill, new stack, keys");
}
static void shop_fill_cases(GameData& data){
 rule::Store store;store.tab=3;store.npc_id="akara";store.perm={"hp1","mp1","tsc","isc"};
 auto stock=[](const char* code){auto i=worn(code,0,{});i.location=item_location::kStored;i.panel=item_panel::kInventory;return i;};
 auto potion=stock("hp1");potion.id=77;store.tabs[3]={potion};Stats stats;stats.values[kLevel]=30;stats.values[kGold]=100000;std::vector<Item> bag;
 const int price=rule::item_price(data.rules,potion,store.npc_id,false,store.header);CHECK(price>0);
 CHECK(rule::store_buy(data.rules,store,0,bag,stats,8)&&bag.size()==1&&bag[0].location==item_location::kBelt&&bag[0].column==0);
 CHECK(rule::store_fill(data.rules,store,0,bag,stats,8)==7&&bag.size()==8&&stats.get(kGold)==100000-8*price);
 for(const auto& bought:bag)CHECK(bought.id<0);
 const auto gold=stats.get(kGold);CHECK(rule::store_fill(data.rules,store,0,bag,stats,8)==0&&store.notice==rule::ShopBeltFull&&stats.get(kGold)==gold&&bag.size()==8);
 // Normal purchase can overflow to inventory, bulk fill cannot.
 CHECK(rule::store_buy(data.rules,store,0,bag,stats,8)&&bag.back().location==item_location::kStored);
 bag.clear();stats.values[kGold]=2*price;stats.values[kGoldBank]=0;
 CHECK(rule::store_fill(data.rules,store,0,bag,stats,16)==2&&bag.size()==2&&stats.get(kGold)==0&&store.notice==rule::ShopNoGold);
 bag.clear();stats.values[kGold]=100000;stats.values[kGoldBank]=0;
 for(int cell=0;cell<40;++cell){auto i=stock("rin");i.column=cell%10;i.row=cell/10;bag.push_back(i);}
 CHECK(rule::store_buy(data.rules,store,0,bag,stats,4)&&bag.size()==41&&bag.back().location==item_location::kBelt);
 auto mana=stock("mp1");bag={mana};bag[0].location=item_location::kBelt;bag[0].column=0;CHECK(rule::store_buy(data.rules,store,0,bag,stats,8)&&bag.back().column==1);
 bag.clear();auto book=stock("tbk");book.quantity=17;bag={book};auto scroll=stock("tsc");scroll.quantity=1;store.tabs[3]={scroll};const auto cash=stats.get(kGold);const int scroll_price=rule::item_price(data.rules,scroll,store.npc_id,false,store.header);
 CHECK(rule::store_fill(data.rules,store,0,bag,stats,8)==3&&bag.size()==1&&bag[0].quantity==20&&stats.get(kGold)==cash-3*scroll_price);
 CHECK(rule::store_fill(data.rules,store,0,bag,stats,8)==0&&store.notice==rule::ShopBookFull&&bag.size()==1);
 bag.clear();auto cap=stock("cap");store.tabs[3]={cap,cap};CHECK(rule::store_fill(data.rules,store,0,bag,stats,8)==1&&bag.size()==1&&store.tabs[3].size()==1);
 for(bool fill:{false,true}){const auto packet=encode(Command{cmd::Buy{2,3,fill}});const auto decoded=decode(packet);CHECK(decoded);const auto* buy=std::get_if<cmd::Buy>(&*decoded);CHECK(buy&&buy->stock==2&&buy->tab==3&&buy->fill==fill);}
 std::puts("PASS shop fill: belt placement/capacity/family, full inventory, partial funds, no overflow, tome cap, finite stock, Buy codec");
}
static void inferno_stream_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);w.level=&arena;w.player.x=w.player.y=20;auto& f=w.fight;auto& c=w.character;c.character_class=6;c.stats.values[kLevel]=30;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;f.world_now=1000;
 const auto* summon=f.skill_named("Inferno Sentry");CHECK(summon);
 f.summon(*summon,5,21,20,1000);CHECK(f.pets.size()==1);
 auto& pet=f.pets[0];const auto* fire=data.skills.get(pet.shot_skill);CHECK(fire&&fire->srvdofunc==rule::ServerDoFunction::kMonsterBreath);
 auto enemy=make_monster(data,data.monsters.row("zombie1"),23,20,f.rng,0);enemy.id=200;enemy.hit_points=enemy.stats.hit_points=10000;f.monsters={enemy};const auto charges=pet.ctrl[1];f.trap_fire(pet,0,1000);CHECK(f.pending.size()==1&&f.missiles.empty()&&pet.monster.breath==fire->id&&pet.ctrl[1]==charges);
 const auto first=f.pending[0];CHECK(first.friendly&&first.item_actor==2&&first.owner==pet.monster.id&&first.velocity_x>0&&first.velocity_y==0);
 const auto next=pet.monster.breath_next,until=pet.monster.breath_until;CHECK(next>1000&&until>next);
 CHECK(f.pet_breath_tick(pet,next-1)&&f.pending.size()==1);f.monsters[0].unit.x=21;f.monsters[0].unit.y=23;
 pet.wake=0;f.pets_turn(next,.04f,{});CHECK(f.pending.size()==2&&f.pending.back().velocity_x==0&&f.pending.back().velocity_y>0&&pet.ctrl[1]==charges);
 const auto count=f.pending.size();CHECK(!f.pet_breath_tick(pet,until)&&pet.monster.breath==-1&&f.pending.size()==count);
 f.trap_fire(pet,0,2000);f.monsters[0].hit_points=0;CHECK(!f.pet_breath_tick(pet,2040)&&pet.monster.breath==-1);f.monsters[0].hit_points=10000;
 f.trap_fire(pet,0,3000);pet.wake=0;pet.monster.stun_until=10000;const auto stopped=f.pending.size();f.pets_turn(3040,.04f,{});CHECK(pet.monster.breath==-1&&f.pending.size()==stopped);
 // Every actual sentry must fire its native AI skill, not its synergy.
 for(const char* name:{"Charged Bolt Sentry","Lightning Sentry","Wake of Fire Sentry","Death Sentry"}){
  f.pets.clear();f.pending.clear();f.missiles.clear();f.summon(*f.skill_named(name),5,21,20,11000);CHECK(f.pets.size()==1);auto& trap=f.pets[0];const auto& type=data.monsters.types[std::size_t(trap.monster.type)];
  const auto* attack=f.skill_named(type.skill[std::string_view(name)=="Death Sentry"?1:0]);CHECK(attack);trap.cast=attack->id;trap.target=0;f.monsters[0].hit_points=10000;
  f.pet_cast(trap,11000,{});CHECK(!f.missiles.empty()||!f.pending.empty());const auto& shot=f.pending.empty()?f.missiles.front():f.pending.front();const auto* expected=f.pet_missile(*attack);CHECK(expected&&shot.info==expected);
  if(std::string_view(name)=="Death Sentry"){
   const auto* blast=f.skill_named(type.skill[0]);CHECK(blast&&blast->srvdofunc==rule::ServerDoFunction::kCorpseExplosion);
   auto body=f.monsters[0];body.hit_points=0;body.stats.hit_points=1000;body.mode="DD";body.corpse_used=false;body.id=201;f.monsters.push_back(body);trap.cast=blast->id;trap.target=1;const int hp=f.monsters[0].hit_points;f.pet_cast(trap,11040,{});CHECK(f.monsters[1].corpse_used&&f.monsters[0].hit_points<hp);const int after=f.monsters[0].hit_points;f.pet_cast(trap,11080,{});CHECK(f.monsters[0].hit_points==after);f.monsters.resize(1);
  }
 }
 std::puts("PASS Inferno stream: actual sentry skills, five trap projectiles, timed flames, target tracking, charges, duration/death/stun, Death Sentry corpse consumption/damage");
}
static void shadow_flight_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);w.level=&arena;w.player.x=w.player.y=20;auto& f=w.fight;auto& c=w.character;c.character_class=6;c.stats.values[kLevel]=30;c.stats.values[kStr]=c.stats.values[kDex]=100;c.stats.values[kLife]=500*256;c.stats.values[kMaxLife]=1000*256;f.world_now=1000;f.pf_kick=rule::simple_fighter(100,100,100000,0);f.pf_kick.kick_lo=f.pf_kick.kick_hi=100;
 const auto* flight=f.skill_named("Dragon Flight");const auto* tap=f.skill_named("Life Tap");const auto* maiden=f.skill_named("Iron Maiden");CHECK(flight&&tap&&maiden);
 Fight::Pet shadow;shadow.monster=make_monster(data,data.monsters.row("zombie1"),21,20,f.rng,0);shadow.monster.id=100;shadow.monster.stats.level=30;shadow.monster.stats.hit_points=1000;shadow.monster.hit_points=100;shadow.where=&arena;shadow.mirror=true;shadow.shadow_gear=std::make_shared<Fight::Pet::ShadowGear>();shadow.shadow_gear->hand[2]=f.pf_kick;shadow.target=0;shadow.cast=flight->id;shadow.skill_list.push_back({.id=flight->id,.level=1,.kind=1});
 auto enemy=make_monster(data,data.monsters.row("zombie1"),22,20,f.rng,0);enemy.id=200;enemy.hit_points=enemy.stats.hit_points=10000;enemy.stats.level=30;enemy.stats.armor_class=0;enemy.curse={tap->id,1,100000};f.monsters={enemy};f.pets={shadow};const auto hero_hp=c.stats.get(kLife);f.rng=rule::Rng{1};f.shadow_cast(f.pets[0],1000,{});CHECK(f.monsters[0].hit_points<10000&&f.pets[0].monster.hit_points>100&&c.stats.get(kLife)==hero_hp);
 f.monsters={enemy};f.monsters[0].curse={};f.pets[0]=shadow;f.pets[0].monster.hit_points=1000;f.pets[0].monster.curse={maiden->id,1,100000};f.rng=rule::Rng{1};f.shadow_cast(f.pets[0],1040,{});CHECK(f.monsters[0].hit_points<10000&&f.pets[0].monster.hit_points<1000&&c.stats.get(kLife)==hero_hp&&f.damage_actor==0&&f.damage_pet_id==-1);
 std::puts("PASS Shadow Dragon Flight: own Life Tap healing and Iron Maiden reflection, hero unaffected, source scope restored");
}
static void allied_aura_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);w.level=&arena;w.player.x=w.player.y=20;auto& f=w.fight;auto& c=w.character;c.character_class=3;c.stats.values[kLevel]=30;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;f.world_now=1000;
 Fight::Pet pet;pet.monster=make_monster(data,data.monsters.row("zombie1"),21,20,f.rng,0);pet.monster.id=100;pet.monster.hit_points=pet.monster.stats.hit_points=1000;pet.monster.stats.a1_min=pet.monster.stats.a1_max=100;pet.monster.stats.to_hit=100;pet.monster.stats.armor_class=100;pet.monster.stats.level=30;pet.monster.boss_res={};pet.res={10,20,30,40};pet.where=&arena;pet.revived=true;pet.skill=95;pet.wake=100000;f.pets={pet};CHECK(!data.monsters.types[std::size_t(pet.monster.type)].no_aura);
 // Each native friendly stat reaches a real pet, without its caster-only passive.
 for(const char* name:{"Might","Blessed Aim","Concentration","Fanaticism","Defiance","Salvation","Resist Fire","Vigor","Thorns"}){
  const auto* row=f.skill_named(name);CHECK(row&&row->srvdofunc==d2d::rules::ServerDoFunction::kFriendlyAura);f.aura_sources={{row->id,3,false,0,20}};f.refresh_allied_auras();rule::StatSum expected{};
  for(std::size_t k=0;k<row->aurastat.size();++k)if(row->aurastat[k]>=0&&row->aurastat[k]!=6&&row->aurastat[k]!=110)expected[std::size_t(row->aurastat[k])]+=f.calc(*row,row->aura_calc[k],3);
  bool any=false;for(std::size_t id=0;id<expected.size();++id)if(expected[id]){any=true;CHECK(f.pet_buff_stat(f.pets[0],int(id))==expected[id]);}CHECK(any);
  const auto stats=f.pet_attack_stats(f.pets[0]);CHECK(stats.a1_min==100+expected[kDamagePercent]);CHECK(stats.to_hit==(100+expected[kToHit])*(100+expected[kToHitPercent])/100);
  const auto receiver=f.pet_foe(f.pets[0]);CHECK(receiver.fighter.defense==(100+expected[kArmorClass])*(100+expected[kArmorPercent]+expected[kSkillArmorPercent])/100);CHECK(receiver.fighter.res[0]==10+expected[kFireResist]);CHECK(receiver.fighter.thorns_pct==expected[kThornsPercent]);CHECK(f.pet_animation_rate(f.pets[0])==100+expected[kAttackRate]);
 }
 const auto* might=f.skill_named("Might");const auto* fan=f.skill_named("Fanaticism");CHECK(might&&fan);w.merc.emplace();w.merc->unit.x=w.merc->unit.y=20;w.merc->mode="NU";w.merc->hit_points=500;f.merc_st.life=1000;f.merc_st.level=30;
 f.aura_sources={{might->id,1,false,0,20},{might->id,5,true,0,20},{might->id,5,true,0,20}};f.refresh_allied_auras();const int boost=f.calc(*might,might->aura_calc[0],5);CHECK(f.pet_buff_stat(f.pets[0],kDamagePercent)==boost);
 w.merc->hit_points=0;f.refresh_allied_auras();CHECK(f.pet_buff_stat(f.pets[0],kDamagePercent)==f.calc(*might,might->aura_calc[0],1));w.merc->hit_points=500;
 f.pets[0].monster.unit.x=100;f.refresh_allied_auras();CHECK(f.pets[0].allied_stats.empty());f.pets[0].monster.unit.x=21;
 f.aura_sources={{fan->id,3,false,0,20}};f.refresh_allied_auras();CHECK(f.pet_buff_stat(f.pets[0],kDamagePercent)<f.aura_stats(false)[kDamagePercent]);
 // Native noaura and other-level/dead recipients never acquire stats or revive.
 const auto no=std::ranges::find_if(data.monsters.types,[](const auto& t){return t.no_aura;});CHECK(no!=data.monsters.types.end());f.pets[0].monster.type=int(no-data.monsters.types.begin());f.refresh_allied_auras();CHECK(f.pets[0].allied_stats.empty());f.pets[0]=pet;f.pets[0].where=nullptr;f.refresh_allied_auras();CHECK(f.pets[0].allied_stats.empty());f.pets[0]=pet;f.pets[0].monster.hit_points=0;f.refresh_allied_auras();CHECK(f.pets[0].allied_stats.empty()&&f.pets[0].monster.hit_points==0);f.pets[0]=pet;
 // Physical aura/curse modifiers are applied once, including revived arrows.
 f.aura_sources={{might->id,3,false,0,20}};f.refresh_allied_auras();const auto* weaken=f.skill_named("Weaken");CHECK(weaken);f.pets[0].monster.curse={weaken->id,1,100000};const auto stats=f.pet_attack_stats(f.pets[0]);const int pct=f.pet_buff_stat(f.pets[0],kDamagePercent)+f.pet_debuff_stat(f.pets[0],kDamagePercent);CHECK(stats.a1_min==100+pct);
 auto enemy=make_monster(data,data.monsters.row("zombie1"),21.5f,20,f.rng,0);enemy.id=200;enemy.hit_points=enemy.stats.hit_points=10000;enemy.stats.armor_class=0;enemy.stats.level=30;enemy.boss_res={};f.monsters={enemy};rule::Rng rng{1};auto fighter=f.pet_attack_fighter(f.pets[0]);fighter.ar_base=100000;const auto hit=rule::player_blow(fighter,f.target_of(0),30,rng);CHECK(hit.hit);const int hp=f.monsters[0].hit_points;f.pet_melee_land(f.pets[0],0,hit,1000,true);CHECK(f.monsters[0].hit_points==hp-hit.damage);
 f.pets[0].monster.mode="A1";const auto& arrow=data.missiles.at("skbowarrow1");const auto shot=monster_attack_missile(arrow,f.pets[0].monster,1,0,1000,false,0,&stats);CHECK(shot.src.to_hit==stats.to_hit&&shot.src.a2_min==stats.a1_min*arrow.src_damage/128+arrow.min);
 // Intrinsic summon mastery and new aura percentages add, never multiply twice.
 f.pets[0]=pet;f.pets[0].base_combat=Fight::Pet::BaseCombat{50,50,100,50,100,50,100};f.aura_sources={{might->id,3,false,0,20}};f.refresh_allied_auras();const int ed=f.pet_buff_stat(f.pets[0],kDamagePercent);CHECK(f.pet_attack_stats(f.pets[0]).a1_min==50*(200+ed)/100);
 const auto* defiance=f.skill_named("Defiance");f.aura_sources={{defiance->id,3,false,0,20}};f.refresh_allied_auras();CHECK(f.pet_foe(f.pets[0]).fighter.defense==50*(200+f.pet_buff_stat(f.pets[0],kSkillArmorPercent))/100);
 f.pets={pet};const auto* skeleton=f.skill_named("Raise Skeleton");CHECK(skeleton);f.monsters[0].hit_points=0;f.monsters[0].mode="DD";f.monsters[0].corpse_used=false;f.summon(*skeleton,5,22,20,1000);CHECK(f.pets.size()>=2&&f.pets.back().base_combat.has_value());f.pets={pet};
 // Actual Oak/Wolverine/Barbs source rows, full recipient set and range/removal.
 f.pets[0]=pet;f.aura_sources.clear();const auto* oak=f.skill_named("Oak Sage");const auto* wolf=f.skill_named("Heart of Wolverine");const auto* barbs=f.skill_named("Spirit of Barbs");CHECK(oak&&wolf&&barbs);
 f.pets={pet};f.pets[0].base_combat=Fight::Pet::BaseCombat{100,100,0,100,0,100,0,100,100};f.pets[0].monster.hit_points=f.pets[0].monster.stats.hit_points=200;f.summon(*oak,3,22,20,1000);f.refresh_allied_auras();CHECK(f.pets[0].monster.stats.hit_points==200+f.pet_buff_stat(f.pets[0],kMaxLifePercent));
 for(const auto* spirit:{oak,wolf,barbs}){
  f.pets={pet};f.aura_sources.clear();f.summon(*spirit,3,22,20,1000);CHECK(f.pets.size()==2);f.refresh_allied_auras();
  const int stat=spirit==oak?kMaxLifePercent:spirit==wolf?kDamagePercent:kThornsPercent;CHECK(f.pet_buff_stat(f.pets[0],stat)>0&&f.aura_stats(false)[std::size_t(stat)]>0&&f.aura_stats(true)[std::size_t(stat)]>0);
  if(spirit==wolf)CHECK(f.pet_buff_stat(f.pets[0],kToHitPercent)>0);
  const int maximum=f.pets[0].monster.stats.hit_points;if(spirit==oak)CHECK(maximum>1000&&f.merc_max_life()>1000);f.refresh_allied_auras();CHECK(f.pets[0].monster.stats.hit_points==maximum);
  f.pets[1].monster.hit_points=0;f.refresh_allied_auras();CHECK(f.pets[0].allied_stats.empty()&&f.pets[0].monster.stats.hit_points==1000&&f.pets[0].monster.hit_points<=1000&&f.aura_stats(false)[std::size_t(stat)]==0&&f.aura_stats(true)[std::size_t(stat)]==0);
 }
 f.pets[0].monster.hit_points=0;f.pets[0].buffs={{oak->id,3,100000}};f.pet_buffs(f.pets[0]);CHECK(f.pets[0].monster.hit_points==0);
 std::puts("PASS allied aura delivery: nine Paladin auras, strongest/noaura/range/death, Fanaticism ally share, physical hit/Revive arrow, three spirits on hero/merc/pets, health clamp and no resurrection");
}
static void pickup_gamble_corpse_cases(GameData& data){
 rule::Wearer owner{0,200,200,80};std::vector<Item> inv;auto cap=worn("cap",0,{});cap.location=0;
 CHECK(rule::auto_equip(data.rules,inv,cap,owner)&&inv.size()==1&&inv[0].slot==1);
 auto next=worn("cap",0,{});next.location=0;CHECK(!rule::auto_equip(data.rules,inv,next,owner)&&inv.size()==1&&next.code=="cap");
 inv.clear();auto unid=next;unid.quality=4;unid.identified=false;CHECK(!rule::auto_equip(data.rules,inv,unid,owner));auto heavy=worn("uar",0,{});rule::Wearer weak{0,1,1,1};CHECK(!rule::auto_equip(data.rules,inv,heavy,weak));
 auto ring=worn("rin",0,{});CHECK(rule::auto_equip(data.rules,inv,ring,owner));ring=worn("rin",0,{});CHECK(rule::auto_equip(data.rules,inv,ring,owner));ring=worn("rin",0,{});CHECK(!rule::auto_equip(data.rules,inv,ring,owner));
 inv.clear();auto arrows=worn("aqv",0,{});arrows.quantity=20;CHECK(!rule::auto_equip(data.rules,inv,arrows,owner));auto bow=worn("sbw",0,{});CHECK(rule::auto_equip(data.rules,inv,bow,owner));auto bolts=worn("cqv",0,{});bolts.quantity=20;CHECK(!rule::auto_equip(data.rules,inv,bolts,owner));CHECK(rule::auto_equip(data.rules,inv,arrows,owner));
 inv.clear();auto sword=worn("ssd",0,{});CHECK(rule::auto_equip(data.rules,inv,sword,owner));auto second=worn("ssd",0,{});CHECK(!rule::auto_equip(data.rules,inv,second,owner));auto barb=owner;barb.cls=4;CHECK(rule::auto_equip(data.rules,inv,second,barb));
 // Actual ground pickup bypasses a full inventory only when a valid empty slot exists.
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);w.level=&arena;w.player.x=w.player.y=20;auto& c=w.character;auto& f=w.fight;c.character_class=0;c.stats.values[kLevel]=30;c.stats.values[kStr]=c.stats.values[kDex]=100;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;
 for(int y=0;y<4;++y)for(int x=0;x<10;++x){auto i=worn("hp1",0,{});i.location=item_location::kStored;i.panel=item_panel::kInventory;i.column=x;i.row=y;c.items.push_back(i);}
 Loot::GroundItem ground;ground.id=900;ground.item=worn("cap",0,{});ground.item.location=0;ground.label="Cap";w.loot.ground={ground};w.loot.take(0);CHECK(w.loot.ground.empty()&&c.items.size()==41&&c.items.back().slot==1);w.loot.ground={ground};w.loot.take(0);CHECK(w.loot.ground.size()==1&&c.items.size()==41);
 // Full freeze destroys corpse usability but still grants kill/experience.
 c.items={worn("cap",1,{{155,data.monsters.row("skeleton2"),100}})};f.refresh_item_effects();auto m=make_monster(data,data.monsters.row("zombie1"),21,20,f.rng,0);m.hit_points=1;m.stats.level=30;m.stats.exp=100;m.id=10;m.freeze_until=10000;f.monsters={m};const auto experience=c.stats.get(kExp);rule::Blow lethal;lethal.hit=true;lethal.damage=10;lethal.phys=10;f.land(0,lethal,true,2000);CHECK(!f.monsters[0].alive()&&f.monsters[0].corpse_used&&f.kills.size()==1&&c.stats.get(kExp)>experience&&f.item_reanimates.empty());f.monsters[0].mode="DD";CHECK(!consumable_corpse(data,f.monsters[0])&&!revivable_corpse(data,f.monsters[0]));
 m.hit_points=1;m.freeze_until=1000;m.chill_until=10000;CHECK(hurt(data,m,10,2000)&&!m.corpse_used);m.mode="DD";CHECK(consumable_corpse(data,m));
 // Every fresh gambling window has fourteen independent offers with fixed hidden results.
 rule::Rng rng{37};auto gamble=rule::open_gamble(data.rules,"gheed",30,rng,true,0);CHECK(gamble.gamble_rolls.size()==14);int count=0,rings=0,amulets=0;std::vector<std::string> old;
 for(const auto& tab:gamble.tabs)for(const auto& shown:tab){++count;rings+=shown.code=="rin";amulets+=shown.code=="amu";old.push_back(shown.code);CHECK(shown.ilvl>=25&&shown.ilvl<=34&&shown.identified&&shown.props.empty());CHECK(data.rules.item_base.at(shown.code).level<=shown.ilvl||shown.code=="rin"||shown.code=="amu");const auto& real=gamble.gamble_rolls.at(shown.id);CHECK(real.ilvl==shown.ilvl&&real.quality>=4&&real.quality<=7);}
 CHECK(count==14&&rings>=1&&amulets>=1);
 auto refreshed=rule::open_gamble(data.rules,"gheed",30,rng,true,0);std::vector<std::string> fresh;for(const auto& tab:refreshed.tabs)for(const auto& shown:tab)fresh.push_back(shown.code);CHECK(fresh!=old);
 auto low=rule::open_gamble(data.rules,"gheed",1,rng,false,0);for(const auto& tab:low.tabs)for(const auto& shown:tab){CHECK(shown.ilvl==5&&data.rules.item_base.at(shown.code).version<100);CHECK(data.rules.item_base.at(low.gamble_rolls.at(shown.id).code).version<100);}
 // No money/no room preserve gold, hidden offer and RNG. Success consumes exactly that offer.
 gamble.tab=3;const int index=int(std::ranges::find(gamble.tabs[3],std::string("rin"),&Item::code)-gamble.tabs[3].begin());CHECK(std::size_t(index)<gamble.tabs[3].size());const auto offer=gamble.tabs[3][std::size_t(index)];const auto expected=gamble.gamble_rolls.at(offer.id);Stats purse;purse.values[kLevel]=30;std::vector<Item> bag;const auto state=rng;CHECK(!rule::store_gamble(data.rules,gamble,index,bag,purse,rng)&&gamble.gamble_rolls.size()==14&&rng.low==state.low&&rng.high==state.high);
 purse.values[kGold]=100;purse.values[kGoldBank]=1000000;for(int y=0;y<4;++y)for(int x=0;x<10;++x){auto i=worn("hp1",0,{});i.location=item_location::kStored;i.panel=item_panel::kInventory;i.column=x;i.row=y;bag.push_back(i);}
 CHECK(!rule::store_gamble(data.rules,gamble,index,bag,purse,rng)&&purse.get(kGold)==100&&purse.get(kGoldBank)==1000000&&gamble.gamble_rolls.size()==14);bag.clear();const int price=rule::gamble_price(data.rules,offer.code,30);CHECK(rule::store_gamble(data.rules,gamble,index,bag,purse,rng)&&bag.size()==1&&bag[0].code==expected.code&&bag[0].ilvl==expected.ilvl&&bag[0].quality==expected.quality&&gamble.gamble_rolls.size()==13&&!gamble.gamble_rolls.contains(offer.id));CHECK(purse.get(kGold)+purse.get(kGoldBank)==1000100-price&&rng.low==state.low&&rng.high==state.high);
 std::puts("PASS pickup gambling corpse: empty-slot autoequip/requirements/hands/full inventory; frozen corpse unavailable with kill credit; fourteen hidden fixed offers, reopen refresh, classic and atomic purchases");
}
static void skill_completion_cases(GameData& data){
 World w(&data,-1,-1);Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);w.level=&arena;w.player.x=w.player.y=20;auto& f=w.fight;auto& c=w.character;c.character_class=6;c.stats.values[kLevel]=50;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;
 const auto* fire=f.skill_named("Fists of Fire");const auto* ice=f.skill_named("Blades of Ice");const auto* hurricane=f.skill_named("Hurricane");const auto* armageddon=f.skill_named("Armageddon");const auto* volcano=f.skill_named("Volcano");CHECK(fire&&ice&&hurricane&&armageddon&&volcano);
 for(bool owner:{false,true})for(int count:{1,2,3}){
  auto fighter=rule::simple_fighter(100,100,100000,0);rule::Swing swing;f.add_charges({Fight::Charge{fire->id,10,count,100000}},fighter,swing,owner);
  const int percent=std::clamp(f.calc(*fire,fire->calc[0],10),0,100);CHECK(percent>0&&swing.charge_conversion[0]==percent);rule::Rng rng{1};rule::Target target{10000,10000,0,50};const auto hit=rule::player_blow(fighter,target,50,rng,swing);CHECK(hit.hit&&hit.phys==100-percent);
  fighter=rule::simple_fighter(100,100,100000,0);swing={};f.add_charges({Fight::Charge{ice->id,10,count,100000}},fighter,swing,owner);const int length=rule::elem_length(data.skills,*ice,f.calc_env(),10);CHECK(swing.freeze_ticks==(count==3?length/ice->par[4]:0));
 }
 {auto fighter=rule::simple_fighter(100,100,100000,0);fighter.life_steal=100;rule::Swing swing;swing.charge_conversion[0]=100;rule::Rng rng{1};auto hit=rule::player_blow(fighter,{10000,10000,0,50},50,rng,swing);CHECK(hit.hit&&hit.phys==0&&hit.life==0&&hit.damage==100);}
 const auto make=[&](int id){auto m=make_monster(data,data.monsters.row("zombie1"),21,20,f.rng,0);m.id=id;m.hit_points=m.stats.hit_points=100000;return m;};
 f.monsters={make(10)};auto& m=f.monsters[0];CHECK(data.monsters.types[std::size_t(m.type)].diff[0].cold_effect<0);CHECK(data.freeze_divisor[0]>0&&data.freeze_divisor[1]>0&&data.freeze_divisor[2]>0);
 f.freeze_target(0,100,1000);CHECK(m.freeze_until==1000+4000/data.freeze_divisor[0]);const auto before=m.freeze_until;f.freeze_target(0,1,1000);CHECK(m.freeze_until==before);
 m.freeze_until=0;m.half_freeze=true;f.freeze_target(0,100,1000);CHECK(m.freeze_until==1000+2000/data.freeze_divisor[0]);m.half_freeze=false;m.freeze_until=0;m.boss=rule::Boss::unique;f.freeze_target(0,100,1000);CHECK(m.freeze_until==0&&m.chill_until>1000);
 m.boss=rule::Boss::none;m.chill_until=0;rule::Blow cold;cold.hit=true;cold.freeze_ticks=100;f.land(0,cold,true,2000);CHECK(m.freeze_until>2000);
 // A real third charge freezes, but a cold-immune victim does not receive freeze.
 auto fighter=rule::simple_fighter(100,100,100000,0);rule::Swing swing;f.add_charges({Fight::Charge{ice->id,10,3,100000}},fighter,swing,true);rule::Target immune{10000,10000,0,50};immune.res[4]=100;rule::Rng rng{1};CHECK(rule::player_blow(fighter,immune,50,rng,swing).freeze_ticks==0);
 // The AI must not finish an already started attack while frozen.
 m.freeze_until=10000;set_mode(data,m,"A1",3000);m.struck=false;m.stats.to_hit=100000;m.stats.a1_min=m.stats.a1_max=100;Foe target;target.x=target.y=20;target.fighter=rule::simple_fighter(0,0,0,0);std::vector<Foe> targets{target};std::vector<Missile> missiles;monster_update(data,arena,m,targets,f.rng,5000,0,{},missiles,f.monsters);CHECK(targets[0].damage==0&&m.mode=="NU");
 // Activation is relative to cast and stays live after a skipped modulo frame.
 f.monsters.clear();f.self_states.clear();f.start_state(*hurricane,1,1013);const auto ht=1013+unsigned(std::max(hurricane->par[3],1))*40;CHECK(f.state_of("hurricane")->next_tick==ht);f.buff_tick(ht-1);CHECK(f.state_of("hurricane")->next_tick==ht);f.buff_tick(ht+57);CHECK(f.state_of("hurricane")->next_tick==ht+57+unsigned(std::max(hurricane->par[3],1))*40);
 f.self_states.clear();f.start_state(*armageddon,1,2013);const auto at=2013+unsigned(std::max(armageddon->par[3],1))*40;CHECK(f.state_of("armageddon")->next_tick==at);f.buff_tick(at+57);CHECK(f.state_of("armageddon")->next_tick==at+57+unsigned(std::max(armageddon->par[3],1))*40&&f.pending.size()==1);f.pending.clear();
 // Original Volcano bounds are remaining frames, strictly inside Param3/4.
 const auto& vi=data.missiles.at(volcano->srvmissilea);CHECK(vi.srv_do==28&&vi.param4>vi.param3);const int interval=std::max(vi.param1>0?vi.param1:f.calc(*volcano,volcano->calc[3],1),1);
 const auto check_volcano=[&](int remaining,int expected){f.missiles.clear();f.pending.clear();Missile maker;maker.info=&vi;maker.friendly=true;maker.skill=volcano->id;maker.level=1;maker.born=1000;maker.dies=10000;maker.x=maker.y=20;f.missiles={maker};f.missile_tick(10000-unsigned(remaining)*40);CHECK(int(f.pending.size())==expected);};
 check_volcano(vi.param4,0);check_volcano(vi.param3,0);int inside=(vi.param4-1)/interval*interval;CHECK(inside>vi.param3);check_volcano(inside,1);
 std::puts("PASS skill completion: charge conversion/freeze on player and Shadow, freeze guards/AI, cast-relative storm cadence and original Volcano remaining-frame window");
}
static void incoming_defense_cases(GameData& data){
 // Per-channel native order and 8.8 precision, independent of world delivery.
 for(int channel=1;channel<5;++channel){rule::Fighter d;d.res={50,50,50,0};d.magic_res=50;d.absorb_pct[std::size_t(channel-1)]=80;d.absorb_flat[std::size_t(channel-1)]=10;rule::IncomingChannels raw{};raw[std::size_t(channel)]=100*256;
  const auto hit=rule::incoming_damage(d,raw);CHECK(hit.life_damage==20*256&&hit.life_heal==30*256); // percent cap40, then flat10
 }
 {rule::Fighter d;d.dr_pct=50;d.res[0]=75;rule::IncomingShield shield;shield.energy_pct=50;shield.energy_per16=32;shield.mana=100*256;const auto hit=rule::incoming_damage(d,{100*256,100*256,0,0,0},&shield);CHECK(shield.mana==0&&hit.life_damage==50*256);}
 {rule::Fighter d;rule::IncomingShield shield;shield.energy_pct=50;shield.energy_per16=12;shield.mana=256;const auto hit=rule::incoming_damage(d,{128,0,0,0,0},&shield);CHECK(hit.life_damage==64&&shield.mana==208);}
 {rule::Fighter d;d.dr_pct=50;d.dr_flat=20;rule::IncomingShield shield;shield.bone=60*256;const auto hit=rule::incoming_damage(d,{100*256,0,0,0,0},&shield);CHECK(shield.bone==0&&hit.life_damage==10*256);}
 {rule::Fighter d;d.res={25,75,50,0};rule::IncomingShield shield;shield.cyclone=120*256;const auto hit=rule::incoming_damage(d,{0,100*256,100*256,100*256,0},&shield);CHECK(shield.cyclone==0&&hit.life_damage==65*256);}
 Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);World w(&data,-1,-1);w.level=&arena;w.player.x=w.player.y=20;auto& f=w.fight;auto& c=w.character;c.character_class=1;c.stats.values[kLevel]=30;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;c.stats.values[kMana]=c.stats.values[kMaxMana]=256000;
 c.items={worn("cap",1,{{39,0,50},{142,0,20},{143,0,10}})};f.update_fighters(1000);CHECK(f.player_combat.absorb_pct[0]==20&&f.player_combat.absorb_flat[0]==10);
 f.receive_damage(0,-1,{0,100*256,0,0,0},1000);CHECK(c.stats.get(kLife)==970*256);f.receive_damage(0,-1,{0,100*256,0,0,0},1040);CHECK(c.stats.get(kLife)==960*256); // prior hit opens headroom
 const auto* bone=f.skill_named("Bone Armor");const auto* cyclone=f.skill_named("Cyclone Armor");const auto* energy=f.skill_named("Energy Shield");CHECK(bone&&cyclone&&energy);
 c.items={worn("cap",1,{{36,0,50}})};c.stats.values[kLife]=256000;f.update_fighters(2000);f.start_state(*bone,1,2000);const int bone_pool=f.absorb_pool;CHECK(bone_pool>0);
 f.receive_damage(0,-1,{std::int64_t(bone_pool+100)*256,0,0,0,0},2000);CHECK(c.stats.get(kLife)==950*256&&!f.state_of("bonearmor"));f.receive_damage(0,-1,{100*256,0,0,0,0},2040);CHECK(c.stats.get(kLife)==900*256);
 c.items.clear();c.stats.values[kLife]=256000;f.update_fighters(3000);f.start_state(*bone,1,3000);f.start_state(*cyclone,1,3000);CHECK(f.state_of("bonearmor")&&f.state_of("cyclonearmor"));
 const auto bp=f.state_of("bonearmor")->absorb,cp=f.state_of("cyclonearmor")->absorb;f.receive_damage(0,-1,{0,256,0,0,0},3000);CHECK(f.state_of("bonearmor")->absorb==bp&&f.state_of("cyclonearmor")->absorb==cp-256);f.receive_damage(0,-1,{256,0,0,0,0},3040);CHECK(f.state_of("bonearmor")->absorb==bp-256&&f.state_of("cyclonearmor")->absorb==cp-256);
 // Fractional pool use, poison exclusion, and recast replacing only its own pool.
 f.receive_damage(0,-1,{128,0,0,0,0},3080);CHECK(f.state_of("bonearmor")->absorb==bp-384);f.start_state(*bone,1,3120);CHECK(f.state_of("bonearmor")->absorb==bp&&f.state_of("cyclonearmor")->absorb==cp-256);
 f.self_states.clear();f.start_state(*energy,1,4000);c.stats.values[kLife]=256000;c.stats.values[kMana]=256000;f.update_fighters(4000);const int pct=std::clamp(f.calc(*energy,energy->calc[0],1),0,95),per16=std::max(f.calc(*energy,energy->calc[1],1),1);CHECK(pct>0);
 f.receive_damage(0,-1,{100*256,0,0,0,0},4000);CHECK(c.stats.get(kLife)==(1000-100+pct)*256&&c.stats.get(kMana)==256000-std::int64_t(pct)*256*per16/16);
 const auto mana=c.stats.get(kMana);Foe poison;poison.fighter=f.player_combat;f.bind_incoming(poison,0,-1,4040);rule::Taken toxic;toxic.hit=true;toxic.poison=256;toxic.poison_ticks=25;poison.take(toxic);CHECK(c.stats.get(kMana)==mana&&poison.poison==256);
 // Merc item absorption and authored Bone Armor use the same channel processor.
 f.self_states.clear();w.merc.emplace();w.merc->unit.x=w.merc->unit.y=20;w.merc->hit_points=1000;w.merc->mode="NU";f.merc_st.life=1000;f.merc_st.level=30;c.merc_items={worn("cap",1,{{39,0,50},{142,0,20},{143,0,10}})};
 f.receive_damage(1,-1,{0,100*256,0,0,0},5000);f.receive_damage(1,-1,{0,100*256,0,0,0},5040);CHECK(w.merc->hit_points==960);
 c.merc_items={worn("cap",1,{{36,0,50}})};w.merc->hit_points=1000;f.add_merc_buff(*bone,1,5080);const auto mp=f.merc_buffs.back().absorb;CHECK(mp>0);f.receive_damage(1,-1,{mp+100*256,0,0,0,0},5080);CHECK(w.merc->hit_points==950&&f.merc_buffs.empty());
 // Sub-point damage is retained for monster-based receivers rather than rounded away.
 c.merc_items.clear();w.merc->hit_points=1000;w.merc->hp_fraction=0;f.receive_damage(1,-1,{128,0,0,0,0},5120);CHECK(w.merc->hit_points==999&&w.merc->hp_fraction==128);f.receive_damage(1,-1,{128,0,0,0,0},5160);CHECK(w.merc->hit_points==999&&w.merc->hp_fraction==0);
 Fight::Pet pet;pet.monster=make_monster(data,data.monsters.row("zombie1"),21,20,f.rng,0);pet.monster.id=100;pet.monster.hit_points=pet.monster.stats.hit_points=1000;pet.monster.boss_res={};pet.where=&arena;pet.revived=true;pet.skill=95;pet.wake=100000;f.pets={pet};f.receive_damage(2,100,{128,0,0,0,0},5200);CHECK(f.pets[0].monster.hit_points==999&&f.pets[0].monster.hp_fraction==128);
 // A lethal hit is settled before another hit can heal/revive the same receiver.
 c.items={worn("cap",1,{{143,0,100}})};f.update_fighters(6000);c.stats.values[kLife]=20*256;f.receive_damage(0,-1,{100*256,0,0,0,0},6000);CHECK(f.dead()&&c.stats.get(kLife)==0);f.receive_damage(0,-1,{0,100*256,0,0,0},6040);CHECK(c.stats.get(kLife)==0);
 w.merc->hit_points=20;w.merc->mode="NU";f.receive_damage(1,-1,{100*256,0,0,0,0},6080);CHECK(w.merc->mode=="DT"&&w.merc->hit_points==0);f.receive_damage(1,-1,{0,100*256,0,0,0},6120);CHECK(w.merc->hit_points==0);
 f.pets[0].monster.hit_points=20;f.receive_damage(2,100,{100*256,0,0,0,0},6160);CHECK(f.pets[0].monster.hit_points==0&&f.pets[0].monster.mode=="DT");
 // Actual two-monster melee delivery consumes Bone Armor before resistance, exactly once.
 World battle(&data,-1,-1);battle.level=&arena;battle.player.x=battle.player.y=20;auto& b=battle.fight;auto& hero=battle.character;hero.character_class=2;hero.stats.values[kLevel]=30;hero.stats.values[kLife]=hero.stats.values[kMaxLife]=256000;hero.items={worn("cap",1,{{36,0,50}})};b.update_fighters(1000);b.start_state(*bone,1,1000);const int pool=b.absorb_pool;unsigned now=0;
 for(int k=0;k<2;++k){auto enemy=make_monster(data,data.monsters.row("zombie1"),20.5f,20.1f-.2f*k,b.rng,0);enemy.id=10+k;enemy.hit_points=enemy.stats.hit_points=1000;enemy.stats.level=30;enemy.stats.to_hit=100000;enemy.stats.a1_min=enemy.stats.a1_max=80;enemy.stats.elements={};set_mode(data,enemy,"A1",1000);enemy.struck=false;now=1000+data.npc_timing(enemy.npc,"A1").action_ms();b.monsters.push_back(enemy);}
 b.rng=rule::Rng{1};b.world(true,now,0,{});CHECK(hero.stats.get(kLife)==256000-std::int64_t(160-pool)*128&&b.absorb_pool==0);
 // Fatal first hit prevents a second attacker's Life Tap from feeding on a corpse.
 b.self_states.clear();b.monsters.clear();b.pmode=-1;hero.items.clear();hero.stats.values[kLife]=10*256;b.update_fighters(5000);b.curses[0]={82,1,100000};
 for(int k=0;k<2;++k){auto enemy=make_monster(data,data.monsters.row("zombie1"),20.5f,20.1f-.2f*k,b.rng,0);enemy.id=20+k;enemy.hit_points=50;enemy.stats.hit_points=200;enemy.stats.level=30;enemy.stats.to_hit=100000;enemy.stats.a1_min=enemy.stats.a1_max=40;enemy.stats.elements={};set_mode(data,enemy,"A1",5000);enemy.struck=false;now=5000+data.npc_timing(enemy.npc,"A1").action_ms();b.monsters.push_back(enemy);}
 b.rng=rule::Rng{1};b.world(true,now,0,{});CHECK(b.dead()&&hero.stats.get(kLife)==0&&b.monsters[0].hit_points==70&&b.monsters[1].hit_points==50);
 // Hostile arrow delivery uses the same pre-resistance Energy Shield order.
 b.pmode=-1;b.monsters.clear();b.self_states.clear();b.curses={};hero.stats.values[kLife]=256000;hero.stats.values[kMana]=hero.stats.values[kMaxMana]=256000;hero.items={worn("cap",1,{{39,0,50},{142,0,20},{143,0,10}})};b.update_fighters(10000);b.start_state(*energy,1,10000);
 rule::IncomingShield expected_shield;expected_shield.energy_pct=b.calc(*energy,energy->calc[0],1);expected_shield.energy_per16=b.calc(*energy,energy->calc[1],1);expected_shield.mana=256000;const auto expected=rule::incoming_damage(b.player_combat,{0,100*256,0,0,0},&expected_shield);
 Missile arrow;arrow.info=&data.missiles.at("skbowarrow1");arrow.x=arrow.y=20;arrow.born=0;arrow.dies=100000;arrow.src.level=30;arrow.src.to_hit=100000;arrow.src.a2_min=arrow.src.a2_max=0;arrow.src.elements[0]={0,100,100,100,0,"A2"};b.missiles={arrow};b.rng=rule::Rng{1};b.world(true,10000,0,{});CHECK(hero.stats.get(kLife)==256000-expected.life_damage&&hero.stats.get(kMana)==expected_shield.mana);
 // A fire zone is damage, not an attack: no armor wear/OnStruck trigger per frame.
 const auto* nova=b.skill_named("Nova");CHECK(nova);b.self_states.clear();hero.stats.values[kLife]=256000;hero.items[0].props.push_back({201,(nova->id<<6)|3,100});b.update_fighters(11000);
 Missile fire;fire.info=&data.missiles.at("firewall");CHECK(fire.info->srv_do==5);fire.row=rule::MissileDamage{.etype=0,.elo=100*256,.ehi=100*256};fire.x=fire.y=20;fire.born=0;fire.dies=100000;b.missiles={fire};b.world(true,11000,0,{});CHECK(hero.stats.get(kLife)==970*256&&b.pending.empty());
 // Hostile aura damage can be absorbed/healed without inventing attack events.
 b.missiles.clear();hero.stats.values[kLife]=500*256;hero.items={worn("cap",1,{{143,0,100},{201,(nova->id<<6)|3,100}})};b.update_fighters(12000);const auto* holy_fire=b.skill_named("Holy Fire");CHECK(holy_fire);auto emitter=make_monster(data,data.monsters.row("zombie1"),20.5f,20,b.rng,0);emitter.id=30;emitter.hit_points=emitter.stats.hit_points=1000;emitter.aura=holy_fire->id;emitter.aura_lvl=1;emitter.aura_next=0;emitter.next_act=100000;b.monsters={emitter};b.world(true,12000,0,{});CHECK(hero.stats.get(kLife)>500*256&&b.pending.empty());
 std::puts("PASS incoming per-hit defenses: 8.8 channels, absorb/heal caps, Energy Shield mana, independent Bone/Cyclone pools, merc armor, fractional receivers and fatal-hit ordering");
}
static void inventory_continuation_cases(GameData& data){
 const auto stored=[](const char* code,int x=0,int y=0){auto i=worn(code,0,{});i.location=item_location::kStored;i.panel=item_panel::kInventory;i.column=x;i.row=y;return i;};
 rule::Store store;store.npc_id="akara";store.tab=0;store.perm={"tsc","aqv"};auto scroll=stored("tsc");scroll.quantity=1;store.tabs[0]={scroll};
 Stats stats;stats.values[kLevel]=30;stats.values[kGold]=100000;stats.values[kGoldBank]=100000;
 auto book=stored("tbk");book.quantity=19;std::vector<Item> inv{book};for(int y=0;y<4;++y)for(int x=0;x<10;++x)if(x!=0||y>1)inv.push_back(stored("hp1",x,y));
 const auto price=rule::item_price(data.rules,scroll,store.npc_id,false,store.header);const auto gold=stats.get(kGold);const auto count=inv.size();CHECK(rule::store_buy(data.rules,store,0,inv,stats));CHECK(inv.size()==count&&inv[0].quantity==20&&stats.get(kGold)==gold-price);
 const auto paid=stats.get(kGold);CHECK(!rule::store_buy(data.rules,store,0,inv,stats)&&stats.get(kGold)==paid&&inv[0].quantity==20&&store.tabs[0].size()==1);
 auto arrows=worn("aqv",5,{});const int cap=data.rules.item_base.at("aqv").max_stack;arrows.quantity=cap-10;inv.push_back(arrows);auto stock=stored("aqv");stock.quantity=10;store.tabs[0]={stock};CHECK(rule::store_buy(data.rules,store,0,inv,stats));CHECK(inv.back().quantity==cap&&inv.size()==count+1);
 inv.back().quantity=cap-5;const auto before=stats.get(kGold);CHECK(!rule::store_buy(data.rules,store,0,inv,stats)&&inv.back().quantity==cap-5&&stats.get(kGold)==before);
 auto normal=stored("tkf");normal.quantity=10;auto other=normal;CHECK(rule::stack_compatible(data.rules,normal,other));other.quality=4;CHECK(!rule::stack_compatible(data.rules,normal,other));other=normal;other.ethereal=true;CHECK(!rule::stack_compatible(data.rules,normal,other));other=normal;other.props={{159,0,1}};CHECK(!rule::stack_compatible(data.rules,normal,other));
 std::vector<Item> stacks{normal};stacks[0].quantity=data.rules.item_base.at("tkf").max_stack-5;std::optional<Item> held=normal;CHECK(rule::put_in_grid(data.rules,stacks,held,item_panel::kInventory,10,4,0,0)&&held&&held->quantity==5&&stacks[0].quantity==data.rules.item_base.at("tkf").max_stack);
 stacks={book};held=scroll;CHECK(rule::put_in_grid(data.rules,stacks,held,item_panel::kInventory,10,4,0,0)&&!held&&stacks[0].quantity==20);
 // Pickup prioritizes the active quiver and preserves the remaining quantity.
 std::vector<Item> picked{arrows};picked[0].quantity=cap-5;auto loose=stock;CHECK(rule::pick_up(data.rules,picked,loose,10,4,4)==rule::Pickup::kGone&&picked[0].quantity==cap&&picked.size()==2&&picked[1].quantity==5);
 auto empty_book=book;empty_book.quantity=0;store.tabs[0]={empty_book};std::vector<Item> empty_inventory;CHECK(rule::store_buy(data.rules,store,0,empty_inventory,stats)&&empty_inventory.size()==1&&empty_inventory[0].quantity==0);
 // An empty rare right-hand stack must not hide live ammunition in the left hand.
 World w(&data,-1,-1);auto& f=w.fight;auto& c=w.character;c.stats.values[kLevel]=30;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;auto empty=worn("tkf",4,{});empty.quality=6;empty.quantity=0;empty.flags|=0x100;auto live=worn("tax",5,{});live.quantity=3;c.items={empty,live};CHECK(f.ammo_index()==1);
 // Removing a 16-slot belt keeps the bottom row, moves extras, and drops overflow if full.
 Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);w.level=&arena;w.player.x=w.player.y=20;
 auto belt=worn("hbl",body_location::kBelt,{});belt.id=100;c.items={belt};CHECK(data.belts[std::size_t(belt_index(data,c.items))].boxes==16);
 for(int slot:{0,4,15}){auto potion=stored("hp1");potion.id=200+slot;potion.location=item_location::kBelt;potion.column=slot;c.items.push_back(potion);}
 w.apply(cmd::ToCursor{100},1000);CHECK(w.held&&w.held->id==100&&std::ranges::count(c.items,item_location::kBelt,&Item::location)==1&&std::ranges::count(c.items,item_location::kStored,&Item::location)==2&&w.loot.ground.empty());
 w.held.reset();c.items={belt};for(int y=0;y<4;++y)for(int x=0;x<10;++x)c.items.push_back(stored("hp1",x,y));auto potion=stored("hp2");potion.id=777;potion.location=item_location::kBelt;potion.column=15;c.items.push_back(potion);w.apply(cmd::ToCursor{100},1040);CHECK(w.held&&w.held->id==100&&w.loot.ground.size()==1&&w.loot.ground[0].item.id==777&&w.loot.ground[0].item.code=="hp2");
 // Replacing the equipped belt directly also trims only rows beyond the new capacity.
 w.held.reset();c.items={belt};for(int slot:{0,4,15}){auto potion=stored("hp1");potion.location=item_location::kBelt;potion.column=slot;c.items.push_back(potion);}w.held=worn("lbl",body_location::kBelt,{});c.stats.values[kStr]=c.stats.values[kDex]=200;w.apply(cmd::ToBody{body_location::kBelt},1080);CHECK(w.held&&w.held->id==100);const int boxes=data.belts[std::size_t(belt_index(data,c.items))].boxes;CHECK(boxes==8&&std::ranges::count(c.items,item_location::kBelt,&Item::location)==2&&std::ranges::count(c.items,item_location::kStored,&Item::location)==1);
 // Hit/kill properties remain attached to the projectile after its weapon disappears.
 w.held.reset();c.items.clear();c.character_class=4;c.stats.values[kStr]=c.stats.values[kDex]=100;c.stats.values[kMana]=0;
 const auto* amp=f.skill_named("Amplify Damage");const auto* weak=f.skill_named("Weaken");const auto* nova=f.skill_named("Nova");CHECK(amp&&weak&&nova);
 auto right=worn("tkf",4,{{198,(amp->id<<6)|1,100},{196,(nova->id<<6)|3,100}});right.quantity=1;auto left=worn("tax",5,{{198,(weak->id<<6)|1,100}});left.quantity=1;c.items={right,left};f.update_fighters(2000);
 f.cast_x=21;f.cast_y=20;const auto* dual=f.skill_named("Double Throw");CHECK(dual);f.spot(*dual,2000,{20,20,5,-1,21,20});CHECK(f.pending.size()==2&&c.items.empty());auto shot=f.pending[0];f.pending.clear();f.update_fighters(2000);CHECK(f.item_procs[0].empty());
 auto enemy=make_monster(data,data.monsters.row("zombie1"),21,20,f.rng,0);enemy.id=10;enemy.hit_points=enemy.stats.hit_points=10000;enemy.stats.armor_class=0;enemy.stats.level=30;enemy.next_act=100000;f.monsters={enemy};
 for(int attempt=0;attempt<20&&f.monsters[0].curse.skill<0;++attempt){auto copy=shot;copy.x=21;copy.y=20;copy.velocity_x=copy.velocity_y=0;copy.dies=100000;copy.struck=std::make_shared<std::vector<int>>();f.missiles={copy};f.world(true,2100+attempt*40,0,{});}CHECK(f.monsters[0].curse.skill==amp->id&&c.stats.get(kMana)==0&&f.projectile_procs==nullptr);
 f.monsters[0].hit_points=1;f.monsters[0].curse={};bool killed=false;
 for(int attempt=0;attempt<20&&!killed;++attempt){auto copy=shot;copy.x=21;copy.y=20;copy.velocity_x=copy.velocity_y=0;copy.dies=100000;copy.struck=std::make_shared<std::vector<int>>();f.missiles={copy};f.world(true,4000+attempt*40,0,{});killed=!f.monsters[0].alive();}CHECK(killed&&std::ranges::any_of(f.pending,[&](const auto& missile){return missile.skill==nova->id;})&&f.projectile_procs==nullptr);
 std::puts("PASS inventory continuation: atomic full-inventory stack/book buying, manual compatible merges, empty-hand fallback, belt shrink inventory and ground overflow");
}
static void broad_mechanics_cases(GameData& data){
 CHECK(rule::affix_level(1,24,3)==27&&rule::affix_level(98,24,3)==99&&rule::affix_level(50,20)==40);
 CHECK(data.rules.item_base.at("ci0").magic_lvl>0);
 rule::Fighter stunner=rule::simple_fighter(10,10,100000);stunner.stun_len=40;rule::Target victim;victim.level=30;victim.max_hp=victim.hit_points=10000;rule::Rng rng{1};bool landed=false;
 for(int n=0;n<30&&!landed;++n){const auto hit=rule::player_blow(stunner,victim,30,rng,{.srcdam=64,.stun_ticks=10,.ranged=true});if(hit.hit){CHECK(hit.stun_ticks==30);landed=true;}}CHECK(landed);
 Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);World w(&data,-1,-1);w.level=&arena;w.player.x=w.player.y=20;auto& f=w.fight;auto& c=w.character;c.character_class=4;c.stats.values[kLevel]=30;c.stats.values[kStr]=c.stats.values[kDex]=100;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;c.stats.values[kMana]=256000;
 auto right=worn("tkf",4,{{kFireMinDamage,0,13},{kFireMaxDamage,0,13}});right.quantity=2;auto left=worn("tax",5,{{kColdMinDamage,0,9},{kColdMaxDamage,0,9}});left.quantity=1;c.items={right,left};f.update_fighters(1000);
 const auto& base=data.rules.item_base.at("tkf");CHECK(base.throw_min>0&&base.throw_max>0);
 const auto throwing=f.player_fighter(nullptr,nullptr,nullptr,nullptr,4,true);CHECK(throwing.phys_lo==base.throw_min*256&&throwing.elem[0].first==13&&throwing.elem[2].first==0);
 const auto* dual=f.skill_named("Double Throw");CHECK(dual);f.cast_x=25;f.cast_y=20;f.spot(*dual,1000,{20,20,5,-1,25,20});CHECK(f.pending.size()==2&&f.pending[0].thrown&&f.pending[1].thrown&&f.pending[0].thrown_hand==4&&f.pending[1].thrown_hand==5);
 CHECK(f.pending[0].thrown->elem[0].first==13&&f.pending[0].thrown->elem[2].first==0&&f.pending[1].thrown->elem[0].first==0&&f.pending[1].thrown->elem[2].first==9);
 CHECK(c.items.size()==1&&c.items[0].quantity==1);const auto off_shot=f.pending[1];const auto saved=f.pending[1].thrown;CHECK(saved->phys_hi>0);f.pending.clear();f.spot(*dual,1040,{20,20,5,-1,25,20});CHECK(f.pending.size()==1&&c.items.empty()&&saved->elem[2].first==9);f.pending.clear();f.spot(*dual,1080,{20,20,5,-1,25,20});CHECK(f.pending.empty());
 // A shot from an exhausted/removed off-hand still deals its captured weapon damage.
 auto enemy=make_monster(data,data.monsters.row("zombie1"),21,20,f.rng,0);enemy.id=10;enemy.hit_points=enemy.stats.hit_points=10000;enemy.stats.armor_class=0;enemy.stats.level=30;enemy.next_act=100000;f.monsters={enemy};
 for(int attempt=0;attempt<20&&f.monsters[0].hit_points==10000;++attempt){auto shot=off_shot;shot.x=21;shot.y=20;shot.velocity_x=shot.velocity_y=0;shot.dies=100000;shot.struck=std::make_shared<std::vector<int>>();f.missiles={shot};f.world(true,1200+attempt*40,0,{});}CHECK(f.monsters[0].hit_points<10000);f.monsters.clear();f.missiles.clear();
 // Holy Bolt contacts owned allies, honors its healing flag and never resurrects.
 c.character_class=3;c.items.clear();f.update_fighters(2000);w.merc.emplace();w.merc->unit.x=21;w.merc->unit.y=20;w.merc->mode="NU";w.merc->hit_points=50;f.merc_st.life=1000;f.merc_st.level=30;
 const auto* holy=f.skill_named("Holy Bolt");CHECK(holy&&holy->target_pet);const auto& info=data.missiles.at(holy->srvmissile);CHECK(info.hit_func==7&&info.hit_par1);
 auto bolt=[&](float x,float y,unsigned now,const GameData::MissileInfo& row){Missile shot;shot.info=&row;shot.friendly=true;shot.skill=holy->id;shot.level=5;shot.x=x;shot.y=y;shot.born=0;shot.dies=100000;f.missiles={shot};f.world(true,now,0,{});};
 bolt(21,20,2000,info);CHECK(w.merc->hit_points>50&&f.missiles.empty());w.merc->hit_points=999;bolt(21,20,2040,info);CHECK(w.merc->hit_points==1000);
 w.merc->hit_points=0;w.merc->mode="DT";bolt(21,20,2080,info);CHECK(w.merc->hit_points==0);w.merc.reset();
 Fight::Pet pet;pet.monster=make_monster(data,data.monsters.row("skeleton1"),21,20,f.rng,0);pet.monster.id=100;pet.monster.hit_points=50;pet.monster.stats.hit_points=1000;pet.where=&arena;pet.wake=100000;pet.revived=true;pet.skill=95;f.pets={pet};bolt(21,20,2120,info);CHECK(f.pets[0].monster.hit_points>50);
 auto no_heal=info;no_heal.hit_par1=0;const int hp=f.pets[0].monster.hit_points;bolt(21,20,2160,no_heal);CHECK(f.pets[0].monster.hit_points==hp);
 std::puts("PASS broad mechanics: original magic-level affixes, item stun SrcDam, throwing hand snapshots and quantity exhaustion, Holy Bolt merc/pet healing, caps, dead and nonhealing bolt exclusions");
}
static void friendly_recovery_cases(GameData& data){
 const auto* prayer=data.skills.get(data.skills.by_name.at("Prayer"));const auto* cleansing=data.skills.get(data.skills.by_name.at("Cleansing"));const auto* amp=data.skills.get(data.skills.by_name.at("Amplify Damage"));const auto* attract=data.skills.get(data.skills.by_name.at("Attract"));const auto* shout=data.skills.get(data.skills.by_name.at("Shout"));
 CHECK(prayer&&cleansing&&amp&&attract&&shout);CHECK(data.states.at(amp->auratarget).curse&&data.states.at(amp->auratarget).curable&&!data.states.at(shout->auratarget).curse);
 Level arena,elsewhere;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);
 World w(&data,-1,-1);w.level=&arena;w.player.x=w.player.y=20;auto& f=w.fight;auto& c=w.character;c.character_class=3;c.stats.values[kLevel]=30;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;c.stats.values[kMana]=c.stats.values[kMaxMana]=25600;
 const auto& ids=data.skills.class_ids[3];for(const auto* skill:{prayer,cleansing}){const auto at=std::ranges::find(ids,skill->id);CHECK(at!=ids.end());c.stats.skills[std::size_t(at-ids.begin())]=10;}
 w.merc.emplace();w.merc->unit.x=w.merc->unit.y=20;w.merc->hit_points=1000;w.merc->mode="NU";f.merc_st.life=1000;f.merc_st.level=30;
 Fight::Pet pet;pet.monster=make_monster(data,data.monsters.row("skeleton1"),20,20,f.rng,0);pet.monster.id=100;pet.monster.hit_points=50;pet.monster.stats.hit_points=100;pet.where=&arena;pet.revived=true;pet.skill=95;CHECK(!data.monsters.types[std::size_t(pet.monster.type)].no_aura);
 f.pets={pet};f.aura=prayer->id;f.refresh_item_effects();int heal=0;for(std::size_t k=0;k<prayer->aurastat.size();++k)if(prayer->aurastat[k]==6)heal=rule::eval_calc(data.skills,prayer->aura_calc[k],f.calc_env(),prayer->id,10)>>8;CHECK(heal>0&&heal<50);
 const auto mana=c.stats.values[kMana];f.aura_pulse(1000);CHECK(f.pets[0].monster.hit_points==50+heal&&c.stats.values[kMana]<mana&&f.prayer_no_mana_regen);
 f.pets[0].monster.hit_points=99;f.aura_pulse(5000);CHECK(f.pets[0].monster.hit_points==100);const auto paid=c.stats.values[kMana];f.aura_pulse(9000);CHECK(c.stats.values[kMana]==paid&&!f.prayer_no_mana_regen);
 // Range, another level, a dead receiver, and original MonStats noaura all exclude healing.
 auto ineligible=pet;ineligible.monster.id=101;ineligible.monster.unit.x=1000;f.pets.push_back(ineligible);ineligible=pet;ineligible.monster.id=102;ineligible.where=&elsewhere;f.pets.push_back(ineligible);ineligible=pet;ineligible.monster.id=103;ineligible.monster.hit_points=0;ineligible.monster.mode="DT";f.pets.push_back(ineligible);
 const auto no_aura=std::ranges::find_if(data.monsters.types,[](const auto& type){return type.no_aura;});CHECK(no_aura!=data.monsters.types.end());ineligible=pet;ineligible.monster.id=104;ineligible.monster.type=int(no_aura-data.monsters.types.begin());f.pets.push_back(ineligible);
 f.aura_pulse(13000);CHECK(f.pets[1].monster.hit_points==50&&f.pets[2].monster.hit_points==50&&f.pets[3].monster.hit_points==0&&f.pets[4].monster.hit_points==50&&c.stats.values[kMana]==paid);
 f.pets.resize(1);f.pets[0]=pet;f.pets[0].monster.hit_points=100;c.stats.values[kMana]=0;f.pets[0].monster.hit_points=50;f.aura_pulse(17000);CHECK(f.pets[0].monster.hit_points==50);c.items={worn("cap",1,{{151,prayer->id,10}})};f.aura=0;f.refresh_item_effects();f.aura_pulse(21000);CHECK(f.pets[0].monster.hit_points==50+heal&&c.stats.values[kMana]==0);
 // Cleansing shortens existing poison/curable curses each pulse; equipment PLR is separate.
 c.items.clear();c.stats.values[kMana]=25600;f.aura=cleansing->id;f.refresh_item_effects();CHECK(f.aura_stats(false)[110]==0&&f.aura_stats(true)[110]==0);
 int remaining=-1;for(std::size_t k=0;k<cleansing->aurastat.size();++k)if(cleansing->aurastat[k]==110)remaining=rule::eval_calc(data.skills,cleansing->aura_calc[k],f.calc_env(),cleansing->id,10);CHECK(remaining>0&&remaining<100);
 const unsigned now=30000,until=40000,expected=now+(until-now)*unsigned(remaining)/100;
 f.regen={{-1,0,until,true},{1,0,until,false}};f.curses[0]={amp->id,1,until};f.marks[0]={shout->id,1,until};f.curses[1]={amp->id,1,until};w.merc->poison_until=until;auto& m=f.pets[0].monster;m.poison_until=until;m.curse={amp->id,1,until};m.cry={shout->id,1,until};
 f.aura_pulse(now);CHECK(f.regen[0].until==expected&&f.regen[1].until==until&&f.curses[0].until==expected&&f.marks[0].until==until&&f.curses[1].until==expected&&w.merc->poison_until==expected&&m.poison_until==expected&&m.curse.until==expected&&m.cry.until==until);
 // Repeated pulse uses remaining time, not original lifetime; no pulse before its due time.
 const unsigned before=f.regen[0].until;f.aura_pulse(now+1);CHECK(f.regen[0].until==before);const unsigned next=f.aura_sources.front().next;const unsigned again=next+(expected-next)*unsigned(remaining)/100;CHECK(expected>next);f.aura_pulse(next);CHECK(f.regen[0].until==again);
 f.curses[0]={attract->id,1,next+10000};const bool curable=data.states.at(attract->auratarget).curse&&data.states.at(attract->auratarget).curable;const unsigned pulse=f.aura_sources.front().next,old=f.curses[0].until;f.aura_pulse(pulse);CHECK(f.curses[0].until==(curable?pulse+(old-pulse)*unsigned(remaining)/100:old));
 // Duplicate equal sources do not apply the duration reduction twice.
 f.aura=0;c.items={worn("cap",1,{{151,cleansing->id,10}})};c.merc_items={worn("cap",1,{{151,cleansing->id,10}})};f.refresh_item_effects();CHECK(f.aura_sources.size()==2);f.curses[0]={amp->id,1,60000};f.aura_pulse(50000);CHECK(f.curses[0].until==50000+10000*unsigned(remaining)/100);
 // A weaker item source cannot shorten again; real equipment PLR remains independent.
 c.items={worn("cap",1,{{151,cleansing->id,5},{110,0,20}})};f.refresh_item_effects();f.update_fighters(70000);CHECK(f.player_combat.plr==20);f.curses[0]={amp->id,1,80000};f.aura_pulse(70000);CHECK(f.curses[0].until==70000+10000*unsigned(remaining)/100);
 // Out-of-range/off-level/noaura/dead receivers keep existing durations unchanged.
 for(int variant=0;variant<4;++variant){auto excluded=pet;excluded.monster.id=200+variant;excluded.monster.poison_until=100000;excluded.monster.curse={amp->id,1,100000};
  if(variant==0)excluded.monster.unit.x=1000;else if(variant==1)excluded.where=&elsewhere;else if(variant==2)excluded.monster.type=int(no_aura-data.monsters.types.begin());else{excluded.monster.hit_points=0;excluded.monster.mode="DT";}
  f.pets.push_back(excluded);
 }
 f.aura_pulse(90000);for(std::size_t i=1;i<f.pets.size();++i)CHECK(f.pets[i].monster.poison_until==100000&&f.pets[i].monster.curse.until==100000);
 // Neither a dead hero nor a dead merc continues to project an equipped aura.
 c.stats.values[kLife]=0;w.merc->hit_points=0;w.merc->mode="DT";f.curses[0]={amp->id,1,120000};f.pets[0].monster.curse={amp->id,1,120000};f.pets[0].monster.poison_until=120000;f.aura_pulse(110000);CHECK(f.curses[0].until==120000&&f.pets[0].monster.curse.until==120000&&f.pets[0].monster.poison_until==120000);
 // A living merc cannot resurrect a zero-HP hero with a late Prayer pulse.
 c.items.clear();f.aura=0;c.merc_items={worn("cap",1,{{151,prayer->id,10}})};w.merc->hit_points=1000;w.merc->mode="NU";f.refresh_item_effects();f.aura_pulse(130000);CHECK(c.stats.get(kLife)==0);
 // A dead stronger source must not suppress a living weaker source on a summon.
 c.items={worn("cap",1,{{151,prayer->id,20}})};f.pets.resize(1);f.pets[0]=pet;f.refresh_item_effects();f.aura_pulse(140000);CHECK(f.pets[0].monster.hit_points>50&&c.stats.get(kLife)==0);
 // A dead-mode merc with stale positive HP is not healed either.
 c.stats.values[kLife]=256000;w.merc->hit_points=50;w.merc->mode="DT";c.merc_items.clear();c.items={worn("cap",1,{{151,prayer->id,10}})};f.refresh_item_effects();f.aura_pulse(150000);CHECK(w.merc->hit_points==50);
 std::puts("PASS real Prayer healing on summons, mana/range/death/noaura and free item source; Cleansing duration pulses on hero/merc/pets, curable flags, no PLR bonus and no duplicate stacking");
}
// Native SUnitDmg subtracts flat reduction before percentage resistance for each channel.
static void flat_reduction_cases(){
 for(bool missile:{false,true})for(int resist:{-100,-50,0,50,75,100,110})for(int flat:{0,20,100,120}){
  rule::MonStats source;source.level=30;source.to_hit=100000;source.a1_min=source.a1_max=source.a2_min=source.a2_max=100;
  rule::Fighter defender;defender.dr_pct=resist;defender.dr_flat=flat;defender.mdr=flat;defender.magic_res=resist;defender.res={resist,resist,resist,resist};
  for(int channel=-1;channel<5;++channel){
   auto attack=source;
   if(channel>=0){attack.a1_min=attack.a1_max=attack.a2_min=attack.a2_max=0;attack.elements[0]={channel,100,100,100,10,missile?"A2":"A1"};}
   rule::Rng rng{1};bool hit=false;
   for(int n=0;n<20&&!hit;++n){const auto taken=rule::monster_blow(defender,30,rule::Motion::standing,attack,missile,rng,missile);if(!taken.hit)continue;hit=true;
    const int expected=rule::resisted(std::max(100-flat,0),resist);
    if(channel==3){CHECK(taken.damage==0&&taken.poison==rule::resisted(1000,resist)&&taken.poison_ticks==20);}
    else{CHECK(taken.damage==expected);CHECK(taken.physical==(channel<0?expected:0));CHECK(taken.elemental==((channel>=0&&channel<=2)?expected:0));}
    CHECK(taken.raw_physical==(channel<0?100:0));
    if(channel==2)CHECK(taken.chill_ticks==rule::resisted(10,resist));
   }CHECK(hit);
  }
 }
 // Distinct components must keep their bookkeeping for shields, Thorns and Life Tap.
 rule::MonStats mixed;mixed.level=30;mixed.to_hit=100000;mixed.a1_min=mixed.a1_max=100;
 mixed.elements[0]={0,100,100,100,0,"A1"};mixed.elements[1]={1,100,100,100,0,"A1"};mixed.elements[2]={2,100,100,100,10,"A1"};
 rule::Fighter defender;defender.dr_pct=50;defender.dr_flat=20;defender.mdr=20;defender.res={50,50,50,0};rule::Rng rng{1};bool hit=false;
 for(int n=0;n<20&&!hit;++n){const auto taken=rule::monster_blow(defender,30,rule::Motion::standing,mixed,false,rng);if(taken.hit){hit=true;CHECK(taken.raw_physical==100&&taken.physical==40&&taken.elemental==120&&taken.damage==160);}}CHECK(hit);
 std::puts("PASS flat reduction before resistance: physical/fire/lightning/cold/magic, melee/missiles, vulnerabilities/immunities, poison exclusion and mixed bookkeeping");
}
static void flat_reduction_world_cases(GameData& data){
 for(int receiver=0;receiver<2;++receiver){
  Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);
  World w(&data,-1,-1);w.level=&arena;w.player.x=w.player.y=receiver==0?20.1f:.1f;
  auto& f=w.fight;auto& c=w.character;c.character_class=1;c.stats.values[kLevel]=30;c.stats.values[kMaxLife]=c.stats.values[kLife]=256000;
  const auto armor=worn("cap",1,{{34,0,20},{35,0,20},{36,0,50},{39,0,50},{41,0,50},{43,0,50}});
  if(receiver==0)c.items={armor};
  else{w.merc.emplace();w.merc->unit.x=w.merc->unit.y=20.1f;w.merc->hit_points=1000;w.merc->mode="NU";f.merc_st.life=1000;f.merc_st.level=30;c.merc_items={armor};}
  f.update_fighters(1000);const auto defender=receiver==0?f.player_combat:f.merc_fighter();CHECK(defender.dr_flat==20&&defender.dr_pct==50&&defender.mdr==20&&defender.res[0]==50&&defender.res[1]==50&&defender.res[2]==50);
  auto enemy=make_monster(data,data.monsters.row("zombie1"),20.5f,20.1f,f.rng,0);enemy.id=10;enemy.hit_points=enemy.stats.hit_points=1000;enemy.stats.level=30;enemy.stats.to_hit=100000;enemy.stats.a1_min=enemy.stats.a1_max=100;enemy.stats.elements={};
  for(int channel=0;channel<3;++channel)enemy.stats.elements[channel]={channel,100,100,100,10,"A1"};
  set_mode(data,enemy,"A1",1000);enemy.struck=false;const unsigned now=1000+data.npc_timing(enemy.npc,"A1").action_ms();f.monsters={enemy};f.rng=rule::Rng{1};f.update_fighters(now);f.world(true,now,0,{});
  CHECK((receiver==0?int(c.stats.get(kLife)/256):w.merc->hit_points)==840);
  f.monsters[0].mode="NU";f.monsters[0].next_act=100000;
  Missile shot;shot.info=&data.missiles.at("skbowarrow1");shot.x=shot.y=20.1f;shot.born=0;shot.dies=10000;shot.owner=10;shot.src=enemy.stats;shot.src.a2_min=shot.src.a2_max=100;for(auto& element:shot.src.elements)element.mode="A2";
  f.missiles={shot};f.rng=rule::Rng{1};f.world(true,now+40,0,{});CHECK((receiver==0?int(c.stats.get(kLife)/256):w.merc->hit_points)==680);
 }
 std::puts("PASS real gear flat reductions on hero/merc: mixed melee and hostile missile damage");
}
static void magic_lifetap_cases(GameData& data){
 // The receiver's magic channel is separate from four elemental channels; flat MDR precedes percent resistance.
 rule::MonStats attack;attack.level=30;attack.to_hit=100000;attack.elements[0]={4,100,100,100,0,"A1"};
 for(int resist:{-50,0,50,75,100}){
  rule::Fighter defender;defender.magic_res=resist;defender.mdr=20;rule::Rng rng{1};bool hit=false;
  for(int n=0;n<20&&!hit;++n){const auto damage=rule::monster_blow(defender,30,rule::Motion::standing,attack,false,rng);if(damage.hit){CHECK(damage.damage==rule::resisted(80,resist));hit=true;}}CHECK(hit);
 }
 for(int receiver=0;receiver<3;++receiver){
  Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);
  World w(&data,-1,-1);w.level=&arena;w.player.x=receiver==0?20.1f:.1f;w.player.y=receiver==0?20.1f:.1f;
  auto& f=w.fight;auto& c=w.character;c.character_class=1;c.stats.values[kLevel]=30;c.stats.values[kMaxLife]=c.stats.values[kLife]=256000;
  c.items={worn("cap",1,{{37,0,50},{35,0,20}})};f.update_fighters(1000);CHECK(f.player_combat.magic_res==50);
  c.header.difficulty={0,0,0x80};f.update_fighters(1000);CHECK(f.player_combat.magic_res==50);c.header.difficulty={0x80,0,0};
  c.items[0].props={{37,0,100},{38,0,10}};f.update_fighters(1000);CHECK(f.player_combat.magic_res==85);c.items[0].props={{37,0,50},{35,0,20}};
  if(receiver==1){w.merc.emplace();w.merc->unit.x=20.1f;w.merc->unit.y=20.1f;w.merc->hit_points=1000;w.merc->mode="NU";f.merc_st.life=1000;f.merc_st.level=30;c.merc_items={worn("cap",1,{{37,0,50},{35,0,20}})};CHECK(f.merc_fighter().magic_res==50);}
  if(receiver==2){Fight::Pet pet;pet.monster=make_monster(data,data.monsters.row("zombie1"),20.1f,20.1f,f.rng,0);pet.monster.id=100;pet.monster.hit_points=pet.monster.stats.hit_points=1000;pet.monster.stats.level=30;pet.monster.boss_res[1]=50;pet.where=&arena;pet.wake=100000;pet.revived=true;pet.skill=95;f.pets={pet};CHECK(f.pet_foe(f.pets[0]).fighter.magic_res==50);}
  auto enemy=make_monster(data,data.monsters.row("zombie1"),20.5f,20.1f,f.rng,0);enemy.id=10;enemy.hit_points=50;enemy.stats.hit_points=200;enemy.stats.level=30;enemy.stats.to_hit=100000;enemy.stats.a1_min=enemy.stats.a1_max=0;enemy.stats.elements={};enemy.stats.elements[0]={4,100,100,100,0,"A1"};set_mode(data,enemy,"A1",1000);enemy.struck=false;const auto now=1000+data.npc_timing(enemy.npc,"A1").action_ms();f.monsters={enemy};f.rng=rule::Rng{1};f.update_fighters(now);f.world(true,now,0,{});
  const int life=receiver==0?int(c.stats.get(kLife)/256):receiver==1?w.merc->hit_points:f.pets[0].monster.hit_points;CHECK(life==(receiver==2?950:960));
  f.monsters[0].mode="NU";f.monsters[0].next_act=100000;Missile shot;shot.info=&data.missiles.at("skbowarrow1");shot.x=20.1f;shot.y=20.1f;shot.born=0;shot.dies=10000;shot.owner=10;shot.src=attack;shot.src.a2_min=shot.src.a2_max=0;shot.src.elements[0].mode="A2";f.missiles={shot};f.rng=rule::Rng{1};f.world(true,now+40,0,{});
  const int after_shot=receiver==0?int(c.stats.get(kLife)/256):receiver==1?w.merc->hit_points:f.pets[0].monster.hit_points;CHECK(after_shot==(receiver==2?900:920));
  if(receiver==2){f.pets[0].monster.boss_res[1]=100;shot.struck=std::make_shared<std::vector<int>>();f.missiles={shot};f.rng=rule::Rng{1};f.world(true,now+80,0,{});CHECK(f.pets[0].monster.hit_points==900);}

 }
 // Two attackers must heal independently from their own physical hits, not total mixed damage.
 for(int receiver=0;receiver<3;++receiver){
  Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);World w(&data,-1,-1);w.level=&arena;w.player.x=receiver==0?20.1f:.1f;w.player.y=receiver==0?20.1f:.1f;
  auto& f=w.fight;auto& c=w.character;c.character_class=1;c.stats.values[kLevel]=30;c.stats.values[kMaxLife]=c.stats.values[kLife]=256000;
  if(receiver==0)f.curses[0]={82,1,10000};
  if(receiver==1){w.merc.emplace();w.merc->unit.x=20.1f;w.merc->unit.y=20.1f;w.merc->hit_points=1000;w.merc->mode="NU";f.merc_st.life=1000;f.merc_st.level=30;f.curses[1]={82,1,10000};}
  if(receiver==2){Fight::Pet pet;pet.monster=make_monster(data,data.monsters.row("zombie1"),20.1f,20.1f,f.rng,0);pet.monster.id=100;pet.monster.hit_points=pet.monster.stats.hit_points=1000;pet.monster.stats.level=30;pet.monster.curse={82,1,10000};pet.where=&arena;pet.wake=100000;pet.revived=true;pet.skill=95;f.pets={pet};}
  unsigned now=0;
  for(int k=0;k<2;++k){auto enemy=make_monster(data,data.monsters.row("zombie1"),20.5f,20.1f-.2f*k,f.rng,0);enemy.id=10+k;enemy.hit_points=50+50*k;enemy.stats.hit_points=200+100*k;enemy.stats.level=30;enemy.stats.to_hit=100000;enemy.stats.a1_min=enemy.stats.a1_max=40+40*k;enemy.stats.elements={};enemy.stats.elements[0]={0,100,100,100,0,"A1"};set_mode(data,enemy,"A1",1000);enemy.struck=false;now=1000+data.npc_timing(enemy.npc,"A1").action_ms();f.monsters.push_back(enemy);}
  f.rng=rule::Rng{1};f.update_fighters(now);f.world(true,now,0,{});CHECK(f.monsters[0].hit_points==70&&f.monsters[1].hit_points==140);
  for(auto& enemy:f.monsters){enemy.mode="NU";enemy.next_act=100000;}
  Missile shot;shot.info=&data.missiles.at("skbowarrow1");shot.x=20.1f;shot.y=20.1f;shot.born=0;shot.dies=10000;shot.owner=10;shot.src=f.monsters[0].stats;shot.src.elements[0].mode="A2";shot.src.a2_min=shot.src.a2_max=40;
  f.missiles={shot};f.rng=rule::Rng{1};f.world(true,now+40,0,{});CHECK(f.monsters[0].hit_points==90&&f.monsters[1].hit_points==140);
  f.monsters[0].hit_points=199;shot.struck=std::make_shared<std::vector<int>>();f.missiles={shot};f.rng=rule::Rng{1};f.world(true,now+80,0,{});CHECK(f.monsters[0].hit_points==200&&f.monsters[1].hit_points==140);
  // Expiry prevents further healing even though the next attacks still land.
  for(auto& enemy:f.monsters){set_mode(data,enemy,"A1",20000);enemy.struck=false;}
  f.rng=rule::Rng{1};f.update_fighters(now+19000);f.world(true,now+19000,0,{});CHECK(f.monsters[0].hit_points==200&&f.monsters[1].hit_points==140);
  f.monsters[0].hit_points=0;f.monsters[0].mode="DT";f.monster_life_tap(10,100,1,now);CHECK(f.monsters[0].hit_points==0);f.monster_life_tap(9999,100,1,now);
 }
 // Life Tap must heal the live attacker before Thorns can kill it.
 {Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);World w(&data,-1,-1);w.level=&arena;w.player.x=w.player.y=20.1f;auto& f=w.fight;auto& c=w.character;c.character_class=1;c.stats.values[kLevel]=30;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;c.items={worn("cap",1,{{131,0,25}})};f.curses[0]={82,1,10000};auto enemy=make_monster(data,data.monsters.row("zombie1"),20.5f,20.1f,f.rng,0);enemy.id=10;enemy.hit_points=5;enemy.stats.hit_points=200;enemy.stats.level=30;enemy.stats.to_hit=100000;enemy.stats.a1_min=enemy.stats.a1_max=40;enemy.stats.elements={};enemy.stats.elements[0]={0,100,100,100,0,"A1"};set_mode(data,enemy,"A1",1000);enemy.struck=false;const unsigned now=1000+data.npc_timing(enemy.npc,"A1").action_ms();f.monsters={enemy};f.rng=rule::Rng{1};f.update_fighters(now);f.world(true,now,0,{});CHECK(f.monsters[0].hit_points==15);}
 // Same-frame replacement and per-hit callback: no stale curse, no spell/blocked/immune healing.
 Foe victim;victim.life_tap_level=1;int healed=0,last=-1;victim.life_tap_hit=[&](int id,int physical,int level){last=id;healed+=physical*50/100;CHECK(level==1);};
 victim.take({.hit=true,.damage=120,.physical=20},10);CHECK(healed==10&&last==10);victim.cursed={66,1};victim.take({.hit=true,.damage=40,.physical=40},11);CHECK(healed==10);victim.cursed={82,1};victim.take({.hit=true,.damage=100},11);CHECK(healed==10);victim.take({.blocked=true},11);CHECK(healed==10);victim.take({.hit=true,.damage=40,.physical=40},11);CHECK(healed==30&&last==11);
 std::puts("PASS incoming magic receiver channel/MDR order, no difficulty penalty and gear caps; hostile Life Tap per-source physical hits on hero/merc/pets, expiry and same-frame replacement");
}
static void pet_curse_cases(GameData& data){
 Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);
 World w(&data,-1,-1);w.level=&arena;w.player.x=20.1f;w.player.y=20.1f;auto& f=w.fight;auto& c=w.character;c.character_class=1;c.stats.values[kLevel]=30;c.stats.values[kMaxLife]=c.stats.values[kLife]=256000;
 Fight::Pet initial;initial.monster=make_monster(data,data.monsters.row("zombie1"),21.1f,20.1f,f.rng,0);initial.monster.id=100;initial.monster.stats.level=30;initial.monster.hit_points=initial.monster.stats.hit_points=1000;initial.monster.stats.armor_class=100;initial.where=&arena;initial.revived=true;initial.skill=95;initial.wake=100000;initial.res={50,50,50,100};f.pets={initial};
 auto& p=f.pets[0];const auto* amp=f.skill_named("Amplify Damage");const auto* weak=f.skill_named("Weaken");const auto* decrep=f.skill_named("Decrepify");const auto* lower=f.skill_named("Lower Resist");const auto* maiden=f.skill_named("Iron Maiden");const auto* shout=f.skill_named("Shout");const auto* sight=f.skill_named("Inner Sight");CHECK(amp&&weak&&decrep&&lower&&maiden&&shout&&sight);
 p.buffs.push_back({shout->id,3,50000});f.pet_buffs(p);f.world_now=1000;const int defense=f.pet_foe(p).fighter.defense;CHECK(defense>100&&f.pet_foe(p).fighter.res[3]==100);
 Foe hit;hit.cursed={amp->id,1};f.pet_conditions(p,hit,1000,0);CHECK(p.monster.curse.skill==amp->id&&f.pet_foe(p).fighter.dr_pct==-100&&p.buffs.size()==1);
 hit.cursed={weak->id,1};f.pet_conditions(p,hit,1100,0);CHECK(p.monster.curse.skill==weak->id&&f.pet_debuff_stat(p,25)==-33&&f.pet_foe(p).fighter.dr_pct==0&&f.pet_foe(p).fighter.defense==defense);
 hit.cursed={decrep->id,1};f.pet_conditions(p,hit,1200,0);CHECK(f.pet_debuff_stat(p,67)==-50&&f.pet_animation_rate(p)==50&&f.pet_foe(p).fighter.dr_pct==-50);
 hit.marked={sight->id,1};f.pet_conditions(p,hit,1200,0);CHECK(p.monster.cry.skill==sight->id&&f.pet_foe(p).fighter.defense<defense);
 const auto expiry=std::max(p.monster.curse.until,p.monster.cry.until);f.pet_conditions(p,Foe{},expiry,0);CHECK(p.monster.curse.skill<0&&p.monster.cry.skill<0&&f.pet_foe(p).fighter.defense==defense&&p.buffs.size()==1);
 p.res[0]=110;hit={};hit.cursed={lower->id,1};f.pet_conditions(p,hit,1500,0);CHECK(f.pet_debuff_stat(p,39)==-31&&f.pet_foe(p).fighter.res[0]==104);p.res[0]=100;CHECK(f.pet_foe(p).fighter.res[0]==94);p.res[0]=50;CHECK(f.pet_foe(p).fighter.res[0]==19);
 // Damage reduction changes the physical component only; curse replacement leaves the friendly buff intact.
 auto enemy=make_monster(data,data.monsters.row("zombie1"),21.5f,20.1f,f.rng,0);enemy.id=5;enemy.hit_points=enemy.stats.hit_points=10000;f.monsters={enemy};hit.cursed={weak->id,1};f.pet_conditions(p,hit,2000,0);f.pet_melee_land(p,0,{.hit=true,.damage=140,.phys=40},2000);CHECK(f.monsters[0].hit_points==9874);
 hit.cursed={maiden->id,1};f.pet_conditions(p,hit,2100,0);const int before=p.monster.hit_points;f.pet_melee_land(p,0,{.hit=true,.damage=140,.phys=40},2100);CHECK(p.monster.hit_points==before-80);f.pet_melee_land(p,0,{.hit=true,.damage=100,.phys=0},2100);CHECK(p.monster.hit_points==before-80);
 f.pet_melee_land(p,0,{.hit=true,.blocked=true,.damage=40,.phys=40},2100);CHECK(p.monster.hit_points==before-80);
 // Iron Maiden is a melee event: a friendly physical projectile must not hurt its cursed source.
 p.monster.hit_points=500;f.monsters[0].mode="NU";f.monsters[0].next_act=100000;const int enemy_before=f.monsters[0].hit_points;
 const auto ar=data.missiles.find("skbowarrow1");CHECK(ar!=data.missiles.end());Missile arrow;arrow.info=&ar->second;arrow.x=f.monsters[0].unit.x;arrow.y=f.monsters[0].unit.y;arrow.friendly=true;arrow.monster_source=true;arrow.item_actor=2;arrow.owner=p.monster.id;arrow.born=2100;arrow.dies=10000;arrow.src=p.monster.stats;arrow.src.level=30;arrow.src.to_hit=100000;arrow.src.a2_min=arrow.src.a2_max=40;arrow.src.elements={};f.missiles={arrow};f.rng=rule::Rng{1};f.update_fighters(2200);f.world(true,2200,.04f,{});CHECK(p.monster.hit_points==500&&f.monsters[0].hit_points==enemy_before-40);
 // Incoming poison, cold and stun are retained, expire, and appear in the snapshot.
 p.monster.curse={};p.monster.cry={};Foe effects;effects.poison=256;effects.poison_ticks=25;effects.chill_ticks=100;effects.stun_ticks=5;
 const int healthy=p.monster.hit_points;f.pet_conditions(p,effects,3000,.04f);CHECK(healthy-p.monster.hit_points>=0&&healthy-p.monster.hit_points<=1&&std::abs((healthy-p.monster.hit_points)+p.monster.dot_acc-1)<.0001&&p.monster.poison_until==4000&&p.monster.chill_until>3000&&p.monster.stun_until==3200);const auto poisoned_hp=p.monster.hit_points;
 w.now=3000;auto view=w.view();CHECK(view.pets.size()==1&&std::ranges::contains(view.pets[0].states,std::string_view("poison"))&&std::ranges::contains(view.pets[0].states,std::string_view("cold")));
 const auto end=std::max(p.monster.poison_until,p.monster.chill_until);f.pet_conditions(p,Foe{},end,1);CHECK(p.monster.hit_points==poisoned_hp);
 // Enemy missile state/damage still goes to pet100 after an earlier dead pet is erased that same tick.
 f.monsters.clear();f.pets={initial};auto dead=initial;dead.monster.id=99;dead.monster.hit_points=0;dead.monster.mode="DT";dead.monster.mode_until=0;f.pets.insert(f.pets.begin(),dead);f.rng=rule::Rng{1};
 const auto mark=std::ranges::find(data.skills.rows,std::string("fingermagecurse"),&rule::Skill::auratarget);CHECK(mark!=data.skills.rows.end());
 const auto row=std::ranges::find_if(data.missiles,[](const auto& x){return x.second.hit_func==19;});CHECK(row!=data.missiles.end());
 Missile shot;shot.info=&row->second;shot.x=21.1f;shot.y=20.1f;shot.born=0;shot.dies=10000;shot.skill=mark->id;shot.level=1;shot.src.level=30;shot.src.to_hit=100000;shot.src.a2_min=shot.src.a2_max=10;f.missiles={shot};f.update_fighters(1000);f.world(true,1000,.04f,{});
 CHECK(f.pets.size()==1&&f.pets[0].monster.id==100&&f.pets[0].monster.hit_points==990&&f.pets[0].monster.cry.skill==mark->id);
 std::puts("PASS pet hostile curse/mark replacement/expiry with friendly buffs; immunity breaking; Weaken/Decrepify/IM; poison/cold/stun and stable-ID missile delivery");
}
static void reflection_cases(GameData& data){
 Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);
 for(int kind=0;kind<5;++kind){
  rule::Rng rng;auto m=make_monster(data,data.monsters.row("zombie1"),20.1f,20.1f,rng,0);m.hit_points=m.stats.hit_points=10000;m.stats.level=30;m.stats.to_hit=100000;m.stats.a1_min=m.stats.a1_max=kind==1?0:40;m.stats.elements={};m.stats.elements[0]={0,100,100,100,0,"A1"};m.boss_res[0]=kind==2?100:50;m.reflect_pct=kind==3?100:0;if(kind==4)m.curse={66,1,100000};
  bool landed=false;
  for(int attempt=0;attempt<12&&!landed;++attempt){
   rule::Fighter defender;defender.dr_pct=50;defender.thorns_pct=kind==3?0:100;
   std::array<Foe,1> foes{Foe{20.5f,20.1f,30,true,false,defender}};std::vector<Missile> shots;
   const unsigned start=1000+attempt*1000;set_mode(data,m,"A1",start);m.struck=false;
   monster_update(data,arena,m,foes,rng,start+data.npc_timing(m.npc,"A1").action_ms(),0,{},shots,{},nullptr,nullptr,nullptr);
   landed=foes[0].hits>0;
  }
  const int expected=(kind==1||kind==2)?0:kind==4?30:10;if(!landed||m.hit_points!=10000-expected)std::fprintf(stderr,"REFLECT kind=%d landed=%d lost=%d expected=%d\n",kind,landed,10000-m.hit_points,expected);CHECK(landed);CHECK(m.hit_points==10000-expected);
 }
 std::puts("PASS actual melee Thorns/IM physical-only reflection, attacker resistance/immunity and Amplify Damage; fire-only attacks do not reflect");
}
static void revived_cases(GameData& data){
 const auto* revive=data.skills.get(95);CHECK(revive);
 for(const char* name:{"sk_archer1","quillrat1","zombie1"}){
  Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(64*5*64*5);
  World w(&data,-1,-1);auto& f=w.fight;auto& c=w.character;w.level=&arena;w.player.x=20.1f;w.player.y=20.1f;
  c.character_class=1;c.stats.values[kLevel]=30;c.stats.values[kMaxLife]=c.stats.values[kLife]=256000;
  const int type=data.monsters.row(name);CHECK(type>=0);
  auto body=make_monster(data,type,20.1f,20.1f,f.rng,0);body.id=1;body.hit_points=0;body.mode="DD";
  body.stats.a1_min=body.stats.a1_max=40;body.stats.to_hit=100000;body.stats.elements={};body.stats.elements[0]={2,100,20,20,50,"A1"};
  CHECK(revivable_corpse(data,body));f.monsters={body};f.next_id=3;
  f.summon(*revive,5,body.unit.x,body.unit.y,1000);CHECK(f.pets.size()==1&&f.pets[0].revived);
  auto target=make_monster(data,data.monsters.row("skeleton1"),20.7f,20.1f,f.rng,0);target.id=2;target.hit_points=target.stats.hit_points=10000;target.boss_res={};target.stats.armor_class=0;target.next_act=100000;target.curse={82,1,100000};
  f.monsters.push_back(target);
  f.pets[0].monster.hit_points=std::max(f.pets[0].monster.stats.hit_points/4,1);
  auto bystander=f.pets[0];bystander.monster.id=9000;bystander.wake=100000;f.pets.push_back(bystander);
  const auto injured=f.pets[0].monster.hit_points,other_life=f.pets[1].monster.hit_points;const auto hero_life=c.stats.get(kLife);
  auto& pet=f.pets[0];pet.wake=0;pet.until=0;pet.target=1;
  set_mode(data,pet.monster,"A1",2000);pet.monster.struck=false;
  const auto now=2000+data.npc_timing(pet.monster.npc,"A1").action_ms();f.pets_turn(now,0,{});
  const auto& info=data.monsters.types[std::size_t(type)];
  if(!info.miss_a1.empty()){
   CHECK(f.pending.size()==1);const auto shot=f.pending[0];CHECK(shot.friendly&&shot.monster_source&&shot.item_actor==2&&shot.owner==pet.monster.id&&shot.src.elements[0].mode=="A2");
   const auto& row=data.missiles.at(info.miss_a1);CHECK(shot.src.a2_min==40*row.src_damage/128+row.min&&shot.src.a2_max==40*row.src_damage/128+row.max);
   rule::Target victim;victim.level=30;victim.hit_points=victim.max_hp=10000;rule::Rng rng;bool chilled=false;
   for(int n=0;n<50;++n){const auto blow=rule::pet_monster_blow(shot.src,victim,rng,true);chilled=chilled||(blow.hit&&blow.chill_ticks>0&&blow.damage>=20);}CHECK(chilled);
   // Deliver the friendly projectile through the actual world collision path.
   f.pets[0].wake=100000;f.update_fighters(now);f.pending.clear();
   for(int attempt=0;attempt<12&&f.monsters[1].hit_points==10000;++attempt){
    auto next=shot;next.born=now+attempt*1000;next.dies=next.born+std::uint32_t(row.range)*40;f.pending.push_back(next);
    for(int tick=1;tick<=20&&f.monsters[1].hit_points==10000;++tick)f.world(true,next.born+tick*40,.04f,{});
   }
   CHECK(f.monsters[1].hit_points<10000&&f.monsters[1].chill_until>now);
  }else{
   for(int attempt=1;attempt<12&&f.monsters[1].hit_points==10000;++attempt){auto& again=f.pets[0];again.target=1;set_mode(data,again.monster,"A1",2000+attempt*1000);again.monster.struck=false;f.pets_turn(2000+attempt*1000+data.npc_timing(again.monster.npc,"A1").action_ms(),0,{});}
   CHECK(f.monsters[1].hit_points<10000&&f.monsters[1].chill_until>now);
  }
  CHECK(f.pets[0].monster.hit_points>injured&&f.pets[0].monster.hit_points<=f.pets[0].monster.stats.hit_points&&f.pets[1].monster.hit_points==other_life&&c.stats.get(kLife)==hero_life);
  auto& follower=f.pets[0];follower.wake=0;follower.monster.next_act=0;follower.monster.unit.x=40.1f;follower.monster.unit.y=40.1f;follower.monster.unit.walking=false;set_mode(data,follower.monster,"NU",30000);
  f.pets_turn(30000,0,{});CHECK(std::hypot(f.pets[0].monster.unit.x-w.player.x,f.pets[0].monster.unit.y-w.player.y)<3.f);
 }
 rule::MonStats source;source.level=30;source.to_hit=100000;source.a1_min=source.a1_max=40;source.elements[0]={2,100,20,20,50,"A1"};source.elements[1]={4,100,10,10,0,"A1"};source.elements[2]={3,100,5,5,40,"A1"};
 rule::Target victim;victim.level=30;victim.hit_points=victim.max_hp=10000;rule::Rng rng;bool landed=false;
 for(int n=0;n<50;++n){auto hit=rule::pet_monster_blow(source,victim,rng);if(hit.hit){CHECK(hit.damage==70&&hit.phys==40&&hit.chill_ticks==50&&hit.poison>0&&hit.poison_ticks==80);landed=true;}}CHECK(landed);
 victim.res={100,100,100,100,100,100};landed=false;
 for(int n=0;n<50;++n){auto hit=rule::pet_monster_blow(source,victim,rng);if(hit.hit){CHECK(hit.damage==0&&hit.poison==0&&hit.chill_ticks==0);landed=true;}}CHECK(landed);
 std::puts("PASS real revived skeleton archers/quill rats carry monster missiles; melee/cold/magic/poison respect victim resistances");
}
static void synthetic(){
 GameData data;data.town.id=1;data.skills.rows.resize(100);data.monsters.types.resize(1);data.rules.item_info["swd"].kind=2;
 auto& bolt=data.skills.rows[5];bolt.id=5;bolt.name="Bolt";bolt.cls="sor";bolt.srvmissile="bolt";data.missiles["bolt"].range=20;data.missiles["bolt"].vel=10;
 auto& aura=data.skills.rows[10];aura.id=10;aura.aura=true;aura.srvdofunc=ServerDoFunction::kFriendlyAura;aura.aurarange=rule::compile_calc("50",data.skills.names);aura.aurastat[0]=25;aura.aura_calc[0]=rule::compile_calc("100",data.skills.names);aura.passive_stat[0]=25;aura.passive_calc[0]=rule::compile_calc("200",data.skills.names);
 auto& tap=data.skills.rows[66];tap.id=66;tap.auratarget="lifetap";tap.calc[0]=rule::compile_calc("50",data.skills.names);
 World world(&data,-1,-1);auto& f=world.fight;auto& c=world.character;c.stats.values[kLevel]=30;c.stats.values[kMaxLife]=c.stats.values[kLife]=25600;c.stats.values[kMana]=0;
 c.items={worn("cap",1,{{198,(5<<6)|3,100},{151,10,3}}),worn("cap",3,{{151,10,4},{127,0,99}})};f.refresh_item_effects();CHECK(f.aura_sources.size()==1&&f.aura_sources[0].level==7);CHECK(f.aura_stats(false)[25]==200);
 f.item_event(f.OnHit,-1,0);CHECK(f.item_casts.size()==1&&f.item_casts[0].level==3);f.run_item_casts(0);CHECK(f.pending.size()==1&&f.pending[0].level==3&&c.stats.get(kMana)==0&&c.header.right_skill==0);f.pending.clear();
 // Matching spell+level chances sum, different levels remain distinct.
 c.items[0].props={{195,(5<<6)|3,40},{195,(5<<6)|3,60},{196,(5<<6)|3,100},{197,(5<<6)|3,100},{199,(5<<6)|3,100},{201,(5<<6)|3,100}};c.items[1].props.clear();f.refresh_item_effects();f.hit(10);CHECK(f.item_casts.size()==1);f.item_casts.clear();
 for(int event:{f.OnKill,f.OnDeath,f.OnLevel,f.OnStruck}){f.item_event(event,-1,10);CHECK(f.item_casts.size()==1);f.item_casts.clear();}
 c.items[0].slot=11;f.refresh_item_effects();f.item_event(f.OnAttack,-1,0);CHECK(f.item_casts.empty());c.items[0].slot=1;c.items[0].flags|=0x100;f.refresh_item_effects();f.item_event(f.OnAttack,-1,0);CHECK(f.item_casts.empty());c.items[0].flags=0;
 // An unused second weapon cannot supply offensive procs.
 c.items={worn("swd",4,{}),worn("swd",5,{{198,(5<<6)|3,100}})};f.refresh_item_effects();f.item_event(f.OnHit,-1,0);CHECK(f.item_casts.empty());c.items.erase(c.items.begin());f.refresh_item_effects();f.item_event(f.OnHit,-1,0);CHECK(f.item_casts.size()==1);f.item_casts.clear();
 world.merc.emplace();world.merc->hit_points=50;world.merc->unit.x=1;f.merc_st.life=200;f.merc_st.level=30;c.items={worn("cap",1,{{151,10,7}})};c.merc_items={worn("cap",1,{{151,10,12},{198,(5<<6)|2,100}})};f.refresh_item_effects();CHECK(f.aura_stats(false)[25]==100&&f.aura_stats(true)[25]==200);world.merc->unit.x=100;CHECK(f.aura_stats(false)[25]==200);world.merc->unit.x=1;
 f.item_event(f.OnHit,-1,0,1);CHECK(f.item_casts.size()==1&&f.item_casts[0].actor==1&&f.item_casts[0].x==1);f.run_item_casts(0);CHECK(f.pending.back().by_merc&&f.pending.back().item_actor==1);
 c.merc_items[0].props={{151,10,7}};f.refresh_item_effects();CHECK(f.aura_stats(false)[25]==200&&f.aura_stats(true)[25]==200);
 aura.aurastat[1]=6;aura.aura_calc[1]=rule::compile_calc("256",data.skills.names);c.stats.values[kLife]-=1024;const auto before_heal=c.stats.get(kLife);f.aura_pulse(1000);CHECK(c.stats.get(kLife)==before_heal+256&&world.merc->hit_points==51);
 c.items.clear();c.merc_items.clear();f.refresh_item_effects();CHECK(f.aura_sources.empty());
 // Ordinary passive evaluation must not grant an inactive aura's own bonus.
 rule::CalcEnv env{[](int){return 20;},[](int){return 20;},[](int){return 0;},30,nullptr};CHECK(rule::passive_stats(data.skills,env).empty());
 Foe foe;foe.poison=256;foe.poison_ticks=10;f.merc_conditions(foe,0,.041f);CHECK(foe.damage==1&&world.merc->poison_until==400);Foe next;f.merc_conditions(next,40,.041f);CHECK(next.damage==1);Foe weak;weak.poison=128;weak.poison_ticks=20;f.merc_conditions(weak,80,0);CHECK(world.merc->poison_until==400);Foe cold;cold.chill_ticks=5;cold.stun_ticks=2;f.merc_conditions(cold,100,0);CHECK(world.merc->chill_until==300&&world.merc->stun_until==180);
 f.monsters.resize(1);auto& target=f.monsters[0];target.type=0;target.hit_points=1000;target.stats.hit_points=1000;target.stats.level=1;target.curse={66,1,1000};const auto life=c.stats.get(kLife);{Fight::ActorScope owner(f.damage_actor,1);f.land(0,{.hit=true,.damage=20,.life=10,.phys=20},false,100);}CHECK(world.merc->hit_points==71&&c.stats.get(kLife)==life);
 c.merc_items={worn("cap",1,{{93,0,50}})};world.merc->chill_until=0;const int fast=f.merc_animation_rate();c.merc_items.clear();const int plain=f.merc_animation_rate();CHECK(fast>plain);world.merc->chill_until=100;CHECK(f.merc_animation_rate()<plain);
 data.monsters.types[0].base=0x146;data.area_level.resize(2);data.area_level[1]={20,40,60,10};c.expansion=true;f.game_difficulty=1;Monster trap;trap.type=0;f.configure_trap(trap,&data.town);CHECK(trap.stats.level==40&&trap.stats.to_hit==90&&trap.stats.elements[0].min==20&&trap.stats.elements[0].max==60);
 data.monsters.types[0].base=0x154;f.monsters.resize(4);for(auto& bone:f.monsters){bone.type=0;bone.leader=0;bone.hit_points=10;bone.stats.hit_points=10;}f.monsters[2].hit_points=0;std::array<Foe,2> foes{};f.boss_events(foes,200);for(const auto& bone:f.monsters)CHECK(!bone.alive());
 // Both electrical burst families use the original four directions and
 // two independent seeded dynamic paths per direction, not a random fan.
 data.missiles["buglightning"].vel=10;data.missiles["buglightning"].range=100;
 data.missiles["lightunique"].vel=10;data.missiles["lightunique"].range=100;
 f.pending.clear();f.monsters.clear();f.monsters.resize(1);auto& bug=f.monsters[0];bug.type=0;bug.unit.x=20.1f;bug.unit.y=30.1f;bug.stats.level=20;bug.hit_points=100;bug.stats.hit_points=100;bug.mode="GH";bug.fx_mode="NU";data.monsters.types[0].base=0x5b;
 f.boss_events(foes,1000);CHECK(f.pending.empty());f.boss_events(foes,1080);CHECK(f.pending.size()==8);
 const std::array<std::pair<int,int>,4> dirs{{{0,-1},{1,0},{0,1},{-1,0}}};
 for(std::size_t i=0;i<f.pending.size();++i){const auto& shot=f.pending[i];CHECK(shot.bolt==int(i%2)&&shot.to&&shot.level==20);const auto [dx,dy]=dirs[i/2];CHECK(int(std::floor(shot.to->first*5))==int(std::floor(bug.unit.x*5))+dx&&int(std::floor(shot.to->second*5))==int(std::floor(bug.unit.y*5))+dy);CHECK(shot.struck==f.pending[0].struck);}
 f.pending.clear();data.monsters.types[0].base=0;bug.boss=rule::Boss::unique;bug.mods={rule::umod::lightning};bug.hit_points=90;bug.last_hp=100;f.boss_events(foes,2000);CHECK(f.pending.size()==8&&f.pending[0].level==10&&f.pending[1].bolt==1);
 // Packing commits only after a complete layout; a full failure preserves coordinates.
 data.rules.item_info["big"].width=2;data.rules.item_info["big"].height=3;
 std::vector<Item> bags(3);for(std::size_t i=0;i<bags.size();++i){bags[i].code=i?"cap":"big";bags[i].id=int(i)+1;bags[i].location=item_location::kStored;bags[i].panel=item_panel::kInventory;bags[i].column=3;bags[i].row=0;}
 CHECK(d2mini::arrange_inventory(data.rules,bags));CHECK(bags[0].id==1&&bags[0].column==0&&bags[0].row==0);
 const auto old_column=bags[0].column;CHECK(!d2mini::arrange_inventory(data.rules,bags,1,1)&&bags[0].column==old_column);
 // Life Tap heals only the actual living attacker; no stale curse or source resurrection.
 f.monsters.resize(1);auto& tapped=f.monsters[0];tapped.type=0;tapped.stats.hit_points=tapped.hit_points=10000;tapped.mode="NU";tapped.curse={66,1,1000};
 f.pets.clear();Fight::Pet p1;p1.monster.id=101;p1.monster.hit_points=10;p1.monster.stats.hit_points=100;p1.monster.mode="NU";p1.where=world.level;f.pets.push_back(p1);auto p2=p1;p2.monster.id=102;f.pets.push_back(p2);
 auto pet_hit=[&](int id,const rule::Blow& hit,unsigned now){Fight::ActorScope owner(f.damage_actor,2),source(f.damage_pet_id,id);f.land(0,hit,false,now);};
 const auto owner_hp=c.stats.get(kLife);pet_hit(101,{.hit=true,.damage=20,.phys=20},100);CHECK(f.pets[0].monster.hit_points==20&&f.pets[1].monster.hit_points==10&&c.stats.get(kLife)==owner_hp);
 pet_hit(101,{.hit=true,.damage=20,.phys=0},100);CHECK(f.pets[0].monster.hit_points==20);
 pet_hit(101,{.hit=true,.blocked=true,.damage=20,.phys=20},100);CHECK(f.pets[0].monster.hit_points==20);
 pet_hit(101,{.hit=true,.damage=20,.phys=20},1000);CHECK(f.pets[0].monster.hit_points==20);
 pet_hit(999,{.hit=true,.damage=20,.phys=20},100);CHECK(f.pets[0].monster.hit_points==20);
 f.pets[0].monster.hit_points=99;pet_hit(101,{.hit=true,.damage=20,.phys=20},100);CHECK(f.pets[0].monster.hit_points==100);
 f.pets[0].monster.hit_points=0;f.pets[0].monster.mode="DT";pet_hit(101,{.hit=true,.damage=20,.phys=20},100);CHECK(f.pets[0].monster.hit_points==0);
 // A lethal hit still heals its living source; immune physical damage does not.
 f.pets[1].monster.hit_points=10;tapped.hit_points=1;pet_hit(102,{.hit=true,.damage=20,.phys=20},100);CHECK(f.pets[1].monster.hit_points==20);
 tapped.hit_points=100;tapped.mode="NU";tapped.curse={66,1,1000};tapped.boss_res[0]=100;pet_hit(102,{.hit=true,.damage=20,.phys=20},100);CHECK(f.pets[1].monster.hit_points==20);
 tapped.boss_res[0]=0;c.stats.values[kLife]=0;f.pmode=kModeDT;f.land(0,{.hit=true,.damage=20,.life=20,.phys=20},true,100);CHECK(c.stats.get(kLife)==0);
 world.merc->hit_points=0;world.merc->mode="DT";{Fight::ActorScope owner(f.damage_actor,1);f.land(0,{.hit=true,.damage=20,.life=20,.phys=20},false,100);}CHECK(world.merc->hit_points==0);
 // Delayed children preserve summon ownership across ticker and explosion callbacks.
 auto& generator=data.missiles["pet-generator"];generator.srv_do=6;generator.sub="bolt";generator.hit_func=2;generator.hit_sub="bolt";
 f.missiles.clear();f.pending.clear();Missile parent;parent.info=&generator;parent.friendly=true;parent.skill=5;parent.level=3;parent.owner=102;parent.item_actor=2;parent.born=0;parent.dies=1000;f.missiles.push_back(parent);f.missile_tick(80);
 CHECK(!f.pending.empty()&&f.pending.back().item_actor==2&&f.pending.back().owner==102);f.pending.clear();f.burst(parent,120);CHECK(f.pending.size()==1&&f.pending[0].item_actor==2&&f.pending[0].owner==102&&f.damage_actor==0&&f.damage_pet_id==-1);
 std::puts("PASS summon Life Tap source identity, caps, expiry, elemental-only, blocked, immune, missing/dead source and lethal hit");
 std::puts("PASS item event grouping, free casting, source filtering, aura stacking/range, inactive aura exclusion, merc poison/leech/LifeTap/speed and trap scaling");
}
int main(int argc,char**argv)try{synthetic();flat_reduction_cases();
 if(argc>1){auto data=load_game_data(argv[1],std::filesystem::path(argv[1])/"LODPatch_114d.exe",3,false);CHECK(data);reddit_firestorm_sound_cases(*data);breath_and_action_sound_cases(*data);attack_panel_cases(*data);missile_damage_function_cases(*data);animation_and_motion_cases(*data);compact_pack_cases(*data);state_group_health_cases(*data);cold_curse_enchant_cases(*data);shadow_passive_cases(*data);shared_trap_cap_cases(*data);summon_death_cases(*data);shadow_finisher_cases(*data);dragon_tail_cases(*data);shadow_affix_child_cases(*data);shadow_item_projectile_cases(*data);contagion_equipment_cases(*data);rabies_shadow_cases(*data);poison_item_cases(*data);socket_durability_cases(*data,argv[1]);summon_level_cases(*data);real_replenish_cases(*data);explicit_cast_cases(*data);ammo_fill_cases(*data);shop_fill_cases(*data);inferno_stream_cases(*data);shadow_flight_cases(*data);allied_aura_cases(*data);pickup_gamble_corpse_cases(*data);skill_completion_cases(*data);incoming_defense_cases(*data);inventory_continuation_cases(*data);broad_mechanics_cases(*data);friendly_recovery_cases(*data);flat_reduction_world_cases(*data);magic_lifetap_cases(*data);pet_curse_cases(*data);reflection_cases(*data);revived_cases(*data);World world(&*data,-1,-1);auto& f=world.fight;auto& c=world.character;c.character_class=3;c.stats.values[kLevel]=30;c.stats.values[kMaxLife]=c.stats.values[kLife]=256000;c.stats.values[kMaxMana]=25600;c.stats.values[kMana]=0;
  const auto* holy=f.skill_named("Holy Fire");CHECK(holy);const auto& ids=data->skills.class_ids[3];const auto at=std::ranges::find(ids,holy->id);CHECK(at!=ids.end());c.stats.skills[std::size_t(at-ids.begin())]=10;f.aura=0;f.update_fighters(0);CHECK(f.player_combat.elem[0].second==0);f.aura=holy->id;f.update_fighters(40);CHECK(f.player_combat.elem[0].second>0);f.aura=0;c.stats.skills.fill(0);
  const auto* meditation=f.skill_named("Meditation");CHECK(meditation);c.items={worn("cap",1,{{151,meditation->id,12}})};f.update_fighters(80);CHECK(f.psum[27]>0&&f.aura_sources.size()==1&&f.aura_sources[0].level==12);
  const auto* amplify=f.skill_named("Amplify Damage");const auto* nova=f.skill_named("Nova");CHECK(amplify&&nova);c.items={worn("cap",1,{{198,(amplify->id<<6)|1,100},{201,(nova->id<<6)|3,100}})};f.update_fighters(120);Monster fallen;fallen.type=data->monsters.row("fallen1");CHECK(fallen.type>=0);fallen.hit_points=1000;fallen.stats.hit_points=1000;fallen.stats.level=1;f.add_monster(fallen);f.item_event(f.OnHit,0,120);f.run_item_casts(120);CHECK(f.monsters[0].curse.skill==amplify->id&&c.stats.get(kMana)==0);f.item_event(f.OnStruck,0,160);f.run_item_casts(160);CHECK(!f.pending.empty()&&c.stats.get(kMana)==0);
  const auto* prayer=f.skill_named("Prayer");CHECK(prayer);world.merc.emplace();world.merc->hit_points=50;f.merc_st.life=200;f.merc_st.level=30;world.merc->unit.x=world.player.x;world.merc->unit.y=world.player.y;f.merc_ids[0]=prayer->id;f.merc_levels[0]=10;f.merc_aura=prayer->id;c.items.clear();c.merc_items={worn("cap",1,{{151,meditation->id,12}})};f.refresh_item_effects();const auto hp=c.stats.values[kLife];c.stats.values[kLife]=hp-10000;f.aura_pulse(1000);CHECK(c.stats.values[kLife]>hp-10000&&world.merc->hit_points>50);
  // Selected Prayer costs mana only when it heals; equipment/merc sources are free.
  f.merc_aura=-1;c.merc_items.clear();c.items.clear();f.aura=prayer->id;const auto prayer_index=std::ranges::find(ids,prayer->id);CHECK(prayer_index!=ids.end());c.stats.skills[std::size_t(prayer_index-ids.begin())]=10;
  c.stats.values[kMana]=0;f.refresh_item_effects();const auto injured=c.stats.values[kLife];f.aura_pulse(3000);CHECK(c.stats.values[kLife]==injured&&!f.prayer_no_mana_regen);
  c.stats.values[kMana]=25600;f.aura_pulse(10000);CHECK(c.stats.values[kLife]>injured&&c.stats.values[kMana]<25600&&f.prayer_no_mana_regen);
  c.stats.values[kLife]=c.stats.values[kMaxLife];world.merc->hit_points=f.merc_max_life();const auto paid=c.stats.values[kMana];f.aura_pulse(20000);CHECK(c.stats.values[kMana]==paid&&!f.prayer_no_mana_regen);
  const auto* sanctuary=f.skill_named("Sanctuary");CHECK(sanctuary);f.aura=0;c.items={worn("cap",1,{{151,sanctuary->id,10}})};f.refresh_item_effects();auto& undead=f.monsters[0];undead.type=data->monsters.row("skeleton1");CHECK(undead.type>=0&&data->monsters.types[std::size_t(undead.type)].undead);undead.curse={};undead.cry={};undead.boss_res[0]=110-data->monsters.types[std::size_t(undead.type)].diff[std::size_t(undead.difficulty)].res[0];CHECK(f.target_of(0).res[0]==0);{Fight::ActorScope ally(f.damage_actor,1);CHECK(f.target_of(0).res[0]==110);}world.merc.reset();{Fight::ActorScope ally(f.damage_actor,1);CHECK(f.target_of(0).res[0]==110);}
  const auto* armor=f.skill_named("Frozen Armor");const auto* enchant=f.skill_named("Enchant");CHECK(armor&&enchant);f.add_merc_buff(*armor,5,0);f.add_merc_buff(*enchant,5,0);CHECK(f.merc_buffs.size()==2);f.add_merc_buff(*enchant,7,10);CHECK(f.merc_buffs.size()==2&&f.merc_buffs.back().level==7);
  CHECK(data->sound_index.contains("item_gold"));
  const auto* conviction=f.skill_named("Conviction");CHECK(conviction);rule::Target immune;immune.res[2]=110;const auto original=immune.res;f.apply_target_stats(*conviction,10,immune,&original);CHECK(immune.res[2]==95);f.apply_target_stats(*conviction,10,immune,&original);CHECK(immune.res[2]==80);
  c.items={worn("hax",4,{{kFireMinDamage,0,13},{kFireMaxDamage,0,13}}),worn("clb",5,{{kColdMinDamage,0,9},{kColdMaxDamage,0,9},{195,(nova->id<<6)|3,100}})};c.stats.values[kLife]=c.stats.values[kMaxLife];f.refresh_item_effects();const auto main_hand=f.player_fighter(nullptr,nullptr,nullptr,nullptr,4),off_hand=f.player_fighter(nullptr,nullptr,nullptr,nullptr,5);CHECK(main_hand.elem[0].first==13&&main_hand.elem[2].first==0&&off_hand.elem[0].first==0&&off_hand.elem[2].first==9);
  const auto* doubles=f.skill_named("Double Swing");CHECK(doubles);f.dual_wield=true;f.swing_skill=doubles->id;f.seq_struck=2;f.attack_mon=-1;f.item_casts.clear();f.hit(25000);CHECK(f.item_casts.size()==1&&f.item_casts[0].skill==nova->id);f.item_casts.clear();f.seq_struck=1;f.hit(25040);CHECK(f.item_casts.empty());
  const auto* valkyrie=f.skill_named("Valkyrie");CHECK(valkyrie);f.pets.clear();f.merc_st.level=10;world.merc.emplace();world.merc->hit_points=50;
  f.summon(*valkyrie,5,world.player.x,world.player.y,26000);{Fight::ActorScope owner(f.damage_actor,1);f.summon(*valkyrie,5,world.player.x,world.player.y,26040);f.summon(*valkyrie,5,world.player.x,world.player.y,26080);}CHECK(f.pets.size()==2&&std::ranges::count_if(f.pets,[](const auto& pet){return pet.merc_owner;})==1);
  const auto* decoy=f.skill_named("Dopplezon");CHECK(decoy&&decoy->srvdofunc==ServerDoFunction::kDecoy);f.pets.clear();f.merc_st.life=123;c.merc_items.clear();const auto owner_life=c.stats.values[kMaxLife];
  f.summon(*decoy,5,world.player.x,world.player.y,27000);CHECK(f.pets.size()==1);const auto player_decoy=f.pets.back().monster.hit_points;
  {Fight::ActorScope owner(f.damage_actor,1);f.summon(*decoy,5,world.player.x,world.player.y,27000);}CHECK(f.pets.size()==2);const auto merc_decoy=f.pets.back().monster.hit_points;CHECK(merc_decoy<player_decoy&&c.stats.values[kMaxLife]==owner_life);
  world.level=&data->town;const auto expiry=std::max(f.pets[0].until,f.pets[1].until);CHECK(expiry>27000);f.pets_turn(expiry,0,{});CHECK(std::ranges::all_of(f.pets,[](const auto& p){return !p.monster.alive();}));f.pets.clear();
  f.merc_st.def=100;f.world_now=10;const int buffed=f.merc_fighter().defense;for(auto& buff:f.merc_buffs)buff.until=11;f.world_now=12;CHECK(f.merc_fighter().defense<buffed);
  if(std::getenv("D2_AURA_BENCH")){
   f.aura=0;c.items={worn("cap",1,{{151,sanctuary->id,10}})};f.refresh_item_effects();auto source=f.aura_sources.front();volatile int sink=0;
   for(bool cached:{false,true}){auto probe=source;if(!cached)probe.radius=-1;const auto start=std::chrono::steady_clock::now();for(int i=0;i<200000;++i)sink=int(sink)+int(f.aura_reaches(probe,world.player.x+float(i%20)/10,world.player.y));const auto us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();std::printf("aura_range_cached=%d calls=200000 microseconds=%lld checksum=%d\n",cached,(long long)us,int(sink));}
  }
  // Successful bank-funded buying and capped-gold selling still have a cue.
  c.items.clear();world.store.npc=0;world.store.npc_id="akara";world.store.tab=3;world.store.gamble=false;world.store.tabs[3]={worn("hp1",0,{})};world.store.perm={"hp1"};c.stats.values[kGold]=0;c.stats.values[kGoldBank]=100000;world.cues.due.clear();world.deal(cmd::Buy{0,3});CHECK(c.items.size()==1&&c.stats.values[kGold]==0&&!world.cues.due.empty()&&world.cues.due.back().sound==data->sound_index.at("item_gold"));
  c.stats.values[kGold]=c.stats.get(kLevel)*10000;const auto full_purse=c.stats.values[kGold];world.cues.due.clear();world.deal(cmd::Sell{c.items[0].id});CHECK(c.items.empty()&&c.stats.values[kGold]==full_purse&&!world.cues.due.empty());
  // Quest and explicitly unsellable items must survive both bag and cursor paths.
  for(const char* code:{"box","msf","vip","hst","qey","qbr","qhr","qf2"}){
   auto quest_item=worn(code,0,{});quest_item.id=500;quest_item.location=item_location::kStored;quest_item.panel=item_panel::kInventory;
   CHECK(!rule::sellable(data->rules,quest_item));c.items={quest_item};world.cues.due.clear();const auto purse=c.stats.get(kGold);world.deal(cmd::Sell{500});CHECK(c.items.size()==1&&c.items[0].code==code&&c.stats.get(kGold)==purse&&world.cues.due.empty());
   c.items.clear();quest_item.location=item_location::kCursor;world.held=quest_item;world.deal(cmd::Sell{500});CHECK(world.held&&world.held->code==code&&c.items.empty()&&c.stats.get(kGold)==purse);world.held.reset();
  }
  auto marked=worn("hax",4,{});marked.flags|=0x1000;CHECK(!rule::sellable(data->rules,marked));marked.flags&=~0x1000;CHECK(rule::sellable(data->rules,marked));
  auto bogus=worn("custom",0,{});data->rules.item_info["custom"].type="ques";CHECK(!rule::sellable(data->rules,bogus));
  // Exercise the actual missile dispatcher, including seed and lifetime cap.
  for(const char* name:{"lightunique","buglightning","coldunique","corpsepoisoncloud"})CHECK(data->missiles.contains(name));
  f.pending.clear();Monster caster;caster.unit.x=20.1f;caster.unit.y=20.1f;caster.stats.level=30;auto ring=std::make_shared<std::vector<int>>();
  f.boss_missile(caster,"buglightning",1,0,ring,30000,30,1);CHECK(f.pending.size()==1);auto& shot=f.pending[0];shot.dies=shot.born+100*40;const int sx=data->town.world_x*5+int(std::floor(shot.x*5)),sy=data->town.world_y*5+int(std::floor(shot.y*5));const int tx=data->town.world_x*5+int(std::floor(shot.to->first*5)),ty=data->town.world_y*5+int(std::floor(shot.to->second*5));
  const auto expected=rule::wiggle_points(sx,sy,tx,ty,77,rule::Rng{std::uint32_t(tx+shot.bolt)});missile_fly(data->town,shot,[](float,float){return std::vector<std::array<int,4>>{};});CHECK(shot.flight.points==expected&&shot.dies==shot.born+77*40);
  if(std::getenv("D2_MOD_AUDIT")){
   const std::array<int,9> remaining{21,31,32,33,34,35,40,41,42};
   for(const auto& unique:data->superuniques)for(int mod:unique.mods)if(std::ranges::find(remaining,mod)!=remaining.end())std::printf("MOD_AUDIT superunique=%s mod=%d\n",unique.name.c_str(),mod);
   for(const auto& type:data->monsters.types)for(int mod:rule::family_mods(type.base))if(std::ranges::find(remaining,mod)!=remaining.end())std::printf("MOD_AUDIT family=%s base=%d mod=%d\n",type.id.c_str(),type.base,mod);
  }
  // Revive checks the original table flags before consuming any corpse.
  const auto* revive=f.skill_named("Revive");CHECK(revive);const int fallen_type=data->monsters.row("fallen1"),diablo_type=data->monsters.row("diablo");CHECK(fallen_type>=0&&diablo_type>=0&&data->monsters.types[fallen_type].revive&&!data->monsters.types[diablo_type].revive);
  f.pets.clear();f.monsters.clear();auto corpse=make_monster(*data,diablo_type,world.player.x,world.player.y,f.rng,0);corpse.hit_points=0;corpse.mode="DD";f.add_monster(corpse);f.summon(*revive,5,corpse.unit.x,corpse.unit.y,31000);CHECK(f.pets.empty()&&!f.monsters[0].corpse_used);
  f.monsters.clear();corpse=make_monster(*data,fallen_type,world.player.x,world.player.y,f.rng,0);corpse.hit_points=0;corpse.mode="DD";f.add_monster(corpse);f.summon(*revive,5,corpse.unit.x,corpse.unit.y,31000);CHECK(f.pets.size()==1&&f.monsters[0].corpse_used);f.pets.clear();
  // Revive preserves the source's actual level, damage and appearance.
  const auto old_level=c.stats.get(kLevel);f.pets.clear();f.monsters[0].corpse_used=false;f.monsters[0].stats.level=40;f.monsters[0].stats.a1_min=170;f.monsters[0].stats.a1_max=230;f.monsters[0].npc.name="Revive source";f.self_hurt=0;c.stats.values[kLevel]=40;const auto world_rng=f.rng;
  f.summon(*revive,5,corpse.unit.x,corpse.unit.y,31000);CHECK(f.pets.size()==1);const auto high=f.pets.back().monster.stats;CHECK(f.rng.low==world_rng.low&&f.rng.high==world_rng.high);CHECK(high.level==40&&high.a1_min>=170&&f.pets.back().monster.npc.name=="Revive source"&&f.pets.back().wake==31600&&f.self_hurt==c.stats.get(kMaxLife)/8);
  f.pets.clear();f.monsters[0].corpse_used=false;c.stats.values[kLevel]=10;f.summon(*revive,5,corpse.unit.x,corpse.unit.y,31000);CHECK(f.pets.size()==1);const auto low=f.pets.back().monster.stats;CHECK(low.level==10&&low.a1_min==high.a1_min&&std::abs(low.hit_points-high.hit_points/4)<=4);
  f.pets.clear();f.monsters[0].corpse_used=false;auto permanent=*revive;permanent.calc[1]=rule::compile_calc("0",data->skills.names);f.summon(permanent,5,corpse.unit.x,corpse.unit.y,31000);CHECK(f.pets.size()==1&&f.pets[0].until==0);f.pets.clear();c.stats.values[kLevel]=old_level;f.self_hurt=0;
  // Rebuilding collision stamps must discard pointers invalidated by pet growth/removal.
  f.monsters[0].corpse_used=false;f.summon(permanent,5,corpse.unit.x,corpse.unit.y,31000);
  Crowd stamps;stamps.units.push_back(&world.player);f.crowd(stamps);CHECK(stamps.dynamic_begin==1);
  f.pets.reserve(f.pets.capacity()+1);f.crowd(stamps);CHECK(stamps.units.back()==&f.pets.back().monster.unit);(void)stamps.at(world.player.x,world.player.y,&world.player);
  f.pets.clear();f.crowd(stamps);CHECK(stamps.units.size()==1);(void)stamps.at(world.player.x,world.player.y,&world.player);
  // Item Reanimate is independent of Revive caps/bonuses; waits for DD,
  // consumes once, preserves owner death safety and expires after 60 seconds.
  const int returned=data->monsters.row("skeleton2");CHECK(returned>=0);c.items={worn("cap",1,{{155,returned,100}})};c.merc_items.clear();f.refresh_item_effects();f.monsters[0].corpse_used=false;f.monsters[0].mode="DT";f.item_event(f.OnKill,0,32000);f.run_item_casts(32000);CHECK(f.pets.empty()&&f.item_reanimates.size()==1);
  f.monsters[0].mode="DD";f.run_item_casts(33000);CHECK(f.pets.size()==1&&f.pets[0].monster.type==returned&&f.monsters[0].corpse_used&&f.item_reanimates.empty()&&f.pets[0].until==93000&&f.pets[0].wake==34000);f.run_item_casts(33040);CHECK(f.pets.size()==1);CHECK(f.pet_foe(f.pets[0]).fighter.res[3]==data->monsters.types[std::size_t(returned)].diff[0].res[5]);
  f.pets[0].where=nullptr;f.pets_turn(93000,0,{});CHECK(!f.pets[0].monster.alive());f.pets.clear();
  for(auto boss:{rule::Boss::champion,rule::Boss::unique,rule::Boss::superunique}){f.monsters[0].corpse_used=false;f.monsters[0].boss=boss;f.item_event(f.OnKill,0,94000);f.run_item_casts(94000);CHECK(f.pets.empty()&&f.item_reanimates.empty()&&!f.monsters[0].corpse_used);}f.monsters[0].boss=rule::Boss::none;
  c.items.clear();c.merc_items={worn("cap",1,{{155,returned,100}})};f.refresh_item_effects();f.item_event(f.OnKill,0,95000,1);f.run_item_casts(95000);CHECK(f.pets.size()==1&&!f.pets[0].merc_owner&&f.monsters[0].corpse_used);f.pets.clear();c.merc_items.clear();f.refresh_item_effects();
  // End-to-end: the actual Tomb Reaver table row grants 10% Returned,
  // rather than a fabricated 100%-chance property used by focused cases.
  auto tomb=rule::generate_item(data->rules,"7pa",99,7,f.rng);CHECK(tomb.quality==7&&std::ranges::any_of(tomb.props,[&](const auto& p){return p.stat==155&&p.param==returned&&p.value==10;}));tomb.location=item_location::kEquipped;tomb.slot=4;c.items={tomb};f.dual_wield=false;f.active_attack_hand=0;f.refresh_item_effects();CHECK(std::ranges::any_of(f.item_procs[0],[&](const auto& p){return p.event==155&&p.skill==returned&&p.chance==10;}));
  int raised=0;for(int k=0;k<100;++k){f.monsters[0].corpse_used=false;f.monsters[0].boss=rule::Boss::none;f.monsters[0].id=10000+k;f.item_event(f.OnKill,0,120000+k*40);f.run_item_casts(120000+k*40);raised+=int(!f.pets.empty());f.pets.clear();}CHECK(raised>0&&raised<100);std::printf("PASS real Tomb Reaver Returned chance: %d / 100 deterministic kills\n",raised);c.items.clear();f.refresh_item_effects();
  // A mummy cloud lasts to Range, refreshes poison on contact and can
  // affect a second unit after the first leaves; poison length is not doubled.
  const auto& cloud_info=data->missiles.at("corpsepoisoncloud");CHECK(cloud_info.srv_do==3&&!cloud_info.collide_kill&&!cloud_info.next_hit);
  f.pending.clear();Monster mummy;mummy.unit.x=world.player.x;mummy.unit.y=world.player.y;mummy.stats.level=30;f.boss_missile(mummy,"corpsepoisoncloud",0,0,std::make_shared<std::vector<int>>(),100000,30);CHECK(f.pending.size()==1&&f.pending[0].row);const auto poison_length=f.pending[0].row->elen;const auto ends=f.pending[0].dies;
  std::vector<Missile> clouds=std::move(f.pending);std::array<Foe,2> victims{};for(auto& v:victims){v.x=mummy.unit.x;v.y=mummy.unit.y;v.size=2;}victims[1].x+=4;
  const auto update_cloud=[&](std::uint32_t now){missiles_update(*world.level,clouds,victims,f.rng,now,[](float,float){return std::vector<std::array<int,4>>{};},[](Missile&){return false;},[](int){return true;});};
  update_cloud(100000);CHECK(clouds.size()==1&&victims[0].poison>0&&victims[0].poison_ticks==poison_length);victims[0].poison=0;update_cloud(100040);CHECK(clouds.size()==1&&victims[0].poison>0);
  victims[0].x+=4;victims[1].x=mummy.unit.x;update_cloud(100080);CHECK(clouds.size()==1&&victims[1].poison>0);victims[1].poison=0;victims[1].fighter.res[3]=100;update_cloud(100120);CHECK(victims[1].poison==0&&clouds.size()==1);
  victims[1].fighter.res[3]=0;victims[1].fighter.plr=50;victims[1].poison_ticks=0;update_cloud(100160);CHECK(victims[1].poison_ticks==poison_length/2);update_cloud(ends);CHECK(clouds.empty());
  if(std::getenv("D2_MECHANIC_AUDIT")){
   for(const char* name:{"corpsepoisoncloud","poisoncloud"})if(const auto it=data->missiles.find(name);it!=data->missiles.end()){const auto& m=it->second;std::printf("MISSILE_AUDIT %s srv=%d hit=%d size=%d range=%d collidekill=%d nexthit=%d delay=%d poison=%d len=%d\n",name,m.srv_do,m.hit_func,m.size,m.range,m.collide_kill,m.next_hit,m.next_delay,m.etype,m.elen);}
  }
  // No living player may survive a zero-HP input tick or attack after death.
  c.stats.values[kLife]=0;f.pmode=-1;f.player_modes(true,30000,.04f);CHECK(f.dead());const auto killed_hp=f.monsters[0].hit_points;f.attack_mon=0;f.hit(30040);CHECK(f.monsters[0].hit_points==killed_hp);f.player_modes(true,50000,.04f);CHECK(f.dead());
  std::printf("PASS real Holy Fire weapon bonus only when active, equipment Meditation, free Amplify Damage on hit and Nova when struck (%zu missiles)\n",f.pending.size());
 }return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}
