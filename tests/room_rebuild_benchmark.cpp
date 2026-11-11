// SPDX-License-Identifier: GPL-3.0-or-later
#include <gamedata.hpp>
#include <gamedata_load.hpp>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <algorithm>
using namespace d2d::game;
int main(int argc,char**argv){
 if(argc!=3)return 2;setenv("D2D_SERIAL_LOADING","1",1);auto data=load_game_data(argv[1],argv[2],3,true);if(!data)return 3;
 for(int id:{75,83,108}){
  auto*p=const_cast<Level*>(data->level(id));if(!p||p->rooms.empty())return 4;
  std::vector<std::size_t> order(p->rooms.size());std::iota(order.begin(),order.end(),0);
  for(int round=0;round<6;++round){
   std::rotate(order.begin(),order.begin()+1,order.end());
   auto at=std::chrono::steady_clock::now();relevel(*p,order);auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-at).count();
   std::uint64_t h=1469598103934665603ull;auto add=[&](std::uint64_t x){h=(h^x)*1099511628211ull;};
   auto tile=[&](const d2d::dt1::Tile*t){add(t!=nullptr);if(t){add(t->style);add(t->sequence);add(t->type);for(auto f:t->subtile_flags)add(f);}};
   for(std::size_t cell=0;cell<p->picks.size();++cell){add(cell);add(p->picks[cell].size());for(const auto&v:p->picks[cell]){add(v.layer);add(v.orient);add(v.hidden);add(v.cell);tile(v.tile);tile(v.stamp);}}
   for(auto v:p->walk)add(v);for(auto v:p->tile_walk)add(v);for(const auto&r:p->room_floors){add(r.size());for(const auto&f:r){add(f.x);add(f.y);add(f.material);}}
   std::printf("REBUILD level=%d round=%d ns=%lld hash=%016llx picks=%zu\n",id,round,(long long)ns,(unsigned long long)h,p->picks.values.size());std::fflush(stdout);
  }
 }
}
