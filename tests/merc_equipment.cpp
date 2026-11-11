// SPDX-License-Identifier: GPL-3.0-or-later
#include <merc_inventory.hpp>
#include <protocol.hpp>
#include <d2s_write.hpp>
#include <cstdio>
#include <stdexcept>
using namespace d2d::game;
using namespace d2d::d2s;
#define CHECK(x) do{if(!(x))throw std::runtime_error("merc test line "+std::to_string(__LINE__));}while(false)
int main(){
 GameData data;
 for(int act:{1,2,3,5}){d2d::rules::Hireling h;h.id=act;h.act=act;h.version=100;h.level=1;h.exp_per_level=1;h.str=h.dex=60;data.rules.hirelings.push_back(h);}
 auto add=[&](const char* code,const char* type,int kind,int slot,int other=0){auto& t=data.rules.types[type];t.body={slot,other};auto& i=data.rules.item_info[code];i.type=type;i.kind=kind;i.req_lvl=1;};
 add("cap","helm",1,body_location::kHead);add("bhl","phlm",1,body_location::kHead);data.rules.types["phlm"].cls="bar";
 add("qui","tors",1,body_location::kTorso);add("swd","swor",2,body_location::kRightArm,body_location::kLeftArm);
 add("bow","bow",2,body_location::kRightArm);add("xbw","xbow",2,body_location::kRightArm);
 add("spe","spea",2,body_location::kRightArm);add("pol","pole",2,body_location::kRightArm);add("jav","jave",2,body_location::kRightArm);
 add("buc","shld",1,body_location::kLeftArm);add("abow","abow",2,body_location::kRightArm);data.rules.types["abow"].equiv[0]="bow";data.rules.types["abow"].cls="ama";
 const auto item=[](const char* code,int id=1){Item x;x.code=code;x.id=id;x.quality=2;x.identified=true;return x;};
 Header h;h.status=0x20;h.merc_seed=123;std::vector<Item> worn;
 for(auto [act,good,bad]:{std::tuple{1,"bow","xbw"},std::tuple{2,"pol","swd"},std::tuple{3,"swd","bow"},std::tuple{5,"swd","pol"}}){
  h.merc_type=act;CHECK(merc_can_equip(data,h,worn,item(good),body_location::kRightArm)==MercItemResult::Ok);
  CHECK(merc_can_equip(data,h,worn,item(bad),body_location::kRightArm)==MercItemResult::Type);
  CHECK(merc_can_equip(data,h,worn,item("cap"),body_location::kHead)==MercItemResult::Ok);
  CHECK(merc_can_equip(data,h,worn,item("qui"),body_location::kTorso)==MercItemResult::Ok);
 }
 h.merc_type=1;CHECK(merc_can_equip(data,h,worn,item("abow"),4)==MercItemResult::Type);
 h.merc_type=5;CHECK(merc_can_equip(data,h,worn,item("bhl"),1)==MercItemResult::Ok);
 h.merc_type=2;CHECK(merc_can_equip(data,h,worn,item("bhl"),1)==MercItemResult::Type);
 for(const char* code:{"spe","jav"})CHECK(merc_can_equip(data,h,worn,item(code),4)==MercItemResult::Ok);
 h.merc_type=3;CHECK(merc_can_equip(data,h,worn,item("buc"),5)==MercItemResult::Ok);
 data.rules.item_info["swd"].two_handed=true;CHECK(merc_can_equip(data,h,worn,item("swd"),4)==MercItemResult::Type);data.rules.item_info["swd"].two_handed=false;
 h.merc_type=2;auto cap=item("cap",11);data.rules.item_info["cap"].req_str=65;
 CHECK(merc_can_equip(data,h,worn,cap,1)==MercItemResult::Strength);cap.ethereal=true;CHECK(merc_can_equip(data,h,worn,cap,1)==MercItemResult::Ok);
 cap.ethereal=false;cap.props.push_back({kRequirementPercent,0,-50});CHECK(merc_can_equip(data,h,worn,cap,1)==MercItemResult::Ok);
 cap.props.clear();data.rules.item_info["cap"].req_str=0;cap.quality=7;cap.unique_id=0;data.rules.unique_req={99};CHECK(merc_can_equip(data,h,worn,cap,1)==MercItemResult::Level);
 cap.quality=4;cap.identified=false;CHECK(merc_can_equip(data,h,worn,cap,1)==MercItemResult::Identify);cap=item("cap",11);
 std::optional<Item> held=cap;CHECK(merc_item_move(data,h,worn,held,1)==MercItemResult::Ok&&!held&&worn.size()==1&&worn[0].id==11);
 held=item("cap",12);CHECK(merc_item_move(data,h,worn,held,1)==MercItemResult::Ok&&held->id==11&&worn[0].id==12); // no inventory space needed for swap
 CHECK(merc_item_move(data,h,worn,held,5)==MercItemResult::Slot&&held->id==11&&worn[0].id==12);
 h.merc_dead=true;CHECK(merc_item_move(data,h,worn,held,1)==MercItemResult::Dead&&held->id==11&&worn[0].id==12);h.merc_dead=false;
 h.merc_seed=0;CHECK(merc_item_move(data,h,worn,held,1)==MercItemResult::NoMerc);h.merc_seed=123;
 held.reset();CHECK(merc_item_move(data,h,worn,held,1)==MercItemResult::Ok&&held->id==12&&worn.empty());
 // Existing item in the target slot cannot supply its own replacement's requirements.
 auto strength=item("cap",20);strength.props.push_back({kStr,0,50});strength.location=item_location::kEquipped;strength.slot=1;worn={strength};data.rules.item_info["cap"].req_str=100;
 CHECK(merc_can_equip(data,h,worn,item("cap",21),1)==MercItemResult::Strength);
 for(int slot:{0,1,3,4,5,99}){Command c=cmd::MercItem{slot};auto back=decode(encode(c));CHECK(back&&std::get<cmd::MercItem>(*back).slot==slot);}
 // A full inventory saves the swapped item on the cursor; restore it on load.
 data.rules.item_info["hp1"].width=data.rules.item_info["hp1"].height=1;
 data.rules.item_info["cap"].width=data.rules.item_info["cap"].height=2;
 std::vector<Item> full;for(int y=0;y<4;++y)for(int x=0;x<10;++x){auto p=item("hp1",100+y*10+x);p.location=item_location::kStored;p.panel=item_panel::kInventory;p.column=x;p.row=y;full.push_back(p);}
 held=item("cap",999);stow_held(data,full,held);CHECK(!held&&full.size()==41&&full.back().location==item_location::kCursor);
 restore_saved_cursor(full,held);CHECK(held&&held->id==999&&full.size()==40);
 // Save rewrite: live merc gear replaces jf, corpse and opaque golem tail survive.
 ItemTables tables;tables.stats.resize(512);for(auto& stat:tables.stats){stat.save_bits=16;stat.csv_bits=16;}
 tables.armor.insert("cap");Header saved=h;saved.name="MercTest";saved.level=1;
 Stats stats;stats.values[kLevel]=1;Item gear=item("cap");gear.location=item_location::kEquipped;gear.slot=1;gear.uid=0x12345678;gear.defense=17;gear.props={{kStr,0,7}};
 std::vector<Item> list{gear},corpse{item("cap")};corpse[0].location=item_location::kEquipped;corpse[0].slot=1;
 auto bytes=write_save({},saved,stats,{},tables,&corpse,&list);auto parsed=parse_merc_items(bytes,tables);
 CHECK(parsed.size()==1&&parsed[0].uid==gear.uid&&parsed[0].slot==1&&parsed[0].props[0].value==7);
 CHECK(parse_corpse(bytes,tables).items.size()==1);
 const std::array<std::byte,3> opaque{std::byte{0xa1},std::byte{0xb2},std::byte{0xc3}};bytes.insert(bytes.end(),opaque.begin(),opaque.end());
 gear.uid=0x87654321;gear.slot=3;list={gear};auto rewritten=write_save(bytes,saved,stats,{},tables,nullptr,&list);parsed=parse_merc_items(rewritten,tables);
 CHECK(parsed.size()==1&&parsed[0].uid==gear.uid&&parsed[0].slot==3&&parsed[0].props[0].value==7&&parse_corpse(rewritten,tables).items.size()==1);
 CHECK(std::equal(opaque.begin(),opaque.end(),rewritten.end()-3));
 list.clear();auto cleared=write_save(rewritten,saved,stats,{},tables,nullptr,&list);CHECK(parse_merc_items(cleared,tables).empty()&&parse_corpse(cleared,tables).items.size()==1);
 CHECK(std::equal(opaque.begin(),opaque.end(),cleared.end()-3));
 saved.merc_seed=0;auto retired=write_save(rewritten,saved,stats,{},tables,nullptr,&list);CHECK(parse_merc_items(retired,tables).empty());
 std::puts("PASS: mercenary families, slots, requirements, safe swap/rejection command codec and save/corpse/golem-tail rewrite");
}
