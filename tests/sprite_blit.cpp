// SPDX-License-Identifier: GPL-3.0-or-later
#include "sprite_blit.hpp"
#include <vector>
#include <cstdio>
#include <stdexcept>
#include <chrono>
#include <string_view>
using d2mini::SpriteBlend;
void reference(std::vector<std::uint8_t>& out,int sw,int sh,const std::vector<std::uint8_t>& src,int w,int h,int dx,int dy,const d2d::palette::Palette& pal,SpriteBlend mode,int shade){
 for(int y=0;y<h;++y){const int yy=y+dy;if(yy<0||yy>=sh)continue;for(int x=0;x<w;++x){const int xx=x+dx;if(xx<0||xx>=sw)continue;auto n=src[y*w+x];if(!n)continue;auto c=pal[n];auto*p=out.data()+(yy*sw+xx)*4;
 if(mode==SpriteBlend::Palette){p[0]=std::uint8_t(std::min(255,c.r*shade/256));p[1]=std::uint8_t(std::min(255,c.g*shade/256));p[2]=std::uint8_t(std::min(255,c.b*shade/256));p[3]=c.a;}
 else{if(mode==SpriteBlend::Opaque){p[0]=c.r;p[1]=c.g;p[2]=c.b;}
 else if(mode==SpriteBlend::Add){p[0]=std::uint8_t(std::min(255,p[0]+c.r));p[1]=std::uint8_t(std::min(255,p[1]+c.g));p[2]=std::uint8_t(std::min(255,p[2]+c.b));}
 else if(mode==SpriteBlend::Multiply){p[0]=std::uint8_t(p[0]*c.r/255);p[1]=std::uint8_t(p[1]*c.g/255);p[2]=std::uint8_t(p[2]*c.b/255);}
 else{int a=mode==SpriteBlend::Quarter?1:mode==SpriteBlend::Half?2:3;p[0]=std::uint8_t((p[0]*(4-a)+c.r*a)/4);p[1]=std::uint8_t((p[1]*(4-a)+c.g*a)/4);p[2]=std::uint8_t((p[2]*(4-a)+c.b*a)/4);}p[3]=255;}
 }}
}
int main(int argc,char** argv)try{
 const bool quick=argc>1&&std::string_view(argv[1])=="--quick";
 std::array<d2d::palette::Rgba,256> colours;for(int i=0;i<256;++i)colours[i]={std::uint8_t(i),std::uint8_t(i*7),std::uint8_t(255-i),std::uint8_t(i*3)};
 const d2d::palette::Palette pal(colours);std::vector<std::uint8_t> src(73*91);for(std::size_t i=0;i<src.size();++i)src[i]=i%3?std::uint8_t(i%255+1):0;
 int cases=0;for(int mode=0;mode<7;++mode)for(int shade:{0,128,256,384})for(int dx:{-100,-20,0,32,110,200})for(int dy:{-120,-12,0,40,90,200})for(int clip=0;clip<3;++clip){
 if(quick&&(mode!=2||shade!=256||dx < -20||dx>32||dy < -12||dy>40))continue;
 std::vector<std::uint8_t> expected(127*103*4),actual;for(std::size_t i=0;i<expected.size();++i)expected[i]=std::uint8_t(i*31+17);actual=expected;
 reference(expected,127,103,src,73,91,dx,dy,pal,SpriteBlend(mode),shade);
 d2mini::WorldOcclusion c;if(clip==1)c={50,50,90,103};if(clip==2){c.clip_left=10;c.clip_top=5;c.clip_right=95;c.clip_bottom=90;}
 {const d2mini::WorldOcclusionScope scope(c);d2mini::blit_indexed(actual,127,103,src,73,91,dx,dy,pal,SpriteBlend(mode),shade);}
 if(clip)for(int y=0;y<103;++y)for(int x=0;x<127;++x)if(c.hides(x,y,1,1))for(int k=0;k<4;++k)expected[(y*127+x)*4+k]=std::uint8_t(((y*127+x)*4+k)*31+17);
 if(actual!=expected)throw std::runtime_error("sprite mismatch mode="+std::to_string(mode));++cases;
 }
 std::printf("PASS: %d full-buffer sprite cases, palette alpha, all blends/shades, clipping and HUD/viewport masks\n",cases);
 if(argc>1&&!quick)for(auto mode:{SpriteBlend::Palette,SpriteBlend::Opaque,SpriteBlend::Add}){
  double ms[2]{};std::uint64_t sums[2]{};
  for(int old=0;old<2;++old){std::vector<std::uint8_t> out(800*600*4,37);const auto t=std::chrono::steady_clock::now();for(int i=0;i<20000;++i){if(old)reference(out,800,600,src,73,91,300,250,pal,mode,256);else d2mini::blit_indexed(out,800,600,src,73,91,300,250,pal,mode);}ms[old]=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count();for(auto b:out)sums[old]=sums[old]*131+b;}
  if(sums[0]!=sums[1])throw std::runtime_error("benchmark mismatch");std::printf("BENCH mode=%d old_ms=%.3f new_ms=%.3f ratio=%.3f\n",int(mode),ms[1],ms[0],ms[1]/ms[0]);
 }
 return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
