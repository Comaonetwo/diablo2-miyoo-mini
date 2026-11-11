// SPDX-License-Identifier: GPL-3.0-or-later
#include <gamedata.hpp>
#include <gamedata_load.hpp>
#include <cstdio>
#include <stdexcept>
void check(bool v){if(!v)throw std::runtime_error("generated room void is walkable or valid floor is blocked");}
bool floor_at(const d2d::game::Level& l,float x,float y){
 const auto& picks=l.picks[std::size_t(int(y))*l.ds1.width()+int(x)];
 for(const auto&p:picks)if(p.layer==1&&!p.hidden)return true;return false;
}
int main(int argc,char**argv)try{
 d2d::dt1::Tile floor;floor.width=160;floor.height=-80;
 // Empty/sparse/multi-layer cells and copies must retain the old vector-grid
 // iteration order without spans pointing into another grid's allocation.
 std::vector<std::uint32_t> sizes(257);for(std::size_t i=0;i<sizes.size();++i)sizes[i]=(i*17)%5;
 d2d::game::Level::TilePicks flat;flat.reset(sizes);
 std::vector<std::vector<d2d::game::Level::Pick>> reference(sizes.size());
 for(std::size_t i=0;i<sizes.size();++i)for(std::uint32_t j=0;j<sizes[i];++j){
  d2d::game::Level::Pick p{std::uint8_t(j%3),std::uint8_t(j+1),&floor,bool(i%2),std::uint8_t(i%32),j%2?&floor:nullptr};
  reference[i].push_back(p);flat.cell(i)[j]=p;
 }
 auto copied=flat;flat.clear();
 for(std::size_t i=0;i<sizes.size();++i){check(copied[i].size()==reference[i].size());for(std::size_t j=0;j<reference[i].size();++j){const auto&a=copied[i][j];const auto&b=reference[i][j];check(a.layer==b.layer&&a.orient==b.orient&&a.tile==b.tile&&a.hidden==b.hidden&&a.cell==b.cell&&a.stamp==b.stamp);}}
 d2d::game::Level l;l.ds1=d2d::ds1::Map(5,5,1,1);std::vector<std::uint32_t> counts(25);counts[2*5+3]=1;l.picks.reset(counts);
 l.picks.cell(2*5+3)[0]={1,0,&floor,false,0,&floor};
 for(int repeat=0;repeat<2;++repeat){d2d::game::finish_level(l);
  check(l.unit_blocked(2.5f,2.5f));check(l.blocked(2.5f,2.5f,0x04));check(!l.unit_blocked(3.5f,2.5f));
  auto [x,y]=l.nearest_free(2.5f,2.5f);check(!l.unit_blocked(x,y)&&floor_at(l,x,y));check(std::ranges::equal(l.tile_walk,l.walk));
 }
 if(argc==3){setenv("D2D_SERIAL_LOADING","1",1);auto data=d2d::game::load_game_data(argv[1],argv[2],3,true);check(bool(data));const auto* cave=data->level(8);check(cave&&cave->id==8);auto [x,y]=cave->nearest_free(cave->ds1.width()/2.f,cave->ds1.height()/2.f);check(!cave->unit_blocked(x,y)&&floor_at(*cave,x,y));std::printf("PASS: Den of Evil nearest free %.2f,%.2f is visible floor\n",x,y);}
 puts("PASS: generated void blocks walkers/missiles; nearest_free finds real floor; rebuilding retains collision");return 0;
}catch(const std::exception&e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
