// SPDX-License-Identifier: GPL-3.0-or-later
#include <gamedata_load.hpp>
#include <world.hpp>
#include <cstdio>
#include <stdexcept>
using namespace d2d::game;using namespace d2d::d2s;
#define CHECK(x) do{if(!(x))throw std::runtime_error("feedback093 line "+std::to_string(__LINE__)+": " #x);}while(0)
int main(int argc,char**argv)try{
 if(argc<2)return 0;setenv("D2D_SERIAL_LOADING","1",1);auto data=load_game_data(argv[1],std::filesystem::path(argv[1])/"LODPatch_114d.exe",3,false);CHECK(data);auto& d=*data;
 int tested=0;
 for(int id:{1,3,4,7,46,50,51,79,89,284,365,208,209,360,362}){
  Level arena;arena.id=1;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(320*320);d2d::rules::Rng rng{3};add_object(d,d.builder->objects,d.builder->obj_row,arena,id,100,100,rng);CHECK(arena.npcs.size()==1);auto& n=arena.npcs[0];n.locked=false;n.trap=0;
  const auto op=d.npc_timing(n,"OP"),on=d.npc_timing(n,"ON"),nu=d.npc_timing(n,"NU");
  CHECK(nu.first_frame==n.object_starts[0]&&on.first_frame==n.object_starts[2]);CHECK(on.first_frame+on.frames<=on.cof_frames);
  if(id==7)CHECK(on.first_frame==9&&on.frames==1&&op.length_ms()==640);
  if(id==50)CHECK(op.first_frame==1&&op.frames==4&&on.first_frame==4&&op.length_ms()==512);
  World w(&d,-1,-1);w.character.character_class=4;w.character.stats.values[kLife]=w.character.stats.values[kMaxLife]=256000;w.arrive(&arena,21,20,"object test");w.operate(0,1000,0);
  // Some caskets intentionally fail to open on a no-drop round; force a successful round with bounded new object seeds.
  for(int retry=0;!w.operated.contains({&arena,0})&&retry<64;++retry)w.operate(0,1000,0);
  CHECK(w.operated.contains({&arena,0}));CHECK(w.npc_states[0].mode=="OP"&&w.npc_states[0].mode_ms==1000);
  const int until=int(op.length_ms());w.tick({},1000+unsigned(until)-1,1000);CHECK(w.npc_states[0].mode=="OP");w.tick({},1000+unsigned(until)+40,1000+unsigned(until)-1);CHECK(w.npc_states[0].mode=="ON");CHECK(w.npc_states[0].mode_ms==1000+op.length_ms());
  auto size=w.loot.ground.size();w.tick({},3000,2000);CHECK(w.loot.ground.size()==size);++tested;
 }
 printf("PASS %d barrel/crate/casket/urn/basket/rockpile modes: original Start+FrameCnt, complete playback, final frame and timestamp\n",tested);
 World w(&d,-1,-1);w.character.character_class=4;w.character.stats.values[kLife]=w.character.stats.values[kMaxLife]=256000;
 Item axe;axe.code="hax";axe.id=1;axe.location=item_location::kStored;axe.panel=item_panel::kInventory;w.character.items={axe};w.apply(cmd::ToCursor{1},4000);CHECK(w.held&&w.cues.due.size()==1&&w.cues.due[0].sound==d.sound_index.at(d.rules.item_info.at("hax").use_sound));
 w.cues.due.clear();w.apply(cmd::ToCursor{1},4040);CHECK(w.cues.due.empty());w.apply(cmd::ToGrid{item_panel::kInventory,0,0},4080);CHECK(!w.held&&w.cues.due.size()==1);puts("PASS item material cue on successful move, no failed-command cue");
 auto shaman=make_monster(d,d.monsters.row("fallenshaman1"),20,20,w.fight.rng,0);shaman.mode="A2";shaman.skill=d.skills.by_name.at("Resurrect");shaman.unit.mode_ms=5000;w.cues.due.clear();w.fight.monster_sounds(shaman,5000);CHECK(w.cues.due.size()==1&&w.cues.due[0].sound==d.sound_index.at("fallenshaman_resurrect_cast"));w.cues.due.clear();w.fight.monster_sounds(shaman,5040);CHECK(w.cues.due.empty());
 CHECK(d.skills.get(d.skills.by_name.at("Corpse Explosion"))->target_sound==d.sound_index.at("necromancer_corpseexp_1"));CHECK(d.states.at("wolf").off_sound=="druid_morph");CHECK(d.states.at("poison").on_sound=="impact_poison_1");CHECK(d.mon_sounds.at("charsi").skill[0]==d.sound_index.at("charsi_work"));
 puts("PASS original monster skill-mode conversion, one event per animation, target/state/NPC work sound metadata");
 auto corpse=make_monster(d,d.monsters.row("zombie1"),20,20,w.fight.rng,0);corpse.hit_points=0;corpse.mode="DD";w.fight.monsters={corpse};
 auto explosion=w.fight.skill_named("Corpse Explosion");CHECK(explosion);w.cues.due.clear();w.fight.corpse_blast(*explosion,5,0,6000);CHECK(std::ranges::any_of(w.cues.due,[&](const auto& cue){return cue.sound==explosion->target_sound;}));
 auto hammer=w.fight.skill_named("Psychic Hammer");CHECK(hammer);w.fight.monsters[0].hit_points=1000;w.fight.monsters[0].corpse_used=false;w.fight.monsters[0].mode="NU";
 w.cues.due.clear();w.fight.spot(*hammer,6040,{20,20,5,0,20,20,false});CHECK(std::ranges::any_of(w.cues.due,[&](const auto& cue){return cue.sound==hammer->target_sound;}));
 w.cues.due.clear();w.fight.spot(*hammer,6080,{20,20,5,-1,20,20,false});CHECK(std::ranges::none_of(w.cues.due,[&](const auto& cue){return cue.sound==hammer->target_sound;}));
 puts("PASS actual Corpse Explosion and Psychic Hammer target cues; invalid hammer target remains silent");return 0;
}catch(const std::exception& e){fprintf(stderr,"%s\n",e.what());return 1;}
