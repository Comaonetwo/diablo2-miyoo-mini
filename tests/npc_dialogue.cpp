// SPDX-License-Identifier: GPL-3.0-or-later
#include <gamedata_load.hpp>
#include <world.hpp>
#include <cstdio>
#include <stdexcept>
using namespace d2d::game;
#define CHECK(x) do{if(!(x))throw std::runtime_error("npc_dialogue line "+std::to_string(__LINE__)+": " #x);}while(0)
int main(int argc,char**argv)try{
 for(bool fresh:{false,true})for(bool select:{false,true}){
  const auto encoded=d2d::game::encode(cmd::Move{20.5f,13.25f,fresh,select});CHECK(encoded.size()==10);
  if(select)CHECK(encoded.back()==unsigned(fresh)); // existing mouse packets unchanged
  auto decoded=d2d::game::decode(encoded);CHECK(decoded);const auto* move=std::get_if<cmd::Move>(&*decoded);CHECK(move&&move->x==20.5f&&move->y==13.25f&&move->fresh==fresh&&move->select_warp==select);
 }
 if(argc<2)return 0;setenv("D2D_SERIAL_LOADING","1",1);auto d=load_game_data(argv[1],std::filesystem::path(argv[1])/"LODPatch_114d.exe",3,false);CHECK(d);
 Level a;a.id=40;a.ds1=d2d::ds1::Map(64,64,0,1);a.walk.resize(320*320);
 auto greiz=std::ranges::find(d->mon_npc,198,&Npc::hc_idx);CHECK(greiz!=d->mon_npc.end());auto n=*greiz;n.x=20;n.y=20;n.path.clear();a.npcs.push_back(n);a.warps.push_back({20,20,47});
 World w(&*d,-1,-1);w.character.character_class=0;w.character.stats.values[d2d::d2s::kLife]=w.character.stats.values[d2d::d2s::kMaxLife]=256000;w.arrive(&a,21,20,"NPC beside entrance regression");w.events.clear();
 w.apply(cmd::Move{20,20,true},1000);CHECK(w.take_warp==0);
 w.apply(cmd::Interact{0},1040);CHECK(w.take_warp==-1);CHECK(w.interact_npc==0);
 w.tick({},1080,1040);CHECK(w.level==&a);CHECK(std::ranges::any_of(w.events,[](const auto& e){const auto* ui=std::get_if<ev::OpenUI>(&e);return ui&&ui->kind==ev::OpenUI::talk&&ui->npc==0;}));
 w.apply(cmd::Move{20,20,true},1120);CHECK(w.take_warp==0);
 w.loot.put({.code="hp1"},20,20,1,w.fight.spawning.game,1160);CHECK(!w.loot.ground.empty());auto& drop=w.loot.ground.back();drop.x=drop.y=20;const int drop_id=drop.id;
 w.apply(cmd::Pickup{drop_id},1160);CHECK(w.take_warp==-1&&w.pick_item==drop_id);
 w.portal[0]={&a,20,20,1200};w.apply(cmd::Interact{-2000},1200);CHECK(w.take_warp==-1&&w.take_portal==0);
 w.corpses.push_back({.level=&a,.x=20,.y=20});w.apply(cmd::Interact{-3000},1240);CHECK(w.take_warp==-1&&w.take_corpse==0);
 w.apply(cmd::Move{20,20,true,false},1280);CHECK(w.take_warp==-1&&w.take_portal==-1&&w.take_corpse==-1);
 w.apply(cmd::Move{w.player.x,w.player.y,true,false},1320);CHECK(w.take_warp==-1);
 w.apply(cmd::Move{20,20,true},1360);CHECK(w.take_warp==0);
 puts("PASS controller steering/stop never enters implicitly; move flags survive wire round trip; original click packets unchanged");
 puts("PASS loot, portal and body beside entrance never select its warp");
 puts("PASS NPC beside entrance opens conversation; explicit walk-to-entrance remains enabled");return 0;
}catch(const std::exception&e){fprintf(stderr,"%s\n",e.what());return 1;}
