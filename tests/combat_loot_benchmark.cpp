// SPDX-License-Identifier: GPL-3.0-or-later
#define d2mini original
#include "baseline.hpp"
#undef d2mini
#include "candidate.hpp"
#include "sprite_blit.hpp"
#include "shadow_blit.hpp"
#include "frame_pixels.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string_view>
using Frame = std::vector<std::uint8_t>;
using Clock = std::chrono::steady_clock;
using Times = std::array<double,6>;
constexpr int width=800, height=600, actor_w=48, actor_h=64;
struct Scene { const char* name; int actors, shadows, labels, label_width; bool solid=false; };
constexpr Scene scenes[] = {
 {"no-labels",24,24,0,80}, {"quiet",4,4,2,80}, {"reference",24,24,12,80}, {"more-actors",64,24,12,80},
 {"more-shadows",24,64,12,80}, {"more-labels",24,24,48,80},
 {"wide-labels",24,24,12,200}, {"crowded",64,64,48,120}, {"solid-labels",24,24,48,80,true}
};

struct Renderer {
 Frame background=Frame(width*height*4), frame=background, sprite=Frame(actor_w*actor_h);
 std::vector<std::uint16_t> mask=std::vector<std::uint16_t>(width*height);
 d2d::palette::Palette palette;
 Renderer() {
  std::array<d2d::palette::Rgba,256> colours{};
  for(int i=0;i<256;++i) colours[i]={std::uint8_t(i*3),std::uint8_t(i*7),std::uint8_t(i*11),255};
  palette=d2d::palette::Palette(colours);
  for(std::size_t i=0;i<background.size();++i) background[i]=i%4==3?255:std::uint8_t(i*17+i/31);
  for(int y=0;y<actor_h;++y) for(int x=0;x<actor_w;++x)
   if((x-24)*(x-24)+(y-32)*(y-32)/2<480) sprite[y*actor_w+x]=1+(x*11+y*7)%254;
 }
 template<class Composite> Times draw(const Scene& scene,int tick,float fade) {
  const auto start=Clock::now();
  frame=background; std::fill(mask.begin(),mask.end(),0);
  const auto ready=Clock::now();
  const auto x=[&](int i){return 50+(i*73+tick*3)%640;};
  const auto y=[&](int i){return 60+(i*43+tick*2)%440;};
  for(int i=0;i<scene.shadows;++i)
   d2mini::blit_shadow(frame,width,height,sprite,actor_w,actor_h,0,-actor_h,x(i),y(i)+actor_h,mask,i+1);
  const auto shaded=Clock::now();
  for(int i=0;i<scene.actors;++i)
   d2mini::blit_indexed(frame,width,height,sprite,actor_w,actor_h,x(i),y(i),palette,d2mini::SpriteBlend::Opaque,256,254);
  const auto painted=Clock::now();
  for(int i=0;i<scene.labels;++i) {
   const int left=x(i)-20, top=y(i)+25, right=std::min(width,left+scene.label_width);
   Composite blend(frame,width,{left,top,right,top+12},fade,i==0);
   // Mirror the port's selected border and darkened label background.
   // The glyph pattern is synthetic and needs no proprietary font.
   for(int py=0;py<12;++py) for(int px=left;px<right;++px) {
    auto* pixel=&frame[((top+py)*width+px)*4];
    if(i==0 && (py==0 || py==11 || px==left || px==right-1)) {
     pixel[0]=211;pixel[1]=174;pixel[2]=96;
    } else for(int c=0;c<3;++c) pixel[c]/=4;
    if(scene.solid || (py%5!=0 && (px+py)%7<2)) {
     pixel[0]=220;pixel[1]=196;pixel[2]=114;pixel[3]=255;
    }
   }
  }
  const auto labelled=Clock::now();
  if (fade>0) d2mini::restore_opaque_alpha(frame);
  const auto done=Clock::now();
  const auto us=[](auto a,auto b){return std::chrono::duration<double,std::micro>(b-a).count();};
  return {us(start,ready),us(ready,shaded),us(shaded,painted),us(painted,labelled),us(labelled,done),us(start,done)};
 }
};
int main(int argc,char** argv) {
 Renderer a,b;
 const bool verify=argc>1 && std::string_view(argv[1])=="--verify";
 int compared=0;
 if(!verify) std::puts("scene,frame,variant,setup_us,shadows_us,sprites_us,labels_us,alpha_us,total_us");
 for(const auto& scene:scenes) {
  for(int i=0;i<(verify?24:320);++i){
   const float fade=verify?float(i%8)/7.f:1.f;
   Times old{},fresh{};
   if(i%2==0)old=a.draw<original::LootLabelComposite>(scene,i,fade);
   fresh=b.draw<d2mini::LootLabelComposite>(scene,i,fade);
   if(i%2)old=a.draw<original::LootLabelComposite>(scene,i,fade);
   if(a.frame!=b.frame) throw std::runtime_error(scene.name);
   ++compared;
   if(!verify && i>=20) for(int variant:{0,1}) {
    const auto& t=variant?fresh:old;
    std::printf("%s,%d,%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",scene.name,i,variant?"candidate":"baseline",t[0],t[1],t[2],t[3],t[4],t[5]);
   }
  }
 }
 std::fprintf(stderr,"PASS %d complete RGBA frame comparisons\n",compared);
}
