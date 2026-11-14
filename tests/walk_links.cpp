// SPDX-License-Identifier: GPL-3.0-or-later
#include <gamedata_load.hpp>
#include <world.hpp>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <array>
using namespace d2d::game;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
static void open_doors(World& w){
 for(std::size_t i=0;i<w.level->npcs.size();++i)if(World::is_door(w.level->npcs[i].operate_fn)){
  const auto found=w.doors.find({w.level,int(i)});
  if(found==w.doors.end()||found->second.mode==0)w.operate_door(int(i),1000);
 }
 w.reopen_doors();
}
struct Crossing{float x,y,gx,gy;};
static Crossing crossing(World& source,World& target){
 const auto& from=*source.level;const auto& to=*target.level;
 const float dx=float(to.world_x-from.world_x),dy=float(to.world_y-from.world_y);
 Crowd empty;
 const auto candidate=[&](float x,float y,float gx,float gy){
  if(from.inside(gx,gy)||!to.inside(gx-dx,gy-dy))return false;
  source.fight.rooms_up(from,x,y,true);target.fight.rooms_up(to,gx-dx,gy-dy,true);
  open_doors(source);open_doors(target);
  if(from.unit_blocked(x,y)||from.unit_blocked(gx,gy))return false;
  const auto path=player_walk(from,x,y,gx,gy,false,empty,nullptr);
  return !path.empty()&&!from.inside(path.back().first,path.back().second);
 };
 for(int axis=0;axis<2;++axis)for(int sign:{-1,1}){
  const int span=axis?from.ds1.width():from.ds1.height();
  const float edge=sign<0?0.f:float(axis?from.ds1.height():from.ds1.width());
  for(int step=0;step<span*5;++step){const float along=(float(step)+.5f)/5;
   const float a=edge-sign*.9f,b=edge+sign*.9f;
   const Crossing c=axis?Crossing{along,a,along,b}:Crossing{a,along,b,along};
   if(candidate(c.x,c.y,c.gx,c.gy))return c;
  }
 }
 throw std::runtime_error("no traversable walking seam");
}
int main(int argc,char**argv)try{
 if(argc<2){puts("SKIP: owner archives required");return 0;}
 setenv("D2D_SERIAL_LOADING","1",1);unsetenv("D2D_PREFETCH");
 auto d=load_game_data(argv[1],std::filesystem::path(argv[1])/"LODPatch_114d.exe",3,false);CHECK(d);
 int traversed=0;
 for(unsigned seed:{3u,7u,12345u})for(auto [from,to]:std::array<std::pair<int,int>,8>{{{26,27},{27,26},{27,28},{28,27},{32,33},{33,32},{107,108},{108,107}}}){
  if(argc>2&&seed!=3u)continue; // bounded ARM smoke; native/sanitizer cover all three seeds
  set_map_seed(*d,seed,0,true);const Level* a=d->level(from);CHECK(a);
  const auto& layout=d->layout_of(from);const bool fixed=std::ranges::find(layout,to,&d2d::drlg::Placed::level)==layout.end();
  want_nearby(*d,*a);d->poll_levels();
  if(fixed)CHECK(d->levels.contains(to)&&d->levels.at(to)); // no explicit loading of target
  const Level* b=d->level(to);CHECK(b);
  World w(&*d,-1,-1),other(&*d,-1,-1);w.character.character_class=4;w.character.stats.values[6]=w.character.stats.values[7]=256000;other.character.stats=w.character.stats;
  w.arrive(a,a->start.first,a->start.second,"test");open_doors(w);
  other.arrive(b,b->start.first,b->start.second,"test");open_doors(other);
  Crossing c;try{c=crossing(w,other);}catch(...){printf("FAIL seam seed=%u %d->%d\n",seed,from,to);throw;}
  w.arrive(a,c.x,c.y,"test seam");
  w.fight.monsters.clear();
  for(unsigned t=1040;t<5040&&w.level==a;t+=40){w.fight.monsters.clear();w.tick({cmd::Move{c.gx,c.gy,false}},t,t-40);}
  if(w.level!=b)printf("FAIL movement seed=%u %d->%d start=%.2f,%.2f target=%.2f,%.2f actual=%.2f,%.2f pmode=%d life=%lld blocked=%d\n",seed,from,to,c.x,c.y,c.gx,c.gy,w.player.x,w.player.y,w.fight.pmode,(long long)w.character.stats.values[6],a->unit_blocked(w.player.x,w.player.y));
  CHECK(w.level==b);++traversed;
  const float back_x=c.x-float(b->world_x-a->world_x),back_y=c.y-float(b->world_y-a->world_y);
  // The same game must return through doors opened before changing level.
  open_doors(w);
  for(unsigned t=5040;t<9040&&w.level==b;t+=40){w.fight.monsters.clear();w.tick({cmd::Move{back_x,back_y,false}},t,t-40);}
  if(w.level!=a)printf("FAIL return seed=%u %d->%d actual=%.2f,%.2f\n",seed,to,from,w.player.x,w.player.y);
  CHECK(w.level==a);++traversed;printf("PASS seed=%u walk %d -> %d and return, actual movement\n",seed,from,to);
 }
 // Outdoor Act IV links must still wait for proximity; don't re-enable broad prefetch.
 set_map_seed(*d,3,0,true);const auto* river=d->level(107);CHECK(river);want_nearby(*d,*river);d->poll_levels();CHECK(!d->levels.contains(108));
 printf("PASS %d crossings, automatic fixed-neighbour loading, outdoor demand loading preserved\n",traversed);
 return 0;
}catch(const std::exception&e){fprintf(stderr,"%s\n",e.what());return 1;}
