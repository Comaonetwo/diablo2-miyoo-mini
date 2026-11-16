// SPDX-License-Identifier: GPL-3.0-or-later
#include <rules.hpp>
#include <stdexcept>
#include <cstdio>
using namespace d2d::d2s;
void check(bool b){if(!b)throw std::runtime_error("belt refill invariant");}
Item item(int id,const char* code,int location,int panel,int box){Item i;i.id=id;i.code=code;i.location=location;i.panel=panel;i.column=box;return i;}
int main(){
 std::vector<Item> items{item(1,"hp1",item_location::kStored,item_panel::kStash,0),item(2,"hp2",item_location::kStored,item_panel::kInventory,0),item(3,"hp1",item_location::kStored,item_panel::kInventory,2),item(4,"mp1",item_location::kBelt,0,1)};
 check(d2d::rules::refill_belt(items,"hp1",0,4));check(items.size()==4&&items[2].id==3&&items[2].location==item_location::kBelt&&items[2].column==0);check(items[0].location==item_location::kStored&&items[1].location==item_location::kStored);
 check(!d2d::rules::refill_belt(items,"hp1",0,4));check(!d2d::rules::refill_belt(items,"mp1",1,4));
 items.push_back(item(5,"hp1",item_location::kStored,item_panel::kInventory,4));
 check(d2d::rules::refill_belt(items,"hp1",0,8));check(items.back().id==5&&items.back().column==4);check(items.size()==5);
 check(!d2d::rules::refill_belt(items,"hp1",0,8));
 items={item(30,"hp1",item_location::kBelt,0,7),item(31,"mp1",item_location::kBelt,0,0),item(32,"hp1",item_location::kBelt,0,3),item(33,"hp1",item_location::kBelt,0,4)};
 const auto pick=[&](bool left){return d2d::rules::sequential_belt_item(items,left);};
 check(pick(true)==31&&pick(false)==32); // shuffled storage order, bottom before upper
 d2d::rules::Tables tables;tables.potions["hp1"]={30,0,192};tables.potions["mp1"]={0,20,128};
 check(d2d::rules::drink_item(tables,items,pick(false))=="hp1");check(pick(false)==30&&pick(true)==31);
 check(d2d::rules::drink_item(tables,items,pick(true))=="mp1");check(pick(true)==33&&pick(false)==30);
 // An empty lower box must not hide an upper-row item, nor change the edge.
 items={item(40,"hp1",item_location::kBelt,0,12),item(41,"mp1",item_location::kBelt,0,6),item(42,"hp1",item_location::kStored,item_panel::kInventory,0)};
 check(pick(true)==40&&pick(false)==41);
 items.clear();check(pick(true)==-1&&pick(false)==-1);
 std::puts("PASS: deterministic left/right edges, alternating controls, upper rows, bottom-first, shuffled storage, drink/shift and empty belt");
 std::puts("PASS: exact grade, column capacity, stash exclusion, identities and counts");}
