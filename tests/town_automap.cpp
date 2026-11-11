// SPDX-License-Identifier: GPL-3.0-or-later
#include <town_automap.hpp>
#include <stdexcept>
using namespace d2d::client;
int main(){
 auto check=[](bool v){if(!v)throw std::runtime_error("town miniature placement");};
 auto a=town_map_cells(40,1,1028,1028);check(a.size()==15);check(a.front().cel==1&&a.front().x==-293&&a.front().y==8105);check(a.back().cel==18&&a.back().x==27&&a.back().y==8405);
 auto b=town_map_cells(40,0,1028,1028);check(b.size()==14);check(b.front().cel==21&&b.back().cel==38);
 auto c=town_map_cells(103,0,1016,1012);check(c.size()==4);check(c[0].x==-101&&c[0].y==8072);check(c[3].x==35&&c[3].y==8162);
 check(town_map_cells(75,0,0,0).empty());
}
