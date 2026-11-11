// SPDX-License-Identifier: GPL-3.0-or-later
#include "compact_hud.hpp"
#include <cstdio>
#include <stdexcept>
#define check(v) do{if(!(v))throw std::runtime_error("compact HUD layout mismatch at line "+std::to_string(__LINE__));}while(false)
int main()try{
 for(std::int64_t raw=1;raw<=256;++raw)check(d2mini::globe_fill_rows(raw,55*256,true)==2);
 check(d2mini::globe_fill_rows(0,55*256,true)==0);
 check(d2mini::globe_fill_rows(55*256,55*256,true)==80);
 check(d2mini::globe_fill_rows(27*256+128,55*256,true)==40);
 check(d2mini::globe_fill_rows(1,55*256,false)==0);

 {
  const std::array<std::uint8_t,16> circle{0,1,1,0,1,0,0,1,1,2,3,1,0,1,1,0};std::array<int,16> out;out.fill(-1);
  d2mini::globe_pixels(circle,4,4,1,[&](int x,int y,std::uint8_t v){out[y*4+x]=v;});
  check(out[0]==-1&&out[3]==-1&&out[1]==-1&&out[5]==-1&&out[13]==1&&out[14]==1);
 }
 // Compare every output byte with the previous transparent-fill rasterizer,
 // including empty, partial, full and out-of-range fills.
 for(int width:{1,4,80})for(int height:{1,4,80})for(int rows:{-1,0,1,3,40,80,90}){
  std::vector<std::uint8_t> source(std::size_t(width)*height);for(std::size_t i=0;i<source.size();++i)source[i]=i%5?std::uint8_t(i%255+1):0;
  std::vector<int> reference(source.size(),-1),result(source.size(),-1);
  for(int y=0;y<height;++y){int left=width,right=-1;for(int x=0;x<width;++x)if(source[y*width+x]){left=std::min(left,x);right=x;}if(y>=height-rows)for(int x=left;x<=right;++x)if(auto v=source[y*width+x])reference[y*width+x]=v;}
  d2mini::globe_pixels(source,width,height,rows,[&](int x,int y,std::uint8_t v){result[y*width+x]=v;});check(result==reference);
 }
 for(int scale:{100,125,150,175,200,225}) {
  const d2mini::CompactHudLayout l(scale);const auto&a=l.pieces[0].target;const auto&b=l.pieces[1].target;
  check(a.x==0&&b.x+b.w==800&&a.w==b.w&&a.w==148);
  check(l.top==499&&l.outer_x==148&&l.outer_w==504);
  check(l.pieces[7].target.y+l.pieces[7].target.h<a.y);
  check(l.pieces[8].target.y+l.pieces[8].target.h<b.y);
  check(l.pieces[7].target.x+l.pieces[7].target.w/2==139*a.w/234);
  check(l.pieces[8].target.x+l.pieces[8].target.w/2==b.x+93*b.w/234);
  check(l.pieces[5].target.x+l.pieces[5].target.w==l.core_x);
  check(l.pieces[6].target.x==l.core_x+l.core_w);
  check(l.pieces[5].target.w==l.pieces[5].target.h&&l.pieces[6].target.w==l.pieces[6].target.h);
  for(int i=0;i<4;++i){const auto&r=l.belt_slots[i].target;check(r.x==l.core_x+i*84&&r.w==84&&r.h==84);}
  check(l.pieces[4].target.x==l.outer_x&&l.pieces[4].target.w==l.outer_w);
  check(l.quick.empty()&&l.pieces[9].target.x==l.outer_x);
  check(a.x+a.w==l.outer_x&&l.outer_x+l.outer_w==b.x);
  check(l.pieces[9].target.y+l.pieces[9].target.h==l.pieces[5].target.y);
  for(const auto&p:l.pieces) {
   if(p.id==d2mini::HudPart::Mini)continue;
   check(p.target.x>=0&&p.target.y>=0&&p.target.x+p.target.w<=800&&p.target.y+p.target.h<=600);
   const auto hit=l.native(p.target.x+p.target.w/2,p.target.y+p.target.h/2,true,true);
   check(hit.part==p.id&&p.source.contains(hit.x,hit.y));
  }
  for(const auto&p:l.quick){auto hit=l.native(p.target.x+2,p.target.y+2,false,false);check(hit.part==p.id);}
  for(int row=1;row<4;++row){auto p=l.popup(row);auto h=l.native(p.target.x+5,p.target.y+5,false,false,4);check(h.part==d2mini::HudPart::Popup&&p.source.contains(h.x,h.y));}
  check(l.native(400,200,false,false).part==d2mini::HudPart::World);
  check(l.native(400,599,false,false).part==d2mini::HudPart::Bars);
 }
 puts("PASS: single-row skills/belt, smaller globes, continuous bars, raised notifications and pointer mapping");return 0;
}catch(const std::exception&e){std::fprintf(stderr,"%s\n",e.what());return 1;}
