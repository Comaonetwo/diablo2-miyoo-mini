// SPDX-License-Identifier: GPL-3.0-or-later
#include "world_view.hpp"
#include "tile_blit_015.hpp"
#include "fb_convert.hpp"
#include "fb_convert_015.hpp"
#include <cstdio>
#include <cstdlib>
#include <string_view>
using namespace d2d::client;
int main(int argc,char** argv){
 if(argc>1&&std::string_view(argv[1]).starts_with("present-")){
  const bool old=std::string_view(argv[1])=="present-before";const int n=argc>2?std::atoi(argv[2]):2;
  std::vector<std::uint8_t> src(800*600*4),out(640*480*4);for(std::size_t i=0;i<src.size();++i)src[i]=std::uint8_t(i%251);
  const d2mini::FrameFormat f{640,480,2560,32,{16,8},{8,8},{0,8},{24,8}};
  const scanout015::FrameFormat b{640,480,2560,32,{16,8},{8,8},{0,8},{24,8}};
  puts("WORKLOAD_BEGIN");fflush(stdout);for(int i=0;i<n;++i){if(old)scanout015::convert_frame(src,800,600,out,b,true);else d2mini::convert_frame(src,800,600,out,f,true);}puts("WORKLOAD_END");fflush(stdout);
  std::uint64_t hash=0;for(auto v:out)hash=hash*131+v;std::printf("checksum=%llu\n",(unsigned long long)hash);return 0;
 }
 const bool before=argc>1&&std::string_view(argv[1])=="before";const int rounds=argc>2?std::atoi(argv[2]):20;
 std::array<d2d::palette::Palette,32> pals;for(int l=0;l<32;++l){std::array<d2d::palette::Rgba,256> e;for(int i=0;i<256;++i)e[i]={std::uint8_t(i*l/31),std::uint8_t((255-i)*l/31),std::uint8_t((i*17%256)*l/31),255};pals[l]=d2d::palette::Palette(e);}
 Lighting light;light.pal=&pals;light.grid.reset(0,0,0);for(int y=0;y<light.grid.grid_size;++y)for(int x=0;x<light.grid.grid_size;++x)light.grid.values[y*light.grid.grid_size+x]=std::uint8_t((x*19+y*37)%256);
 d2d::dt1::Tile tile;tile.width=160;tile.height=-128;tile.pixels.resize(160*128);for(std::size_t i=0;i<tile.pixels.size();++i)tile.pixels[i]=(i%160>=25&&i%160<135)?std::uint8_t(i%255+1):0;tile.compact();tile.ensure_pixels();
 std::vector<std::uint8_t> fb(800*600*4,37);g_fast_render=true;
 puts("WORKLOAD_BEGIN");fflush(stdout);
 for(int i=0;i<rounds;++i){if(before)reference015::blit_dt1_tile_lit(fb,tile,light,300,200,0,0,380,248,true,255,nullptr);else blit_dt1_tile_lit(fb,tile,light,300,200,0,0,380,248,true,255,nullptr);}
 puts("WORKLOAD_END");fflush(stdout);std::uint64_t hash=0;for(auto b:fb)hash=hash*131+b;std::printf("checksum=%llu\n",(unsigned long long)hash);
}
