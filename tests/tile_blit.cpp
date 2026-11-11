// SPDX-License-Identifier: GPL-3.0-or-later
#include "world_view.hpp"
#include <mpq.hpp>
#include <txt.hpp>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <set>
#include <stdexcept>
#include "tile_blit_reference.hpp"
using namespace d2d::client;
namespace {
std::array<d2d::palette::Palette,32> palettes;
Lighting light;
const Hole cutout{400,300,70};
std::size_t comparisons=0;
void draw(bool old,std::vector<std::uint8_t>& fb,const d2d::dt1::Tile& tile,int x,int y,int mode) {
 const auto& p=palettes[19];
 if(mode==0){if(old)reference::blit_dt1_tile(fb,tile,p,x,y,255,nullptr);else blit_dt1_tile(fb,tile,p,x,y);}
 else if(mode==1){if(old)reference::blit_dt1_shadow(fb,tile,p,x,y);else blit_dt1_shadow(fb,tile,p,x,y);}
 else if(mode==2){if(old)reference::blit_dt1_tile(fb,tile,p,x,y,128,&cutout);else blit_dt1_tile(fb,tile,p,x,y,128,&cutout);}
 else {g_fast_render=mode<5;const bool floor=mode%2;const int alpha=mode==7?128:255;const Hole* h=mode==7?&cutout:nullptr;
  if(old)reference::blit_dt1_tile_lit(fb,tile,light,x,y,0,0,x+80,y+48,floor,alpha,h);
  else blit_dt1_tile_lit(fb,tile,light,x,y,0,0,x+80,y+48,floor,alpha,h);
 }
}
void verify(const d2d::dt1::Tile& tile,int width=800) {
 if(tile.width<=0||tile.height==0)return;
 // Clipping on all sides; fully offscreen; dense/sparse pixels; negative heights.
 const int positions[][2]={{width<800?5:300,250},{-73,80},{width-30,100},{width<800?5:200,-37},{width<800?5:200,580},{width+100,10},{20,700}};
 std::vector<std::uint8_t> before(width*600*4),after;
 for(const auto& pos:positions)for(int mode=0;mode<8;++mode){
  std::fill(before.begin(),before.end(),37);after=before;
  draw(true,before,tile,pos[0],pos[1],mode);draw(false,after,tile,pos[0],pos[1],mode);
  if(before!=after)throw std::runtime_error("tile pixels differ at mode "+std::to_string(mode)+" position "+std::to_string(pos[0])+","+std::to_string(pos[1]));
  ++comparisons;
 }
}
void verify_occlusion(const d2d::dt1::Tile& tile) {
 const int positions[][2]={{160,450},{300,400},{300,450},{550,450},{300,580}};
 for(auto& pos:positions)for(int mode=0;mode<8;++mode){
  std::vector<std::uint8_t> expected(800*600*4,37),actual=expected;
  draw(true,expected,tile,pos[0],pos[1],mode);
  {
   const d2mini::WorldOcclusionScope scope({205,420,595,600});
   draw(false,actual,tile,pos[0],pos[1],mode);
  }
  if(d2mini::world_occlusion.left!=0)throw std::runtime_error("occlusion leaked outside world scope");
  for(int y=420;y<600;++y)std::fill(expected.begin()+(y*800+205)*4,expected.begin()+(y*800+595)*4,37);
  if(expected!=actual)throw std::runtime_error("visible tile pixels changed with HUD occlusion at mode "+std::to_string(mode));
  ++comparisons;
 }
}
void bench(const std::vector<d2d::dt1::Tile>& tiles,int rounds){
 for(int mode:{0,1,3,4,5,6}){
  double elapsed[2]{};std::uint64_t sum[2]{};
  // Identical input, final framebuffer checksum checked for both paths.
  for(int old=0;old<2;++old){std::vector<std::uint8_t> fb(800*600*4,37);auto start=std::chrono::steady_clock::now();
   for(int r=0;r<rounds;++r)for(const auto&t:tiles)draw(old,fb,t,300,200,mode);
   elapsed[old]=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
   for(auto b:fb)sum[old]=sum[old]*131+b;
  }
  if(sum[0]!=sum[1])throw std::runtime_error("benchmark framebuffer differs");
  std::printf("BENCH mode=%d tiles=%zu rounds=%d old_ms=%.3f new_ms=%.3f ratio=%.3f checksum=%llu\n",mode,tiles.size(),rounds,elapsed[1],elapsed[0],elapsed[1]/elapsed[0],(unsigned long long)sum[0]);
 }
}
}
int main(int argc,char**argv)try{
 for(int level=0;level<32;++level){std::array<d2d::palette::Rgba,256> e{};for(int i=0;i<256;++i)e[i]={std::uint8_t(i*level/31),std::uint8_t((255-i)*level/31),std::uint8_t((i*17%256)*level/31),std::uint8_t(i?255:0)};palettes[level]=d2d::palette::Palette(e);}
 light.pal=&palettes;light.grid.reset(0,0,0);
 for(int y=0;y<light.grid.grid_size;++y)for(int x=0;x<light.grid.grid_size;++x)light.grid.values[y*light.grid.grid_size+x]=std::uint8_t((x*19+y*37)%256);
 std::vector<d2d::dt1::Tile> tiles;
 const bool arm_quick=argc>1&&std::string(argv[1])=="--arm-quick";
 for(int variant=0;variant<5;++variant){d2d::dt1::Tile t;t.width=160;t.height=-128;t.pixels.resize(160*128);
  for(std::size_t i=0;i<t.pixels.size();++i)t.pixels[i]=variant==0?0:variant==1?std::uint8_t(i%255+1):variant==2?std::uint8_t(i%2):variant==3?(i%41==0?19:0):((i%160>25&&i%160<130)?std::uint8_t(i%255+1):0);
  verify(t,arm_quick?80:800);t.compact();verify(t,arm_quick?80:800);tiles.push_back(std::move(t));
 }
 if(!arm_quick)for(const auto&t:tiles){verify(t,1024);verify_occlusion(t);}
 auto varied=light.grid.values;light.grid.reset(0,0,64);for(const auto&t:tiles)verify(t,arm_quick?80:800);light.grid.values=varied;
 if(argc>1&&!arm_quick&&std::string(argv[1])!="--synthetic"){d2d::mpq::Stack stack;std::filesystem::path data(argv[1]);stack.push_installer((data/"LODPatch_114d.exe").string());stack.push((data/"d2exp.mpq").string());stack.push((data/"d2data.mpq").string());
  const d2d::txt::Table types(stack.read(R"(data\global\excel\LvlTypes.txt)"));std::set<std::string> files{R"(Act1\Town\floor.dt1)"};
  for(std::size_t i=0;i<types.size();++i){int id=std::atoi(std::string(types.get(i,"Id")).c_str());if(id<1||id>4)continue;for(int slot=1;slot<=32;++slot){auto f=types.get(i,"File "+std::to_string(slot));if(!f.empty()&&f!="0")files.emplace(f);}}
  for(auto file:files){std::replace(file.begin(),file.end(),'/','\\');const d2d::dt1::Archive a(stack.read(R"(data\global\tiles\)"+file),d2d::dt1::Pixels::compact);for(const auto&t:a.tiles()){verify(t);verify_occlusion(t);tiles.push_back(t);}}
  std::printf("REAL files=%zu tiles=%zu\n",files.size(),tiles.size()-5);
 }
 std::printf("PASS: %zu full-frame pixel comparisons against reference, including opaque HUD clipping\n",comparisons);
 bench(tiles,argc>2?std::atoi(argv[2]):10);return 0;
}catch(const std::exception&e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
