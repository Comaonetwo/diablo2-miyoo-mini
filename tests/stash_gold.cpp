// SPDX-License-Identifier: GPL-3.0-or-later
#include <gamedata_load.hpp>
#include <world.hpp>
#include <stash_gold.hpp>
#include <d2s_write.hpp>
#include <cstdio>
#include <stdexcept>
using namespace d2d::game;using namespace d2d::d2s;
#define CHECK(x) do{if(!(x))throw std::runtime_error("stash_gold line "+std::to_string(__LINE__)+": " #x);}while(0)
int main(int argc,char**argv)try{

 Stats s;s.values[kLevel]=10;s.values[kGold]=5000;s.values[kGoldBank]=0;
 CHECK(d2d::rules::transfer_stash_gold(s,2000,false)==2000&&s.get(kGold)==3000&&s.get(kGoldBank)==2000);
 CHECK(d2d::rules::transfer_stash_gold(s,500,true)==500&&s.get(kGold)==3500&&s.get(kGoldBank)==1500);
 auto total=s.get(kGold)+s.get(kGoldBank);CHECK(d2d::rules::transfer_stash_gold(s,-1,false)==0&&d2d::rules::transfer_stash_gold(s,0,true)==0&&s.get(kGold)+s.get(kGoldBank)==total);
 s.values[kGold]=100;s.values[kGoldBank]=2499990;CHECK(d2d::rules::transfer_stash_gold(s,1000000,false)==10&&s.get(kGoldBank)==2500000&&s.get(kGold)==90);
 s.values[kLevel]=1;s.values[kGold]=9990;s.values[kGoldBank]=100;CHECK(d2d::rules::transfer_stash_gold(s,1000,true)==10&&s.get(kGold)==10000&&s.get(kGoldBank)==90);
 CHECK(d2d::rules::transfer_stash_gold(s,1000,true)==0);s.values[kGold]=0;CHECK(d2d::rules::transfer_stash_gold(s,1000,false)==0);
 for(bool withdraw:{false,true}){const auto bytes=encode(cmd::StashGold{12345,withdraw});CHECK(bytes.size()==6);const auto decoded=decode(bytes);CHECK(decoded);const auto* cmd=std::get_if<cmd::StashGold>(&*decoded);CHECK(cmd&&cmd->amount==12345&&cmd->withdraw==withdraw);CHECK(!decode(std::span(bytes).first(4)));}
 if(argc<2){puts("PASS deposit/withdraw, gold caps, empty/negative transfers and wire format");return 0;}
 setenv("D2D_SERIAL_LOADING","1",1);auto d=load_game_data(argv[1],std::filesystem::path(argv[1])/"LODPatch_114d.exe",3,false);CHECK(d&&d->item_tables);
 Level a;a.id=1;a.ds1=d2d::ds1::Map(64,64,0,1);a.walk.resize(320*320);Npc stash;stash.root="objects";stash.operate_fn=32;stash.x=20;stash.y=20;a.npcs.push_back(stash);
 World w(&*d,-1,-1);w.character.character_class=0;w.character.stats.values[kLife]=w.character.stats.values[kMaxLife]=256000;w.character.stats.values[kLevel]=10;w.character.stats.values[kGold]=5000;w.arrive(&a,21,20,"stash transaction regression");w.cues.due.clear();
 w.apply(cmd::StashGold{2000,false},1000);CHECK(w.character.stats.get(kGold)==3000&&w.character.stats.get(kGoldBank)==2000&&w.cues.due.size()==1&&w.cues.due[0].sound==d->sound_index.at("item_gold"));
 w.apply(cmd::StashGold{500,true},1040);CHECK(w.character.stats.get(kGold)==3500&&w.character.stats.get(kGoldBank)==1500);
 const auto wallet=w.character.stats.values;w.player.x=30;w.apply(cmd::StashGold{1000,false},1080);CHECK(w.character.stats.values==wallet);
 w.player.x=21;w.held=Item{};w.apply(cmd::StashGold{1000,false},1120);CHECK(w.character.stats.values==wallet);w.held.reset();w.character.stats.values[kLife]=0;w.apply(cmd::StashGold{1000,false},1160);CHECK(w.character.stats.get(kGold)==3500&&w.character.stats.get(kGoldBank)==1500);
 Header h;h.name="GoldTest";h.cls=0;h.level=10;h.status=0x20;h.difficulty={0x80,0,0};h.appearance.fill(0xff);h.tints.fill(0xff);
 const auto saved=write_save({},h,w.character.stats,{},*d->item_tables);const auto read=parse_stats(saved,*d->item_tables);CHECK(read.get(kGold)==3500&&read.get(kGoldBank)==1500);
 puts("PASS deposit/withdraw, atomic caps/conservation, empty/negative, wire, proximity/dead/held guards, gold sound and save round trip");return 0;
}catch(const std::exception&e){fprintf(stderr,"%s\n",e.what());return 1;}
