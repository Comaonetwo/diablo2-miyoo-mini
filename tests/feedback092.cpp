// SPDX-License-Identifier: GPL-3.0-or-later
#include <gamedata_load.hpp>
#include <world.hpp>
#include <cstdio>
#include <stdexcept>
using namespace d2d::game;using namespace d2d::d2s;
#define CHECK(x) do{if(!(x))throw std::runtime_error("feedback092 line "+std::to_string(__LINE__)+": " #x);}while(0)
int main(int argc,char**argv)try{
 if(argc<2)return 0;
 setenv("D2D_SERIAL_LOADING","1",1);
 auto data=load_game_data(argv[1],std::filesystem::path(argv[1])/"LODPatch_114d.exe",3,false);CHECK(data);auto& d=*data;
 World w(&d,-1,-1);auto& c=w.character;c.character_class=4;c.stats.values[kLife]=c.stats.values[kMaxLife]=256000;
 Item potion;potion.code="hp1";potion.id=123;potion.uid=456;potion.location=item_location::kBelt;potion.column=12;c.items={potion};
 auto bytes=encode(cmd::ReturnBelt{123});auto command=decode(bytes);CHECK(command&&std::get<cmd::ReturnBelt>(*command).item==123);
 w.apply(*command,1000);CHECK(c.items.size()==1&&c.items[0].location==item_location::kStored&&c.items[0].panel==item_panel::kInventory&&c.items[0].uid==456&&!w.held);
 auto copy=c.items[0];w.apply(cmd::ReturnBelt{123},1040);CHECK(c.items.size()==1&&c.items[0].column==copy.column&&c.items[0].row==copy.row);
 c.items.clear();for(int y=0;y<4;++y)for(int x=0;x<10;++x){auto i=potion;i.id=200+y*10+x;i.location=item_location::kStored;i.panel=item_panel::kInventory;i.column=x;i.row=y;c.items.push_back(i);}c.items.push_back(potion);
 w.apply(cmd::ReturnBelt{123},1080);CHECK(c.items.size()==41&&c.items.back().location==item_location::kBelt&&!w.held);
 c.items={potion};w.held=copy;w.apply(cmd::ReturnBelt{123},1120);CHECK(c.items[0].location==item_location::kBelt&&w.held->uid==456);w.held.reset();
 c.stats.values[kLife]=0;w.fight.pmode=kModeDT;w.apply(cmd::ReturnBelt{123},1160);CHECK(c.items[0].location==item_location::kBelt);c.stats.values[kLife]=256000;w.fight.pmode=1;
 puts("PASS belt transfer: identity, protocol, upper row, repeated command, full inventory, held/dead guards");
 int doors=0;
 for(int id:{26,27}){auto* l=d.level(id);w.arrive(l,28,1.3f,"test");w.fight.rooms_up(*l,28,1.3f,true);
 for(const auto& n:l->npcs)if(World::is_door(n.operate_fn)&&n.object_frames[0]){const auto& timing=d.npc_timing(n,"NU");CHECK(timing.frames==std::min<unsigned>(n.object_frames[0],timing.cof_frames));CHECK(timing.frames==1);++doors;}}
 CHECK(doors>0);
 for(int id:{119,145,156,157,237,238,288,323,398})CHECK(d2d::rules::object_sound(id,1)=="object_waypoint_open");
 for(const char* s:{"object_waypoint_open","object_chest_large","object_chest_small","object_chest_special","object_burialchest","object_xwoodchest"}){auto i=d.sound_index.find(s);CHECK(i!=d.sound_index.end());auto wav=d.mpqs.try_read(std::string(R"(data\global\sfx\)")+d.sounds[i->second].file);if(!wav)wav=d.mpqs.try_read(std::string(R"(data\local\sfx\)")+d.sounds[i->second].file);if(!wav||wav->size()<=44)fprintf(stderr,"MISSING %s file=%s\n",s,d.sounds[i->second].file.c_str());CHECK(wav&&wav->size()>44);}
 // Exercise first ignition and repeated touch through the actual world interaction.
 auto& town=d.town;auto wi=std::ranges::find_if(town.npcs,[](const auto& n){return n.operate_fn==d2d::rules::operate_fn::kWaypoint;});CHECK(wi!=town.npcs.end());
 int index=int(wi-town.npcs.begin());const auto old_mode=wi->mode;wi->mode="NU";
 w.arrive(&town,wi->x,wi->y,"waypoint test");w.player.x=wi->x;w.player.y=wi->y;w.fight.pmode=-1;w.cues.due.clear();
 w.tick({cmd::Interact{index}},2000,1960);
 const auto ignition=d.sound_index.at("object_waypoint_open");
 CHECK(std::ranges::count_if(w.cues.due,[&](const auto& cue){return cue.sound==ignition;})==1);
 w.cues.due.clear();w.tick({cmd::Interact{index}},2040,2000);CHECK(std::ranges::none_of(w.cues.due,[&](const auto& cue){return cue.sound==ignition;}));
 wi->mode=old_mode;puts("PASS actual waypoint ignition once, repeated touch without duplicate cue");
 // Real object definitions in a bounded arena: sounds at start/end, exactly once.
 Level arena;arena.id=1;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(320*320);d2d::rules::Rng rng{3};
 for(int id:{5,87})add_object(d,d.builder->objects,d.builder->obj_row,arena,id,100,100,rng);
 CHECK(arena.npcs.size()==2);for(auto& n:arena.npcs){n.locked=false;n.trap=0;}
 w.arrive(&arena,21,20,"container test");w.fight.pmode=-1;
 unsigned now=4000;
 for(int n=0;n<2;++n){
  const auto& object=arena.npcs[n];const auto name=d2d::rules::object_sound(object.object_id,object.op_frames?1:2);
  w.cues.due.clear();w.operate(n,now,0);
  if(!name.empty())CHECK(std::ranges::count_if(w.cues.due,[&](const auto& cue){return cue.sound==d.sound_index.at(std::string(name));})==1);
  int end_count=0;const auto end_name=d2d::rules::object_sound(object.object_id,2);
  for(unsigned t=now+40;t<now+2000;t+=40){w.cues.due.clear();w.tick({},t,t-40);if(!end_name.empty())end_count+=int(std::ranges::count_if(w.cues.due,[&](const auto& cue){return cue.sound==d.sound_index.at(std::string(end_name));}));}
  CHECK(end_count==(!end_name.empty()&&object.op_frames?1:0));now+=2200;
 }
 puts("PASS common chest cues: original start/end phase, no repeated completion sound");
 CHECK(d2d::rules::object_sound(5,2)=="object_chest_large");CHECK(d2d::rules::object_sound(267,1).empty());
 printf("PASS %d door timing definitions and original waypoint/container sound IDs/files; stash has no invented cue\n",doors);
 return 0;
}catch(const std::exception&e){fprintf(stderr,"%s\n",e.what());return 1;}
