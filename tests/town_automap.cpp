// SPDX-License-Identifier: GPL-3.0-or-later
#include <town_automap.hpp>
#include <stdexcept>
using namespace d2d::client;
int main(){
 auto check=[](bool v){if(!v)throw std::runtime_error("town miniature placement");};
 auto a=town_map_cells(40,1,1028,1028);check(a.size()==15);check(a.front().cel==1&&a.front().x==-293&&a.front().y==8105);check(a.back().cel==18&&a.back().x==27&&a.back().y==8405);
 auto b=town_map_cells(40,0,1028,1028);check(b.size()==14);check(b.front().cel==21&&b.back().cel==38);
 auto c=town_map_cells(103,0,1016,1012);check(c.size()==4);check(c[0].x==-101&&c[0].y==8072);check(c[3].x==35&&c[3].y==8162);
 // Moving the town origin projects every piece by the same isometric delta.
 for(int level:{40,103})for(int file:{0,1}){
  const auto original=town_map_cells(level,file,1016,1028);
  for(auto [dx,dy]:{std::pair{3,0},std::pair{0,-5},std::pair{-4,4}}){
   const auto shifted=town_map_cells(level,file,1016+dx,1028+dy);
   check(shifted.size()==original.size());
   for(std::size_t i=0;i<original.size();++i){
    check(shifted[i].cel==original[i].cel);
    check(shifted[i].x-original[i].x==8*(dx-dy));
    check(shifted[i].y-original[i].y==4*(dx+dy));
   }
  }
 }
 check(town_map_cells(75,0,0,0).empty());
}
