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
 check(!d2d::rules::refill_belt(items,"hp1",0,8));std::puts("PASS: exact grade, column capacity, stash exclusion, identities and counts");}
