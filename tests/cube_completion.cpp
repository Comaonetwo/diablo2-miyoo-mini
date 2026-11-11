// SPDX-License-Identifier: GPL-3.0-or-later
#include <world.hpp>
#include <gamedata_load.hpp>
#include <cube.hpp>
#include <d2s_write.hpp>
#include <cstdio>
#include <stdexcept>
#include <chrono>
using namespace d2d::game;using namespace d2d::d2s;namespace rule=d2d::rules;
#define CHECK(x) do{if(!(x))throw std::runtime_error("cube completion line "+std::to_string(__LINE__));}while(false)
int main(int argc,char**argv)try{
 Header h;h.status=0x20;h.map_id=123;h.difficulty[0]=0x83;CHECK(unlocked_difficulty(h)==0&&!select_difficulty(h,1));h.progression=5;CHECK(unlocked_difficulty(h)==1&&select_difficulty(h,1)&&h.active_difficulty()==1&&h.active_act()==0&&h.map_id==0&&h.difficulty[0]==3);h.progression=10;CHECK(select_difficulty(h,2));h.status=0;h.progression=4;CHECK(unlocked_difficulty(h)==1);h.progression=0;h.quests[0][26*2]|=1;CHECK(unlocked_difficulty(h)==1);h.status=0x20;CHECK(unlocked_difficulty(h)==0);
 rule::Tables t;t.types["armo"];t.item_info["cap"].type="armo";t.item_info["cap"].kind=1;t.item_base["cap"].minac=3;t.item_base["cap"].maxac=5;t.item_base["cap"].durability=12;t.item_info["xap"]=t.item_info["cap"];t.item_base["xap"].minac=50;t.item_base["xap"].maxac=60;t.item_base["xap"].durability=20;t.properties["life"]={{1,kMaxLife,0}};
 t.prefixes.resize(8);t.suffixes.resize(8);for(int i=1;i<8;++i)for(bool pre:{false,true}){auto& a=pre?t.prefixes[i]:t.suffixes[i];a.spawnable=a.rare=true;a.frequency=1;a.itypes={"armo"};a.group=i+(pre?100:0);a.mods={{"life","",i,i}};}
 rule::Rng rng;for(int level:{1,30,31,50,51,70,71,99})for(int n=0;n<200;++n){auto made=rule::generate_item(t,"cap",level,8,rng);int count=0;for(int a:made.affixes)count+=a>0;CHECK(made.quality==8&&count>= (level>=71?4:level>=51?3:level>=31?2:1)&&count<=4);}
 rule::ItemRollOptions opts{{5},{6},-1};auto forced=rule::generate_item(t,"cap",80,4,rng,nullptr,nullptr,false,0,&opts);CHECK(forced.prefix==5&&forced.suffix==6&&forced.props.size()==2);
 auto source=forced;source.uid=456;source.flags|=0x100;source.defense=4;source.max_durability=12;source.durability=1;
 rule::CubeRecipe recipe;recipe.outputs[0].kind=rule::cube_out::kUseType;recipe.outputs[0].flags=rule::cube_out::kMod|rule::cube_out::kRepair;rule::CubeMatch match;match.source[0]=0;match.base[0]="xap";match.ilvl[0]=80;
 auto changed=rule::cube_make(t,recipe,match,{&source},80,true,0,rng,nullptr);CHECK(changed.made&&changed.items[0].code=="xap"&&changed.items[0].defense>=50&&changed.items[0].defense<=60&&changed.items[0].max_durability==20&&changed.items[0].durability==20&&!rule::broken(changed.items[0])&&changed.items[0].prefix==5&&changed.items[0].props.size()==source.props.size());
 t.item_base["cap"].quest_diff=true;recipe={};recipe.inputs[0]={rule::cube_in::kItem,"cap",0,0,1};recipe.op=28;CHECK(!rule::cube_match(t,recipe,{&source},1,true));source.props.push_back({kQuestItemDifficulty,0,1});CHECK(rule::cube_match(t,recipe,{&source},1,true));
 recipe={};recipe.outputs[0].kind=rule::cube_out::kCowPortal;auto portal=rule::cube_make(t,recipe,{}, {},30,true,0,rng,nullptr);CHECK(portal.made&&portal.portal==1&&portal.items.empty());
 // Shared scratch preserves affix picks and the RNG sequence, not only counts.
 rule::Rng old_seed{123},new_seed{123};std::vector<std::pair<int,int>> scratch;for(int i=0;i<1000;++i)CHECK(rule::pick_affix(t,t.prefixes,"armo",90,true,{},old_seed)==rule::pick_affix(t,t.prefixes,"armo",90,true,{},new_seed,&scratch));CHECK(old_seed.low==new_seed.low&&old_seed.high==new_seed.high);
 std::puts("PASS: crafted affix tiers, forced prefixes/suffixes, upgraded base/repair, quest difficulty, portal request, difficulty unlock and affix scratch parity");
 if(argc>1){auto data=load_game_data(argv[1],std::filesystem::path(argv[1])/"LODPatch_114d.exe",3,false);CHECK(data);auto& tables=data->rules;std::vector<Item> input;for(const auto* code:{"vgl","r04","gpr","jew"})input.push_back(rule::generate_item(tables,code,85,std::string_view(code)=="vgl"||std::string_view(code)=="jew"?4:2,rng));std::vector<const Item*> cube;for(const auto& i:input)cube.push_back(&i);rule::CubeMatch real_match;const int row=rule::cube_recipe(tables,data->cube_recipes,cube,0,true,0,real_match);CHECK(row>=0);auto made=rule::cube_make(tables,data->cube_recipes[row],real_match,cube,85,true,0,rng,nullptr);CHECK(made.made&&made.items.size()==1&&made.items[0].quality==8);int count=0;for(int a:made.items[0].affixes)count+=a>0;CHECK(count==4);bool cb=false,ll=false,life=false;for(const auto& p:made.items[0].props){cb|=p.stat==136;ll|=p.stat==60;life|=p.stat==7;}CHECK(cb&&ll&&life);
  World world(&*data,-1,-1);world.character.expansion=true;world.character.header.status=0x20;world.player.x=20;world.player.y=20;CHECK(!world.cube_portal(1,0));world.character.header.quests[0][40*2]|=1;CHECK(world.cube_portal(1,0));CHECK(world.portal[10].level->id==1&&world.portal[11].level->id==39);CHECK(!world.cube_portal(1,1)&&!world.cube_portal(2,1));World transmute(&*data,-1,-1);transmute.character.expansion=true;transmute.character.header.status=0x20;
  for(const auto* code:{"leg","tbk"}){auto part=rule::generate_item(tables,code,30,2,rng);part.location=item_location::kStored;part.panel=item_panel::kCube;transmute.character.items.push_back(part);}
  transmute.transmute(0);CHECK(transmute.character.items.size()==2&&!transmute.portal[10].level);
  transmute.character.header.quests[0][80]|=1;transmute.transmute(1);CHECK(transmute.character.items.empty()&&transmute.portal[10].level);
  transmute.act1_kill({0,20,20,30,39,-1},2);CHECK(transmute.character.header.quest_flag(0,4,10)&&transmute.loot.ground.size()==8);
  std::map<int,int> ops;for(const auto& recipe:data->cube_recipes)if(recipe.enabled&&!recipe.ladder)++ops[recipe.op];for(const auto& [op,count]:ops)std::printf("AUDIT enabled offline cube op=%d rows=%d\n",op,count);
  if(argc>2){const auto run=[&](bool shared){rule::Rng seed{123};std::uint64_t sum=0;const auto start=std::chrono::steady_clock::now();for(int i=0;i<3000;++i){std::vector<std::pair<int,int>> buffer;for(int k=0;k<6;++k)sum+=rule::pick_affix(tables,tables.prefixes,"helm",90,true,{},seed,shared?&buffer:nullptr);}return std::pair{std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count(),sum};};std::vector<double> old_times,new_times;for(int i=0;i<9;++i){auto a=run(i%2),b=run(!(i%2));CHECK(a.second==b.second);old_times.push_back(i%2?b.first:a.first);new_times.push_back(i%2?a.first:b.first);}std::ranges::sort(old_times);std::ranges::sort(new_times);std::printf("BENCH affix scratch old_us=%.0f shared_us=%.0f reduction_pct=%.2f\n",old_times[4],new_times[4],100*(1-new_times[4]/old_times[4]));}
  std::printf("PASS real Blood Gloves: %s; Cow Level built and portal pair opened with quest guard\n",data->cube_recipes[row].description.c_str());
 }return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
