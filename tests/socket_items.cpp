// SPDX-License-Identifier: GPL-3.0-or-later
#include <inventory.hpp>
#include <d2s_write.hpp>
#include <cstdio>
#include <stdexcept>
using namespace d2d::game;using namespace d2d::d2s;
#define CHECK(x) do{if(!(x))throw std::runtime_error("socket test line "+std::to_string(__LINE__));}while(false)
int main(){GameData d;auto& t=d.rules;t.types["gem"];t.types["gemy"].equiv[0]="gem";t.item_info["gcy"].type="gemy";t.item_info["cap"].type="helm";t.item_info["cap"].kind=1;t.item_info["cap"].width=2;t.item_info["cap"].height=2;t.item_info["hp1"].type="hpot";
 Item cap;cap.code="cap";cap.id=10;cap.uid=1234;cap.socketed=true;cap.sockets=1;cap.quality=2;cap.identified=true;cap.location=0;cap.panel=1;
 Item gem;gem.code="gcy";gem.id=11;gem.uid=5678;gem.simple=true;gem.identified=true;gem.location=4;
 std::vector<Item> items{cap};std::optional<Item> held=gem;
 CHECK(d2d::rules::insert_socket(t,items[0],held));CHECK(!held&&items.size()==1&&items[0].socketed_items.size()==1);CHECK(items[0].socketed_items[0].location==item_location::kSocketed);
 held=gem;CHECK(!d2d::rules::insert_socket(t,items[0],held));CHECK(held&&items[0].socketed_items.size()==1);
 Item wrong=gem;wrong.code="hp1";held=wrong;CHECK(!d2d::rules::insert_socket(t,items[0],held));CHECK(held->code=="hp1");
 cap.socketed=false;held=gem;CHECK(!d2d::rules::insert_socket(t,cap,held));CHECK(held);
 ItemTables tables;tables.stats.resize(512);for(auto& stat:tables.stats){stat.save_bits=16;stat.csv_bits=16;}tables.armor.insert("cap");Header h;h.name="SocketTest";h.level=1;Stats stats;stats.values[kLevel]=1;
 auto bytes=write_save({},h,stats,items,tables);auto loaded=parse_items(bytes,tables);CHECK(loaded.size()==1&&loaded[0].socketed_items.size()==1&&loaded[0].sockets==1);CHECK(loaded[0].uid==1234&&loaded[0].socketed_items[0].code=="gcy"&&loaded[0].socketed_items[0].location==item_location::kSocketed);
 puts("PASS: insertion, full/invalid socket rejection, identity and nested save/reload");}
