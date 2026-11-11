// SPDX-License-Identifier: GPL-3.0-or-later
#include <world.hpp>
#include <cube.hpp>
#include <d2s_write.hpp>
#include <gamedata_load.hpp>
#include <item_text.hpp>
#include <cstdio>
#include <chrono>
#include <algorithm>
#include <stdexcept>
using namespace d2d::game;using namespace d2d::d2s;namespace rule=d2d::rules;
#define CHECK(x) do{if(!(x))throw std::runtime_error("item mechanics line "+std::to_string(__LINE__));}while(false)
static Item item(const char* code){Item i;i.code=code;i.quality=2;i.identified=true;i.uid=123;i.location=item_location::kEquipped;i.slot=4;return i;}
static void rules_test(){
 GameData d;auto& t=d.rules;t.types["swor"].equiv[0]="weap";t.types["swor"].body={4,5};t.types["rune"];t.types["tors"].body={3,0};t.types["weap"];
 t.item_info["swd"].type="swor";t.item_info["swd"].kind=2;t.item_info["swd"].req_str=100;t.item_info["swd"].req_dex=50;t.item_info["swd"].req_lvl=1;
 t.item_info["r01"].type=t.item_info["r02"].type="rune";t.item_base["swd"];t.properties["fire"]={{1,kFireResist,0}};
 t.runewords.push_back({27,{"r01","r02"},{"weap"},{"tors"},{{"fire","",15,15}}});
 Item sword=item("swd");sword.socketed=true;sword.sockets=2;sword.props={{kStr,0,5}};
 Item first=item("r01"),second=item("r02");first.simple=second.simple=true;
 auto build=[&](Item base,bool reverse){std::optional<Item> held=reverse?second:first;CHECK(rule::insert_socket(t,base,held)&&!held&&!base.runeword);held=reverse?first:second;CHECK(rule::insert_socket(t,base,held)&&!held);return base;};
 auto word=build(sword,false);CHECK(word.runeword&&word.runeword_id==27&&word.main_props==1&&word.props.size()==2&&word.props[1].value==15&&word.socketed_items.size()==2);
 CHECK(!build(sword,true).runeword);auto magic=sword;magic.quality=4;CHECK(!build(magic,false).runeword);auto extra=sword;extra.sockets=3;CHECK(!build(extra,false).runeword);
 auto wrong=sword;wrong.code="qui";t.item_info["qui"].type="tors";t.item_info["qui"].kind=1;CHECK(!build(wrong,false).runeword);
 rule::Rng rng{123};auto before=rng.low;CHECK(!rule::make_runeword(t,word,rng)&&rng.low==before&&word.props.size()==2);
 ItemTables save;save.weapons.insert("swd");save.stats.resize(512);for(auto& s:save.stats){s.save_bits=16;s.csv_bits=16;}Header h;h.name="ItemTest";h.level=30;Stats stats;stats.values[kLevel]=30;
 auto loaded=parse_items(write_save({},h,stats,{word},save),save);CHECK(loaded.size()==1&&loaded[0].runeword&&loaded[0].runeword_id==27&&loaded[0].main_props==1&&loaded[0].props.size()==2&&loaded[0].socketed_items.size()==2);
 rule::CubeRecipe recipe;recipe.outputs[0].kind=rule::cube_out::kUseItem;recipe.outputs[0].flags=rule::cube_out::kUnsocket;rule::CubeMatch match;match.source[0]=0;
 auto cleaned=rule::cube_make(t,recipe,match,{&loaded[0]},30,true,0,rng,nullptr);CHECK(cleaned.made&&cleaned.items.size()==1&&!cleaned.items[0].runeword&&cleaned.items[0].socketed_items.empty()&&cleaned.items[0].props.size()==1);
 auto eth=sword;eth.ethereal=true;eth.props={{kRequirementPercent,0,-20}};CHECK(rule::required_attribute(t,eth,100)==70&&rule::required_attribute(t,eth,50)==30);
 CHECK(rule::can_wear(t,eth,4,{4,70,30,40}));CHECK(!rule::can_wear(t,eth,4,{4,69,30,40}));
 t.unique_req={45};eth.quality=7;eth.unique_id=0;CHECK(!rule::can_wear(t,eth,4,{4,70,30,44}));CHECK(rule::can_wear(t,eth,4,{4,70,30,45}));
 auto rune=item("r01");t.item_info["r01"].req_lvl=55;eth.socketed_items={rune};CHECK(rule::required_level(t,eth)==55);
 t.gem_props["r01"][0]={{kRequirementPercent,0,-20}};CHECK(rule::required_attribute(t,eth,100)==50);
 std::puts("PASS: rune order/type/quality/socket count, one-time bonus, save lists, unsocket cleanup, shared effective requirements");
}
static void recovery_rules_test(){
 rule::Tables t;t.item_base["jav"].stackable=true;t.item_base["jav"].max_stack=80;t.item_base["jav"].cost=10000;t.item_info["jav"].kind=2;t.item_info["jav"].type="thro";t.types["thro"].autostack=true;t.types["thro"].throwable=t.types["thro"].repairable=true;t.item_info["rin"].width=t.item_info["rin"].height=1;
 Item jav=item("jav");jav.id=1;jav.quantity=79;jav.props={{254,0,60}};CHECK(rule::total_max_stack(t,jav)==140);auto more=jav;more.props[0].value=1000;CHECK(rule::total_max_stack(t,more)==511);more.props[0].value=-1000;CHECK(rule::total_max_stack(t,more)==1);
 std::vector<Item> bag{jav};bag[0].location=item_location::kStored;bag[0].panel=item_panel::kInventory;auto source=jav;source.quantity=100;std::optional<Item> held=source;CHECK(rule::put_in_grid(t,bag,held,item_panel::kInventory,10,4,0,0)&&bag[0].quantity==140&&held&&held->quantity==39);
 bag={jav};bag[0].quantity=139;rule::Store shop;shop.tab=0;shop.perm={"jav"};auto offer=jav;offer.quantity=20;shop.tabs[0]={offer};Stats stats;stats.values[kGold]=1000000;CHECK(rule::store_fill(t,shop,0,bag,stats,4)==1&&bag.size()==1&&bag[0].quantity==140);
 bag={jav};source.quantity=2;CHECK(rule::pick_up(t,bag,source,10,4,4)==rule::Pickup::kGone&&bag[0].quantity==81&&bag.size()==1);
 jav.quantity=100;CHECK(rule::store_repair(t,shop,jav,stats)&&jav.quantity==140);jav.quantity=0;jav.props.push_back({253,0,100});CHECK(rule::repair_cost(t,jav,"",shop.header)==1&&rule::store_repair(t,shop,jav,stats)&&jav.quantity==140);jav.quantity=0;jav.ethereal=true;CHECK(rule::repair_cost(t,jav,"",shop.header)==0&&!rule::store_repair(t,shop,jav,stats)&&jav.quantity==0);
 rule::ItemRecovery recovery;jav.flags|=0x100;jav.max_durability=20;jav.durability=0;bag={jav};
 const auto step=[&](std::uint32_t now){recovery.begin();for(auto& i:bag)recovery.update(t,i,now);recovery.end();};
 step(0);step(1039);CHECK(bag[0].quantity==0);step(1040);CHECK(bag[0].quantity==1&&!rule::broken(bag[0])&&bag[0].durability==20);
 step(6039);CHECK(bag[0].quantity==1);step(6040);CHECK(bag[0].quantity==2);step(100000);CHECK(bag[0].quantity==3);step(100000);CHECK(bag[0].quantity==3); // no catch-up burst
 bag[0].quantity=139;step(105000);CHECK(bag[0].quantity==140);step(110000);CHECK(bag[0].quantity==140&&recovery.clocks.empty());
 auto armor=item("arm");armor.id=2;armor.slot=3;armor.ethereal=true;armor.max_durability=40;armor.durability=38;armor.props={{252,0,4}};bag={armor};step(120000);step(145039);CHECK(bag[0].durability==38);step(145040);CHECK(bag[0].durability==39);step(170080);CHECK(bag[0].durability==40);step(200000);CHECK(recovery.clocks.empty());
 bag[0].durability=0;bag[0].flags|=0x100;step(300000);step(500000);CHECK(bag[0].durability==0&&rule::broken(bag[0]));bag.clear();step(500040);CHECK(recovery.clocks.empty());
 // Rate changes restart the clock, independent identities do not borrow time.
 recovery={};jav.quantity=1;jav.id=10;jav.flags=0;bag={jav};step(0);bag[0].props.back().value=20;step(500);step(5539);CHECK(bag[0].quantity==1);step(5540);CHECK(bag[0].quantity==2);bag[0].id=11;step(6000);step(11039);CHECK(bag[0].quantity==2);step(11040);CHECK(bag[0].quantity==3&&recovery.clocks.size()==1);
 std::puts("PASS item recovery rules: extra-stack caps, grid/pickup/shop/repair consistency, exact first/repeat delays, empty-stack reactivation, ethereal self-repair, no broken armor repair, rate/identity cleanup, no stall burst");
}
static void stone_skin_test(){
 rule::MonType type;type.diff[2].res={0,0,100,0,100,0};rule::UMods mods;
 const auto blocked=rule::boss_stats(mods,type,rule::Boss::unique,{rule::umod::stoneskin},2);CHECK(blocked.double_defense&&blocked.res_add[0]==0);
 type.diff[2].res={0,0,100,0,0,0};const auto allowed=rule::boss_stats(mods,type,rule::Boss::unique,{rule::umod::stoneskin},2);CHECK(allowed.double_defense&&allowed.res_add[0]==50);
 std::puts("PASS Stone Skin: defense doubles even when two preexisting immunities prevent another resistance bonus");
}
static void specialized_price_test(){
 rule::Tables t;t.item_base["staff"].cost=1024;t.item_info["staff"].type="staf";t.item_info["staff"].kind=2;t.types["staf"].staff_mods=1;
 t.skill_recharge.resize(60);t.skill_recharge[54]={1024,100};t.stat_prices.resize(359);t.stat_prices[kSingleSkill]={0,0,1};t.stat_prices[kChargedSkill]={0,0,3};t.stat_prices[198]={0,0,2};t.stat_prices[kMaxLife]={1024,10,0};
 auto wand=item("staff");wand.props={{kSingleSkill,54,3}};
 CHECK(rule::item_value(t,wand,rule::PriceKind::Buy)==6644);CHECK(rule::item_value(t,wand,rule::PriceKind::Sell)==2804);CHECK(rule::item_value(t,wand,rule::PriceKind::Repair)==6644);
 // Merge equal skill layers before the (2*level-1) staffmod multiplier.
 wand.props={{kSingleSkill,54,1},{kSingleSkill,54,2}};CHECK(rule::item_value(t,wand)==6644);
 t.prefix_cost={{1024,0}};t.automagic_cost={{512,10}};wand.quality=4;wand.prefix=0;wand.suffix=-1;wand.class_affix=1;
 // Bonus stats apply before affix extras, staffmods after both; sale has
 // its own /4096 skill scale. AutoMagic contributes once, too.
 CHECK(rule::item_value(t,wand)==34952);CHECK(rule::item_value(t,wand,rule::PriceKind::Sell)==8233);
 wand.identified=false;CHECK(rule::item_value(t,wand)==1024&&rule::item_value(t,wand,rule::PriceKind::Sell)==1024);wand.identified=true;
 wand.props={{kChargedSkill,(54<<6)|3,(20<<8)|20}};wand.prefix=-1;wand.class_affix=-1;
 CHECK(rule::item_value(t,wand)==4196&&rule::item_value(t,wand,rule::PriceKind::Sell)==1892);wand.props[0].value=(20<<8);CHECK(rule::item_value(t,wand)==4196); // charge level, not charges remaining
 wand.props={{198,(54<<6)|5,1}};CHECK(rule::item_value(t,wand)==6244);wand.props[0].value=100;CHECK(rule::item_value(t,wand)==6244); // proc level, not percent
 wand.props={{kMaxLife,0,20}};CHECK(rule::item_value(t,wand)==21514); // saved life value is already unshifted
 wand.props.clear();wand.quality=3;wand.socketed_items={item("jew")};wand.socketed_items[0].props={{kMaxLife,0,20}};CHECK(rule::item_value(t,wand)==21514);
 // Unique and set metadata already price their generic stats; do not add
 // the generic stat surcharge a second time, but staffmods still apply.
 wand.socketed_items.clear();wand.props={{kMaxLife,0,20}};wand.quality=7;wand.unique_id=0;t.unique_cost={{1024,0}};CHECK(rule::item_value(t,wand)==2048);
 wand.quality=5;wand.set_id=0;t.set_cost={{512,0}};CHECK(rule::item_value(t,wand)==1536);
 t.item_base["staff"].cost=100000;t.skill_recharge[54]={1536,100};wand.quality=2;wand.props={{kSingleSkill,54,2}};CHECK(rule::item_value(t,wand)==400300&&rule::item_value(t,wand,rule::PriceKind::Sell)==100300);t.item_base["staff"].cost=1024;t.skill_recharge[54]={1024,100};
 Header h;auto& npc=t.npc_prices["vendor"];npc.max_buy={1000000,1000000,1000000};wand.quality=2;wand.props={{kSingleSkill,54,3}};wand.max_durability=20;wand.durability=10;
 CHECK(rule::item_price(t,wand,"vendor",false,h)==6644&&rule::item_price(t,wand,"vendor",true,h)==2804&&rule::repair_cost(t,wand,"vendor",h)==3322);
 CHECK(rule::curse_duration(101,50)==51&&rule::curse_duration(25,99)==1&&rule::curse_duration(100,100)==0&&rule::curse_duration(100,-50)==150);
 std::puts("PASS specialized prices: staffmod own buy/sale/repair profiles and stage order, encoded single/proc/charge skills, remaining-charge independence, ordinary stat and socket bonus aggregation, AutoMagic, hidden properties and unique/set no double surcharge; curse duration rounding/immunity");
}

static void stack_price_test(){
 rule::Tables t;auto& base=t.item_base["jav"];base.cost=100;base.stackable=true;base.max_stack=80;
 t.item_info["jav"].type="thro";t.item_info["jav"].kind=2;t.item_info["jav"].width=t.item_info["jav"].height=1;t.types["thro"].autostack=true;t.types["thro"].throwable=t.types["thro"].repairable=true;
 t.prefix_cost={{1024,700}};auto jav=item("jav");jav.quality=4;jav.prefix=0;jav.suffix=-1;jav.quantity=20;
 Header h;auto& npc=t.npc_prices["vendor"];npc.buy=2048;npc.sell=1024;npc.rep=512;npc.max_buy={1000000,1000000,1000000};
 CHECK(rule::item_value(t,jav)==110);CHECK(rule::item_price(t,jav,"vendor",false,h)==4400);
 CHECK(rule::item_price(t,jav,"vendor",true,h)==5500&&rule::repair_cost(t,jav,"vendor",h)==3300);
 CHECK(rule::item_price(t,jav,"vendor",false,h,25)==3300&&rule::repair_cost(t,jav,"vendor",h,25)==2475);
 jav.quantity=80;CHECK(rule::item_price(t,jav,"vendor",false,h)==17600&&rule::item_price(t,jav,"vendor",true,h)==8800&&rule::repair_cost(t,jav,"vendor",h)==0);
 jav.quantity=0;CHECK(rule::item_price(t,jav,"vendor",false,h)==220&&rule::item_price(t,jav,"vendor",true,h)==4455&&rule::repair_cost(t,jav,"vendor",h)==4345);
 jav.quantity=20;jav.props={{253,0,20}};CHECK(rule::item_price(t,jav,"vendor",true,h)==8800&&rule::repair_cost(t,jav,"vendor",h)==1);
 jav.props={{254,0,60}};CHECK(rule::item_value(t,jav)==105&&rule::item_price(t,jav,"vendor",false,h)==4200&&rule::item_price(t,jav,"vendor",true,h)==8460&&rule::repair_cost(t,jav,"vendor",h)==6240);
 jav.props.clear();jav.ethereal=true;CHECK(rule::item_price(t,jav,"vendor",true,h)==540&&rule::repair_cost(t,jav,"vendor",h)==0);
 t.types["thro"].cls="ama";CHECK(rule::item_price(t,jav,"vendor",true,h)==120);jav.ethereal=false;CHECK(rule::item_price(t,jav,"vendor",true,h)==1); // refill can exceed class-discounted sale value
 t.types["thro"].cls.clear();jav.identified=false;CHECK(rule::item_price(t,jav,"vendor",true,h)==2000&&rule::repair_cost(t,jav,"vendor",h)==0);jav.identified=true;
 npc.max_buy[0]=500;CHECK(rule::item_price(t,jav,"vendor",true,h)==500);npc.max_buy[0]=1000000;
 // Integer rounding is applied to each unit before multiplying quantities.
 npc.buy=1536;npc.rep=333;CHECK(rule::item_price(t,jav,"vendor",false,h)==3300&&rule::repair_cost(t,jav,"vendor",h)==2100);npc.buy=2048;npc.rep=512;
 // Actual purchase, sale and refill use the quotes, with atomic failed payment.
 rule::Store store;store.npc_id="vendor";store.tab=1;store.tabs[1]={jav};Stats gold;gold.values[kLevel]=80;gold.values[kGold]=4399;std::vector<Item> bag;
 CHECK(!rule::store_buy(t,store,0,bag,gold)&&bag.empty()&&store.tabs[1].size()==1&&gold.get(kGold)==4399);
 gold.values[kGold]=10000;CHECK(rule::store_buy(t,store,0,bag,gold)&&gold.get(kGold)==5600&&bag[0].quantity==20);
 rule::store_sell(t,store,0,bag,gold);CHECK(gold.get(kGold)==11100&&bag.empty());
 auto repair=jav;gold.values[kGold]=3299;CHECK(!rule::store_repair(t,store,repair,gold)&&repair.quantity==20&&gold.get(kGold)==3299);
 gold.values[kGold]=3300;CHECK(rule::store_repair(t,store,repair,gold)&&repair.quantity==80&&gold.get(kGold)==0);
 t.types["thro"].repairable=false;auto consumable=jav;CHECK(!rule::restocks(t,consumable)&&rule::repair_cost(t,consumable,"vendor",h)==0&&rule::item_price(t,consumable,"vendor",true,h)==2200);CHECK(!rule::store_repair(t,store,consumable,gold)&&consumable.quantity==20);t.types["thro"].repairable=true;
 jav.flags|=0x20000;CHECK(rule::item_price(t,jav,"vendor",true,h)==1&&rule::item_price(t,jav,"vendor",false,h)==1&&rule::repair_cost(t,jav,"vendor",h)==1);jav.flags=0;
 // Defense-scaled armor base and sockets are shared by quote and repair.
 t.item_base["arm"].cost=1000;t.item_base["arm"].minac=2;t.item_base["arm"].maxac=10;t.item_info["arm"].kind=1;t.item_base["gem"].cost=200;
 auto armor=item("arm");armor.defense=5;armor.max_durability=20;armor.durability=10;armor.socketed_items={item("gem")};
 CHECK(rule::item_value(t,armor)==600&&rule::item_price(t,armor,"vendor",false,h)==1200&&rule::repair_cost(t,armor,"vendor",h)==150);
 t.item_base["jav"].cost=INT32_MAX;jav.quantity=511;CHECK(rule::item_price(t,jav,"vendor",false,h)==INT32_MAX);
 std::puts("PASS native stack prices: amortized affixes/capacity, unit-before-quantity NPC rounding, native sale-minus-refill ordering, replenishment/empty/ethereal/class/starting flags, capped sale and discount, atomic actual trade/refill, shared armor/socket base and overflow bounds");
}

static void book_price_test(){
 rule::Tables t;t.item_base["tbk"].cost=200;t.item_base["tbk"].stackable=true;t.item_base["tbk"].max_stack=20;
 t.item_info["tbk"].type="book";t.book_charge_cost["tbk"]=100;
 auto book=item("tbk");book.location=item_location::kStored;book.panel=item_panel::kInventory;book.quantity=20;book.id=12;
 Header h;CHECK(rule::item_price(t,book,"",false,h)==2200);book.quantity=5;CHECK(rule::item_price(t,book,"",false,h)==700);
 book.quantity=0;CHECK(rule::item_price(t,book,"",false,h)==300); // native minimum quantity of one
 book.quantity=5;auto& price=t.npc_prices["akara"];price.buy=2048;price.sell=256;price.max_buy={100000,100000,100000};
 CHECK(rule::item_price(t,book,"akara",false,h)==1400&&rule::item_price(t,book,"akara",false,h,25)==1050);
 CHECK(rule::item_price(t,book,"akara",true,h)==175);book.ethereal=true;CHECK(rule::item_price(t,book,"akara",true,h)==43);book.ethereal=false;
 price.max_buy[0]=100;CHECK(rule::item_price(t,book,"akara",true,h)==100);
 // The same function drives actual payment and buy-back offers.
 rule::Store shop;shop.npc_id="akara";shop.tab=3;shop.tabs[3]={book};Stats stats;stats.values[kGold]=5000;stats.values[kLevel]=30;
 std::vector<Item> bag;CHECK(rule::store_buy(t,shop,0,bag,stats)&&stats.get(kGold)==3600&&bag[0].quantity==5);
 rule::store_sell(t,shop,0,bag,stats);CHECK(bag.empty()&&stats.get(kGold)==3700&&shop.tabs[3][0].quantity==5);
 // A book code without metadata still uses the existing generic fallback.
 t.book_charge_cost.clear();CHECK(rule::item_price(t,book,"",false,h)==200);
 std::puts("PASS tome prices: charge count, native empty minimum, vendor/discount/sell cap, shared payment and buyback, metadata fallback");
}

static void recovery_world_test(){
 GameData data;auto& t=data.rules;
 t.item_info["arm"].kind=1;t.item_info["arm"].type="tors";t.item_info["arm"].width=t.item_info["arm"].height=1;
 t.item_base["arm"].cost=100;t.item_info["jav"].kind=2;t.item_info["jav"].type="thro";t.item_info["jav"].width=t.item_info["jav"].height=1;
 t.item_base["jav"].stackable=true;t.item_base["jav"].max_stack=80;t.types["thro"].autostack=true;t.types["thro"].throwable=t.types["thro"].repairable=true;
 data.town.npcs.resize(3);Level away;
 World w(&data,-1,-1);w.character.stats.values[kGold]=100000;w.character.stats.values[kLevel]=50;
 auto armor=item("arm");armor.max_durability=40;armor.durability=38;armor.props={{252,0,100}};
 w.character.items={armor};w.character.merc_items={armor};w.held=armor;
 w.corpses.push_back({});w.corpses[0].items={armor};
 Loot::GroundItem drop;drop.item=armor;w.loot.ground={drop};w.loot.kept[&away]={drop};
 w.store.npc=0;w.store.tabs[0]={armor};w.stocks[0]=w.store;
 w.stocks[1].npc=1;w.stocks[1].tabs[0]={armor};
 auto& gamble=w.stocks[2];gamble.npc=2;gamble.gamble=true;auto hidden=armor;hidden.id=7;gamble.tabs[0]={hidden};gamble.gamble_rolls[7]=hidden;
 w.recover_items(0);CHECK(w.item_recovery.clocks.size()==8); // one clock per actual unit, never vendor cache or gamble
 const int carried=w.character.items[0].id;const int floor=w.loot.ground[0].item.id;const int vendor=w.store.tabs[0][0].id;
 CHECK(carried!=floor&&carried!=vendor&&floor!=vendor&&w.stocks[0].tabs[0][0].id==-1);
 // Same item identity moves between containers midway through the delay.
 w.loot.ground.push_back(drop);w.loot.ground.back().item=std::move(w.character.items[0]);w.character.items.clear();
 w.corpses[0].items.push_back(std::move(w.loot.ground[0].item));w.loot.ground.erase(w.loot.ground.begin());
 w.recover_items(500);CHECK(w.loot.ground[0].item.id==carried&&w.corpses[0].items.back().id==floor);
 CHECK(w.deal(cmd::CloseTrade{}));CHECK(w.store.npc<0&&w.stocks[0].tabs[0][0].id==vendor);
 w.recover_items(1039);CHECK(w.stocks[0].tabs[0][0].durability==38);
 w.recover_items(1040);
 CHECK(w.loot.ground[0].item.durability==39&&w.corpses[0].items[0].durability==39&&w.corpses[0].items[1].durability==39);
 CHECK(w.character.merc_items[0].durability==39&&w.held->durability==39&&w.loot.kept[&away][0].item.durability==39);
 CHECK(w.stocks[0].tabs[0][0].durability==39&&w.stocks[1].tabs[0][0].durability==39);
 CHECK(gamble.tabs[0][0].id==7&&gamble.tabs[0][0].durability==38&&gamble.gamble_rolls[7].durability==38);
 CHECK(w.deal(cmd::OpenTrade{0,false}));CHECK(w.store.tabs[0][0].durability==39);
 // Buy a finite unit back: its existing timer must not restart.
 CHECK(rule::store_buy(t,w.store,0,w.character.items,w.character.stats));CHECK(w.character.items[0].id==vendor);
 w.stocks[0]=w.store;w.recover_items(3000);w.recover_items(6039);CHECK(w.character.items[0].durability==39);
 w.recover_items(6040);CHECK(w.character.items[0].durability==40&&w.loot.kept[&away][0].item.durability==40);
 w.recover_items(11040);CHECK(w.item_recovery.clocks.empty());
 // Timer updates made while the store is open survive direct vendor switches.
 w.store.tabs[0]={armor};w.recover_items(12000);w.recover_items(13040);CHECK(w.store.tabs[0][0].durability==39);
 CHECK(w.deal(cmd::OpenTrade{1,false}));CHECK(w.stocks[0].tabs[0][0].durability==39);
 // Permanent offers create fresh units, instead of copying the vendor's clock/id.
 w.store.tabs[0]={armor};w.store.perm={"arm"};w.recover_items(14000);const int offer=w.store.tabs[0][0].id;
 w.character.items.clear();CHECK(rule::store_buy(t,w.store,0,w.character.items,w.character.stats));
 CHECK(w.character.items[0].id==-1&&w.store.tabs[0][0].id==offer);w.recover_items(15000);CHECK(w.character.items[0].id!=offer);
 // Native on-floor regeneration changes quantity but cannot invoke a player repair callback.
 auto jav=item("jav");jav.max_durability=20;jav.durability=0;jav.quantity=0;jav.flags|=0x100;jav.props={{253,0,100}};
 drop.item=jav;w.loot.ground={drop};w.recover_items(20000);w.recover_items(21040);
 CHECK(w.loot.ground[0].item.quantity==1&&rule::broken(w.loot.ground[0].item));
 w.character.items={std::move(w.loot.ground[0].item)};w.loot.ground.clear();w.recover_items(26040);
 CHECK(w.character.items[0].quantity==2&&!rule::broken(w.character.items[0])&&w.character.items[0].durability==20);
 // Destroyed units clean up; ordinary loot and empty cached pages add no clocks.
 w.character.items.clear();w.character.merc_items.clear();w.held.reset();w.corpses.clear();w.loot.kept.clear();w.stocks.clear();w.store={};w.recover_items(30000);
 CHECK(w.item_recovery.clocks.empty());
 std::puts("PASS world item timers: floor/off-level/corpse/merc/cursor/vendor recovery, stable transfer and buyback identities, no duplicated vendor cache, close/switch persistence, permanent-copy and gamble isolation, owned-only broken-stack reactivation, removed-unit cleanup");
}

static void socket_local_rules_test(){
 rule::Tables t;t.item_info["swd"].kind=2;t.item_info["swd"].type="weap";t.item_info["cap"].kind=1;t.item_info["cap"].type="helm";t.item_info["ash"].kind=1;t.item_info["ash"].type="ashd";t.types["ashd"].equiv[0]="shld";
 for(int k=0;k<3;++k)t.gem_props["gem"][std::size_t(k)]={{kMaxDurability,0,3+k},{kMaxDurabilityPercent,0,20+10*k},{kRequirementPercent,0,-10-k},{254,0,3+k},{252,0,100}};
 auto jewel=item("jew");jewel.props={{kMaxDurability,0,5},{kMaxDurabilityPercent,0,5},{kRequirementPercent,0,-5},{254,0,2}};auto gem=item("gem");
 int kind=0;for(const char* code:{"swd","cap","ash"}){
  auto a=item(code);a.id=7;a.max_durability=40;a.durability=1;a.props={{kMaxDurability,0,2},{kMaxDurabilityPercent,0,10},{kRequirementPercent,0,-5},{254,0,5}};a.socketed_items={gem,jewel};
  const int expected=(50+kind)*(135+10*kind)/100;CHECK(rule::max_durability(t,a)==expected&&rule::item_local_stat(t,a,254)==10+kind&&rule::required_attribute(t,a,100)==80-kind);
  t.item_base[code].cost=10000;t.item_base[code].max_stack=40;CHECK(rule::total_max_stack(t,a)==50+kind);
  rule::Store vendor;Stats gold;gold.values[kGold]=100000;CHECK(rule::store_repair(t,vendor,a,gold)&&a.durability==expected);
  a.durability=expected-1;rule::ItemRecovery recovery;recovery.begin();recovery.update(t,a,0);recovery.end();recovery.begin();CHECK(!recovery.update(t,a,1039));recovery.end();recovery.begin();CHECK(recovery.update(t,a,1040)&&a.durability==expected);recovery.end();
  rule::CubeRecipe recipe;recipe.outputs[0].kind=rule::cube_out::kUseItem;recipe.outputs[0].flags=rule::cube_out::kRepair;rule::CubeMatch match;match.source[0]=0;rule::Rng rng{3};a.durability=1;
  auto repaired=rule::cube_make(t,recipe,match,{&a},50,true,0,rng,nullptr);CHECK(repaired.made&&repaired.items.size()==1&&repaired.items[0].durability==expected);
  a.socketed_items.clear();CHECK(rule::max_durability(t,a)==46&&rule::item_local_stat(t,a,254)==5);++kind;
 }
 auto a=item("swd");a.id=1;a.max_durability=40;a.durability=10;a.socketed_items={gem};t.gem_props["gem"][0].push_back({kIndestructible,0,1});CHECK(rule::indestructible(t,a)&&rule::repair_cost(t,a,"",{})==0);
 rule::ItemRecovery recovery;recovery.begin();CHECK(!recovery.update(t,a,0));recovery.end();CHECK(recovery.clocks.empty());
 a.socketed_items={jewel};a.socketed_items[0].props.push_back({kIndestructible,0,1});CHECK(rule::indestructible(a)&&rule::indestructible(t,a));a.socketed_items.clear();CHECK(!rule::indestructible(t,a));
 a.props={{kMaxDurability,0,INT32_MAX},{kMaxDurabilityPercent,0,INT32_MAX}};CHECK(rule::max_durability(t,a)==INT32_MAX);a.props={{kMaxDurabilityPercent,0,-200}};CHECK(rule::max_durability(t,a)==0);
 std::puts("PASS socket local rules: weapon/armor/inherited shield context, own+jewel+gem properties once, durability and stack capacity, repair/cube/recovery/requirements, removal, indestructibility, arithmetic bounds");
}
static void repair_transaction_test(){
 rule::Tables t;t.item_base["arm"].cost=1000;rule::Store shop;
 auto a=item("arm");a.slot=3;a.max_durability=20;a.durability=10;
 auto b=a;b.slot=1;b.durability=15;auto bag=a;bag.location=item_location::kStored;bag.panel=item_panel::kInventory;
 auto stash=bag;stash.panel=item_panel::kStash;auto cube=bag;cube.panel=item_panel::kCube;
 auto eth=a;eth.ethereal=true;auto unknown=a;unknown.identified=false;
 std::vector<Item> items{a,b,bag,stash,cube,eth,unknown};Stats gold;
 CHECK(rule::repair_cost(t,unknown,"",{})==0&&rule::repair_all_cost(t,shop,items)==750);
 gold.values[kGold]=600;gold.values[kGoldBank]=149;
 CHECK(rule::store_repair_all(t,shop,items,gold)==0&&gold.get(kGold)==600&&gold.get(kGoldBank)==149);
 CHECK(items[0].durability==10&&items[1].durability==15&&items[2].durability==10);
 gold.values[kGoldBank]=200;CHECK(rule::store_repair_all(t,shop,items,gold)==2&&gold.get(kGold)==0&&gold.get(kGoldBank)==50);
 CHECK(items[0].durability==20&&items[1].durability==20&&items[2].durability==10&&items[3].durability==10&&items[4].durability==10&&items[5].durability==10&&items[6].durability==10);
 CHECK(rule::store_repair_all(t,shop,items,gold)==0&&gold.get(kGoldBank)==50);
 gold.values[kGold]=1000;CHECK(rule::store_repair(t,shop,items[2],gold)&&items[2].durability==20&&gold.get(kGold)==500); // individual carried repair still allowed
 GameData data;data.rules=t;World w(&data,-1,-1);w.store.npc=1;w.character.items={a,b};w.character.stats.values[kGold]=600;w.character.stats.values[kGoldBank]=149;
 w.apply(cmd::Repair{-1},1000);CHECK(w.store.notice==rule::ShopNoGold&&w.character.items[0].durability==10&&w.character.stats.get(kGold)==600);
 w.character.stats.values[kGoldBank]=150;w.apply(cmd::Repair{-1},1040);CHECK(w.store.notice==rule::ShopOk&&w.character.items[0].durability==20&&w.character.items[1].durability==20&&w.character.stats.get(kGoldBank)==0);
 std::puts("PASS repair transaction: equipped-only Repair All, whole-price preflight, atomic insufficient-gold failure, purse/stash payment, no unidentified/ethereal repair, world notices");
}
static void skills_test(){
 GameData data;data.town.id=1;data.skills.rows.resize(55);auto& spell=data.skills.rows[54];spell.id=54;spell.name="Teleport";spell.cls="sor";spell.mana=10;spell.manashift=8;data.skills.class_ids[1]={54};
 World world(&data,-1,-1);auto& c=world.character;c.character_class=4;c.stats.values[kLevel]=30;c.stats.values[kMana]=0;
 Item staff=item("sst");staff.props={{kChargedSkill,(54<<6)|3,(20<<8)|2},{kAllSkills,0,10}};c.items={staff};
 CHECK(skill_level(data,c,54,{})==3);CHECK(world.fight.pay_skill(spell,3,100)&&c.stats.get(kMana)==0&&(c.items[0].props[0].value&255)==1);
 CHECK(world.fight.pay_skill(spell,3,200));world.fight.swing_skill=54;world.fight.pmode=kModeSC;
 CHECK(world.fight.skill_level(54)==3);world.fight.pmode=-1;CHECK(skill_level(data,c,54,{})==0&&!world.fight.pay_skill(spell,3,300));
 // Explicit charges may coexist with a learned skill and a stronger source.
 c.items[0].id=11;c.items[0].props[0].value=(20<<8)|2;c.character_class=1;c.stats.skills[0]=1;c.stats.values[kMana]=0;
 auto ring=item("rin");ring.id=12;ring.slot=6;ring.props={{kChargedSkill,(54<<6)|7,(30<<8)|5}};c.items.push_back(ring);
 world.fight.charge_request_skill=54;world.fight.charge_request_item=11;world.fight.charge_request_level=3;
 CHECK(world.fight.skill_level(54)==11&&world.fight.cast_level(54)==3); // +all10 improves learned, never charges.
 CHECK(world.fight.pay_skill(spell,world.fight.cast_level(54),250)&&(c.items[0].props[0].value&255)==1&&(c.items[1].props[0].value&255)==5&&c.stats.get(kMana)==0);
 CHECK(world.fight.pay_skill(spell,world.fight.cast_level(54),260)&&world.fight.cast_level(54)==0&&!world.fight.pay_skill(spell,0,270));
 CHECK((c.items[1].props[0].value&255)==5);world.fight.charge_request_item=12;world.fight.charge_request_level=7;CHECK(world.fight.cast_level(54)==7);
 c.items[1].flags|=0x100;CHECK(world.fight.cast_level(54)==0);c.items[1].flags=0;c.items[1].location=item_location::kStored;CHECK(world.fight.cast_level(54)==0);
 c.items[1].location=item_location::kEquipped;c.items[1].slot=11;CHECK(world.fight.cast_level(54)==0);c.items.pop_back();world.fight.charge_request_skill=world.fight.charge_request_item=-1;world.fight.charge_request_level=0;c.stats.skills[0]=0;c.character_class=4;
 const auto packet=encode(Command{cmd::UseSkill{54,10,20,-1,false,11,3}});const auto parsed=decode(packet);CHECK(parsed);const auto* use=std::get_if<cmd::UseSkill>(&*parsed);CHECK(use&&use->charge_item==11&&use->charge_level==3&&use->skill==54);
 std::puts("PASS explicit charged source: learned coexistence, exact item/level, no mana, depleted/broken/inactive/moved rejection, no stronger-source fallback, codec");
 c.items[0].props[0].value=(20<<8)|2;c.items[0].slot=11;CHECK(skill_level(data,c,54,{})==0&&charged_source(c.items,54)==nullptr);c.items[0].slot=4;
 c.items[0].props={{97,54,1},{kAllSkills,0,2}};CHECK(skill_level(data,c,54,{})==3);
 c.character_class=1;c.items[0].props={{97,54,20},{kAllSkills,0,2}};CHECK(skill_level(data,c,54,{})==5);
 c.items[0].props={{107,54,3},{kAllSkills,0,2}};CHECK(skill_level(data,c,54,{})==5);
 c.character_class=4;CHECK(skill_level(data,c,54,{})==0);c.character_class=1;c.items[0].props={{kAllSkills,0,10}};CHECK(skill_level(data,c,54,{})==0);
 c.stats.skills[0]=1;c.items[0].props={{kChargedSkill,(54<<6)|3,(20<<8)|2}};CHECK(skill_level(data,c,54,{})==1);CHECK(!world.fight.pay_skill(spell,1,400)&&(c.items[0].props[0].value&255)==2);
 c.stats.values[kMana]=10000;CHECK(world.fight.pay_skill(spell,1,500)&&c.stats.get(kMana)==7440&&(c.items[0].props[0].value&255)==2);
 ItemTables tables;tables.weapons.insert("sst");tables.stats.resize(512);for(auto& s:tables.stats){s.save_bits=16;s.csv_bits=16;}tables.stats[kChargedSkill].param_bits=16;
 Header h;h.name="Charges";h.level=30;auto loaded=parse_items(write_save({},h,c.stats,c.items,tables),tables);CHECK(loaded.size()==1&&loaded[0].props[0].param==((54<<6)|3)&&loaded[0].props[0].value==((20<<8)|2));
 // Fully durable items can be recharged; payment is atomic and ethereal
 // items remain unavailable for vendor repair. Cube recharge uses its flag.
 data.rules.skill_levels.resize(55);data.rules.skill_levels[54]={18,20};data.rules.skill_recharge.resize(55);data.rules.skill_recharge[54]={1024,100};
 c.items[0].props[0].value=(20<<8)|18;rule::Store store;store.npc_id="charsi";
 const int price=rule::repair_cost(data.rules,c.items[0],store.npc_id,store.header);CHECK(price==11010);
 c.stats.values[kGold]=price-1;CHECK(!rule::store_repair(data.rules,store,c.items[0],c.stats)&&(c.items[0].props[0].value&255)==18);
 c.stats.values[kGold]=price+100;CHECK(rule::store_repair(data.rules,store,c.items[0],c.stats)&&c.stats.get(kGold)==100&&(c.items[0].props[0].value&255)==20);
 c.items[0].props[0].value=(20<<8)|18;c.items[0].ethereal=true;CHECK(rule::repair_cost(data.rules,c.items[0],"charsi",store.header)==0);c.items[0].ethereal=false;
 rule::CubeRecipe recipe;recipe.outputs[0].kind=rule::cube_out::kUseItem;recipe.outputs[0].flags=rule::cube_out::kRecharge;rule::CubeMatch match;match.source[0]=0;rule::Rng random;
 auto recharged=rule::cube_make(data.rules,recipe,match,{&c.items[0]},30,true,0,random,nullptr);CHECK(recharged.made&&(recharged.items[0].props[0].value&255)==20);
 std::puts("PASS: charged spells, zero mana, last-charge cast snapshot, empty/inactive sources, staffmods/oskill cap, learned priority and charge save/reload");
}
static void poison_rules_test(){
 rule::Tables tables;rule::StatSum sum{},weapon;Stats stats;stats.values[kLevel]=30;
 sum[kPoisonMinDamage]=sum[kPoisonMaxDamage]=512;sum[kPoisonLength]=300;sum[kPoisonCount]=2;
 auto f=rule::make_fighter(tables,nullptr,nullptr,sum,weapon,stats,{},0,{});CHECK(f.poison_len==150&&f.elem[3].first==300&&f.elem[3].second==300);
 auto dagger=f;rule::weapon_poison(dagger,256,256,200);CHECK(dagger.poison_len==250&&dagger.elem[3].first==750); // (skill200 + item300) / 2
 sum[kSkillPoisonOverrideLength]=10;sum[kPoisonMinDamage]+=256;sum[kPoisonMaxDamage]+=256;
 f=rule::make_fighter(tables,nullptr,nullptr,sum,weapon,stats,{},0,{});CHECK(f.poison_len==10&&f.elem[3].first==30);
 rule::weapon_poison(f,256,256,200);CHECK(f.poison_len==210&&f.elem[3].first==840); // Venom: item length/count ignored
 sum[kSkillPoisonOverrideLength]=0;sum[kPoisonCount]=0;f=rule::make_fighter(tables,nullptr,nullptr,sum,weapon,stats,{},0,{});CHECK(f.poison_len==300&&f.elem[3].first==900);
 sum[kPoisonCount]=2;sum[kPassivePoisonMastery]=50;f=rule::make_fighter(tables,nullptr,nullptr,sum,weapon,stats,{},0,{});CHECK(f.poison.low==1152&&f.elem[3].first==675);
 rule::Rng rng{1};rule::MissileDamage nova;nova.etype=3;nova.elo=nova.ehi=256;nova.elen=125;const auto spell=rule::missile_blow(nova,{}, {},rng);CHECK(spell.poison==125&&spell.poison_ticks==125); // pure poison spells do not average item sources
 std::puts("PASS poison rules: source average, Poison Dagger combined length, Venom override and dagger interaction, zero-count fallback, mastery rates, standalone spell isolation");
}
static void combat_test(){
 rule::Target target;target.hit_points=800;CHECK(rule::crushing_damage(target,false)==200);CHECK(rule::crushing_damage(target,true)==100);
 target.boss=true;CHECK(rule::crushing_damage(target,false)==100&&rule::crushing_damage(target,true)==50);
 target.res[0]=-100;CHECK(rule::crushing_damage(target,false)==100);target.res[0]=50;CHECK(rule::crushing_damage(target,false)==50);target.res[0]=100;CHECK(rule::crushing_damage(target,false)==0);
 target.res[0]=0;target.player_like=true;CHECK(rule::crushing_damage(target,false)==80&&rule::crushing_damage(target,true)==40);
 auto attacker=rule::simple_fighter(1,1,100);attacker.smite_lo=attacker.smite_hi=1;attacker.crushing=100;attacker.open_wounds=100;rule::Rng random;
 auto hit=rule::player_blow(attacker,target,30,random,{.smite=true});CHECK(hit.hit&&hit.crushing&&hit.bleed&&hit.damage==81&&hit.life==0&&hit.mana==0);
 GameData data;data.monsters.types.resize(1);data.monsters.types[0].boss_column=true;Monster monster;monster.type=0;monster.hit_points=800;CHECK(monster.target(data).boss);
 data.monsters.types[0].boss_column=false;monster.boss=rule::Boss::champion;CHECK(monster.target(data).boss);monster.boss=rule::Boss::minion;CHECK(!monster.target(data).boss);
 std::puts("PASS: Crushing Blow monster ranks, melee/missile fractions, resistance, Smite and Open Wounds");
}
static void portal_lifecycle_test(GameData& data){
 Level arena;arena.id=2;arena.ds1=d2d::ds1::Map(64,64,0,1);arena.walk.resize(320*320);const auto* field=&arena;
 World w(&data,-1,-1);w.level=field;w.character.stats.values[kLevel]=30;w.character.stats.values[kLife]=w.character.stats.values[kMaxLife]=25600;
 const auto spot=field->nearest_free(10,10);w.player.x=spot.first;w.player.y=spot.second;
 auto trip=[&](int end){w.take_portal=end;w.player.walking=false;w.player.x=w.portal[end].x;w.player.y=w.portal[end].y;w.use_portal(1000);};
 w.open_portal(100);CHECK(w.portal[0].level==field&&w.portal[1].level&&w.portal[1].consumable);
 const auto* town=w.portal[1].level;
 trip(0);CHECK(w.level==town&&w.portal[0].level&&w.portal[1].level);
 trip(1);CHECK(w.level==field&&!w.portal[0].level&&!w.portal[1].level);
 // Shrine/quest portals and QA routes are permanent, unlike scroll portals.
 w.open_portal_at(w.player.x,w.player.y,200,false);trip(0);trip(1);
 CHECK(w.level==field&&w.portal[0].level&&w.portal[1].level&&!w.portal[1].consumable);
 w.portal[0].consumable=w.portal[1].consumable=true;trip(0);
 w.portal[0]={};w.take_portal=1;w.player.x=w.portal[1].x;w.player.y=w.portal[1].y;w.use_portal(2000);CHECK(w.level==town&&w.take_portal==-1);
 std::puts("PASS portal lifecycle: scroll round trip closes pair, permanent pairs survive, missing endpoint rejected");
}
int main(int argc,char**argv)try{rules_test();recovery_rules_test();stack_price_test();stone_skin_test();specialized_price_test();book_price_test();recovery_world_test();socket_local_rules_test();repair_transaction_test();skills_test();poison_rules_test();combat_test();
 if(argc>1){auto d=load_game_data(argv[1],std::filesystem::path(argv[1])/"LODPatch_114d.exe",3,false);CHECK(d);portal_lifecycle_test(*d);const auto& t=d->rules;CHECK(!t.runewords.empty());
  for(const char* code:{"tbk","ibk"}){CHECK(t.book_charge_cost.contains(code)&&t.book_charge_cost.at(code)>0);
   auto book=item(code);Header h;book.quantity=1;const int one=rule::item_price(t,book,"",false,h);book.quantity=20;
   CHECK(rule::item_price(t,book,"",false,h)-one==19*t.book_charge_cost.at(code));
  }
  std::puts("PASS real Books.txt: both tome codes and per-charge transaction prices");
  for(const char* code:{"jav","tkf","tax"}){auto stack=item(code);stack.quantity=1;const int single=rule::item_price(t,stack,"",false,{});stack.quantity=20;CHECK(rule::item_price(t,stack,"",false,{})==20*single);
   CHECK(t.item_base.at(code).stackable&&t.item_info.at(code).kind==2);
  }
  std::puts("PASS real throwing price rows: javelin/knife/axe quantity-scaled offers");
  for(const char* code:{"jav","tkf","tax"})CHECK(rule::restocks(t,item(code)));
  for(const char* code:{"gpl","gpm","gps","opl","opm","ops"}){auto potion=item(code);potion.quantity=5;CHECK(t.item_base.contains(code)&&t.item_base.at(code).stackable&&t.item_info.at(code).kind==2&&!rule::restocks(t,potion)&&rule::repair_cost(t,potion,"charsi",{})==0);}
  std::puts("PASS real refill eligibility: javelin/knife/axe repair enabled, all six throwing potion bases excluded by native Repair flag");
  if(std::getenv("D2_MECHANIC_AUDIT")){
   std::vector<std::string> codes;for(const auto& [code,fs]:t.properties)for(const auto& f:fs)if(f.func==18){codes.push_back(code);std::printf("TIME_PROPERTY %s stat=%d\n",code.c_str(),f.stat);}
   const auto uses=[&](const auto& mods){for(const auto& mod:mods)if(std::ranges::find(codes,rule::lower_case(mod.code))!=codes.end())return true;return false;};
   for(const auto* list:{&t.prefixes,&t.suffixes,&t.automagic})for(const auto& row:*list)if(uses(row.mods))std::printf("TIME_AFFIX %s spawn=%d freq=%d lvl=%d\n",row.name.c_str(),row.spawnable,row.frequency,row.level);
   for(const auto* list:{&t.uniques,&t.sets})for(const auto& row:*list)if(uses(row.mods)||uses(row.bonus))std::printf("TIME_SPECIAL %s enabled=%d\n",row.index.c_str(),row.enabled);
   for(const auto& row:t.runewords)if(uses(row.mods))std::printf("TIME_RUNEWORD %d\n",row.id);
   for(const auto& row:t.set_bonuses)if(uses(row.partial)||uses(row.full))std::puts("TIME_SET_BONUS");
   for(const auto& row:t.superior)if(uses(row.mods))std::puts("TIME_SUPERIOR");

  }

  auto sword=item("ssd");sword.socketed=true;sword.sockets=2;std::optional<Item> held=item("r03");held->simple=true;CHECK(rule::insert_socket(t,sword,held));held=item("r01");held->simple=true;CHECK(rule::insert_socket(t,sword,held)&&sword.runeword); // Steel: Tir El
  CHECK(d->item_names.runeword[std::size_t(sword.runeword_id-27)].size()>0);CHECK(sword.props.size()>0);auto lines=item_lines(*d,sword,30);CHECK(!lines.empty()&&lines[0].text.find("Steel")!=std::string::npos);
  CHECK(d->item_tables);Header real_header;real_header.name="SteelSave";real_header.level=30;real_header.status=0x20;Stats real_stats;real_stats.values[kLevel]=30;
  auto saved=parse_items(write_save({},real_header,real_stats,{sword},*d->item_tables),*d->item_tables);CHECK(saved.size()==1&&saved[0].runeword&&saved[0].runeword_id==sword.runeword_id&&saved[0].props.size()==sword.props.size()&&saved[0].socketed_items.size()==2);
  CHECK(item_lines(*d,saved[0],30)[0].text.find("Steel")!=std::string::npos);
  if(argc>2&&std::string_view(argv[2])=="--bench"){
   Character c;c.character_class=0;c.stats.skills.fill(1);c.stats.values[kLevel]=70;
   for(int slot=1;slot<=10;++slot){auto gear=item(slot==4?"ssd":"cap");gear.slot=slot;gear.props={{kAllSkills,0,1},{kStr,0,10},{kFireResist,0,20},{17,0,25}};c.items.push_back(gear);}
   std::vector<int> ids;for(const auto& row:d->skills.rows)if(row.id>=0)ids.push_back(row.id);
   const auto run=[&](bool shared){std::uint64_t sum=0;const auto start=std::chrono::steady_clock::now();for(int n=0;n<120;++n){
    if(shared){const auto props=gear_props(*d,c.items);for(int id:ids)sum+=skill_level_from_props(*d,c,id,props);}
    else for(int id:ids)sum+=skill_level(*d,c,id,{});
   }return std::pair{std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count(),sum};};
   std::vector<double> old_times,new_times;for(int n=0;n<9;++n){const auto a=run(n%2),b=run(!(n%2));CHECK(a.second==b.second);old_times.push_back(n%2?b.first:a.first);new_times.push_back(n%2?a.first:b.first);}
   std::ranges::sort(old_times);std::ranges::sort(new_times);std::printf("BENCH skill query batches: ids=%zu batches=120 old_us=%.0f shared_us=%.0f reduction_pct=%.2f\n",ids.size(),old_times[4],new_times[4],100*(1-new_times[4]/old_times[4]));
  }
  std::printf("PASS: real Runes.txt loaded %zu offline runewords; Tir El Steel id=%d props=%zu\n",t.runewords.size(),sword.runeword_id,sword.props.size());
 }return 0;}catch(const std::exception&e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
