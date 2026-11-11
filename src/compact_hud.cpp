// SPDX-License-Identifier: GPL-3.0-or-later
#include "compact_hud.hpp"
#include "control_hint.hpp"
#include "hud_art.hpp"
#include "common.hpp"
#include "panels.hpp"
#include "skillbar.hpp"
#include "gamemenu.hpp"
#include <inventory.hpp>
#include <cstring>
#include <bit>
namespace d2mini {
using namespace d2d::client;
namespace {
static_assert(std::endian::native==std::endian::little);
template<std::size_t N> constexpr auto expanded_material(const std::uint16_t (&src)[N]){
 std::array<std::uint32_t,N> out{};for(std::size_t i=0;i<N;++i){const auto c=src[i];out[i]=(((c>>11)&31)*255/31)|(((c>>5)&63)*255/63)<<8|((c&31)*255/31)<<16|0xff000000u;}return out;
}
constexpr auto material_slot0=expanded_material(hud_art::slot0);
constexpr auto material_slot1=expanded_material(hud_art::slot1);
constexpr auto material_slot2=expanded_material(hud_art::slot2);
constexpr auto material_slot3=expanded_material(hud_art::slot3);
constexpr auto material_slot4=expanded_material(hud_art::slot4);
constexpr auto material_slot5=expanded_material(hud_art::slot5);
constexpr auto material_attack0=expanded_material(hud_art::attack0);
constexpr auto material_attack1=expanded_material(hud_art::attack1);
constexpr auto material_left_base=expanded_material(hud_art::left_base);
constexpr auto material_right_base=expanded_material(hud_art::right_base);
constexpr auto material_gold=expanded_material(hud_art::gold);
// Only the canonical HUD strip is staged: 576 KiB, no second world framebuffer.
struct HudCanvas {
 std::vector<std::uint8_t> rgba=std::vector<std::uint8_t>(800*180*4);
 void clear(const CompactHudLayout& layout){
  for(const auto& p:layout.pieces){if(p.id==HudPart::Mini)continue;const auto& a=p.source;for(int y=std::max(420,a.y);y<std::min(600,a.y+a.h);++y)std::fill_n(rgba.data()+((y-420)*800+a.x)*4,a.w*4,0);}
 }
 void stage(const d2d::dc6::Frame& f,const d2d::palette::Palette& pal,int x,int y,const HudRect& clip){
  const int x0=std::max({0,clip.x-x,-x}),x1=std::min({int(f.width),clip.x+clip.w-x,800-x});
  const int y0=std::max({0,clip.y-y,420-y}),y1=std::min({int(f.height),clip.y+clip.h-y,600-y});
  for(int j=y0;j<y1;++j)for(int i=x0;i<x1;++i)if(auto n=f.pixels[j*f.width+i])put(x+i,y+j,pal[n]);
 }
 void put(int x,int y,d2d::palette::Rgba c){if(x<0||x>=800||y<420||y>=600)return;auto*p=rgba.data()+((y-420)*800+x)*4;p[0]=c.r;p[1]=c.g;p[2]=c.b;p[3]=255;}
 void sprite(const d2d::dc6::Frame& f,const d2d::palette::Palette& pal,int x,int y,int first_row=0){
  for(int j=first_row;j<int(f.height);++j)for(int i=0;i<int(f.width);++i)if(auto n=f.pixels[j*f.width+i])put(x+i,y+j,pal[n]);
 }
 void bottom(const d2d::dc6::Sprite& s,int frame,const d2d::palette::Palette& pal,int x,int bottom){if(frame<0||unsigned(frame)>=s.frames_per_direction())return;const auto&f=s.frame(0,frame);sprite(f,pal,x,bottom-int(f.height));}
 void item(const d2d::dc6::Frame& f,const d2d::palette::Palette& pal,int x,int y,int w,int h){
  if(!f.width||!f.height)return;
  const int scale=std::min({100,(w-6)*100/int(f.width),(h-6)*100/int(f.height)});
  const int dw=std::max(1,int(f.width)*scale/100),dh=std::max(1,int(f.height)*scale/100);
  for(int j=0;j<dh;++j)for(int i=0;i<dw;++i)if(auto idx=f.pixels[(j*f.height/dh)*f.width+i*f.width/dw])put(x+(w-dw)/2+i,y+(h-dh)/2+j,pal[idx]);
 }
 void blit(std::vector<std::uint8_t>& out,const HudPiece& p)const {
  const auto&a=p.source;const auto&b=p.target;
  // Compute horizontal sampling once, not a division for every pixel.
  std::array<int,800> columns;
  const int left=std::max(0,b.x),right=std::min(800,b.x+b.w);
  for(int x=left;x<right;++x)columns[x]=a.x+(x-b.x)*a.w/b.w;
  for(int y=std::max(0,b.y);y<std::min(600,b.y+b.h);++y){
   const int sy=a.y+(y-b.y)*a.h/b.h;if(sy<420||sy>=600)continue;
   const auto* row=rgba.data()+(sy-420)*800*4;auto* dst=out.data()+y*800*4;
   for(int x=left;x<right;++x){const int sx=columns[x];if(sx<0||sx>=800)continue;
    const auto*src=row+sx*4;if(src[3])std::memcpy(dst+x*4,src,4);
   }
  }
 }
};
}
void draw_compact_hud(std::vector<std::uint8_t>& out,const Scene& s,const SkillBar& skills,const Hud& hud,const LevelButtons& points,const MiniPanel& mini,bool popup,int mx,int my) {
 if(out.size()!=800*600*4)return;
 const d2d::font::TextScaleScope fixed(100);
 static HudCanvas canvas;const CompactHudLayout layout(g_ui_scale);canvas.clear(layout);
 const auto& pal=s.act_pal.entries().empty()?s.pal:s.act_pal;const auto& stats=skills.character.stats;
 const int positions[]{0,165,293,421,549,683};
 for(int i=0;i<6;++i){const auto& f=s.ctrl_panel.frame(0,i);for(const auto& p:layout.pieces)if(p.id!=HudPart::Mini&&p.id!=HudPart::Stats&&p.id!=HudPart::Skills)canvas.stage(f,pal,positions[i],600-int(f.height),p.source);}
 auto globe=[&](int frame,int x,std::int64_t cur,std::int64_t max){
  if(unsigned(frame)>=s.globes.frames_per_direction())return;
  const auto& f=s.globes.frame(0,frame);const int rows=globe_fill_rows(cur,max,frame!=1);
  globe_pixels(f.pixels,int(f.width),int(f.height),rows,[&](int i,int y,std::uint8_t index){canvas.put(x+i,507+y,index?pal[index]:d2d::palette::Rgba{0,0,0,255});});
 };
 globe(hud.poisoned?2:0,29,stats.get(d2d::d2s::kLife),stats.get(d2d::d2s::kMaxLife));
 globe(1,689,stats.get(d2d::d2s::kMana),stats.get(d2d::d2s::kMaxMana));
 canvas.bottom(s.globe_glass,0,pal,28,595);canvas.bottom(s.globe_glass,1,pal,690,591);
 // Experience, run/walk and stamina retain the original authored frames.
 const int lvl=stats.get(d2d::d2s::kLevel);const auto exp=stats.values[d2d::d2s::kExp];
 if(lvl>=1&&std::size_t(lvl)<s.exp_next.size()){
  auto low=s.exp_next[lvl-1],high=s.exp_next[lvl];int fill=high>low?int(std::clamp<std::int64_t>(exp-low,0,high-low)*119/(high-low)):0;
  for(int x=0;x<fill;++x){canvas.put(256+x,562,pal[255]);canvas.put(256+x,563,pal[255]);}
 }
 canvas.bottom(s.run_button,(hud.running?2:0)+(hud.run_down&&over_run_button(mx,my)?1:0),pal,255,590);
 auto stamina=stats.values[d2d::d2s::kStamina],maximum=stats.values[d2d::d2s::kMaxStamina];
 const bool stamina_excess=stamina>maximum+5;if(stamina_excess)maximum=stamina;
 const int fill=maximum>0?int(std::clamp<std::int64_t>(stamina,0,maximum)*102/maximum):0;
 d2d::palette::Rgba colour=stamina_excess?d2d::palette::Rgba{0,0,255,255}:fill<25?d2d::palette::Rgba{210,28,20,255}:d2d::palette::Rgba{244,192,76,255};
 for(int y=573;y<591;++y)for(int x=273;x<273+fill;++x){const auto* previous=canvas.rgba.data()+((y-420)*800+x)*4;auto c=colour;c.r=(3*int(c.r)+previous[0])/4;c.g=(3*int(c.g)+previous[1])/4;c.b=(3*int(c.b)+previous[2])/4;canvas.put(x,y,c);}
 // Skill squares replace the compact menu grid; panel access stays on L2/R2.

 auto icon=[&](int id,int x){const auto* sk=s.skills.get(id);if(!sk)return;int cls=-1;for(int i=0;i<7;++i)if(sk->cls==d2d::rules::kClassCode[i])cls=i;const auto& spr=cls>=0?s.skill_icons[cls]:s.generic_skill_icons;if(sk->icon>=0&&unsigned(sk->icon)<spr.frames_per_direction())canvas.bottom(spr,sk->icon,pal,x,600);};
 icon(skills.left,117);icon(skills.right,635);
 auto point=[&](int x,bool down){canvas.bottom(s.level_socket,0,pal,x,495);canvas.bottom(s.level_button,down?1:0,pal,x+3,491);};
 if(points.stats_x>=0)point(40,points.stats_down);if(points.skills_x>=0)point(727,points.skills_down);
 const auto& items=skills.character.items;const auto& belt=s.belts[std::size_t(d2d::game::belt_index(s,items))];
 for(const auto& item:items)if(item.location==d2d::d2s::item_location::kBelt&&item.column>=0&&item.column<4&&item.column<belt.boxes){
  const auto& b=belt.box[std::size_t(item.column)];if(const auto* spr=s.item_sprite(item);spr&&spr->frames_per_direction()){const auto&f=spr->frame(0,0);canvas.item(f,s.item_pal(item,pal),b[0],b[2],b[1]-b[0]+1,b[3]-b[2]+1);}
 }
 // The central housing is opaque, including any transparent art indices.
 // World tiles can safely omit precisely this rectangle.
 for(int y=layout.top;y<600;++y)for(int x=layout.outer_x;x<layout.outer_x+layout.outer_w;++x){
  auto*p=out.data()+(y*800+x)*4;p[0]=14;p[1]=12;p[2]=11;p[3]=255;
 }
 // One continuous stone housing behind the rows removes transparent seams.
 // All texture and gilded bevel pixels come from the original control panel.
 if(s.ctrl_panel.frames_per_direction()>1){
  const auto& f=s.ctrl_panel.frame(0,1);
  auto pixel=[&](int x,int y,int sx,int sy){if(x<0||x>=800||y<0||y>=600)return;auto idx=f.pixels[sy*f.width+sx];if(!idx)return;auto c=pal[idx];auto*p=out.data()+(y*800+x)*4;p[0]=c.r;p[1]=c.g;p[2]=c.b;p[3]=255;};
  for(int y=layout.top;y<600;++y)for(int x=layout.outer_x;x<layout.outer_x+layout.outer_w;++x)pixel(x,y,22+(x-layout.outer_x)%14,12+(y-layout.top)%36);

 }
 for(const auto&p:layout.pieces){if(p.id==HudPart::Mini||p.id==HudPart::Belt||p.id==HudPart::LeftSkill||p.id==HudPart::RightSkill)continue;if(p.id==HudPart::Stats&&points.stats_x<0)continue;if(p.id==HudPart::Skills&&points.skills_x<0)continue;canvas.blit(out,p);}
 auto material=[&](const std::uint32_t* pixels,int x,int y,int w,int h){for(int j=0;j<h;++j)std::memcpy(out.data()+((y+j)*800+x)*4,pixels+j*w,std::size_t(w)*4);};
 const std::uint32_t* slots[]{material_slot0.data(),material_slot1.data(),material_slot2.data(),material_slot3.data(),material_slot4.data(),material_slot5.data()};
 for(int i=0;i<6;++i)material(slots[i],layout.outer_x+i*84,layout.pieces[5].target.y,84,84);
 auto live_icon=[&](const d2d::dc6::Frame& f,const d2d::palette::Palette& colours,int x,int y,int available){
  if(!f.width||!f.height)return;const int w=std::max(1,int(f.width)*available/int(std::max(f.width,f.height))),h=std::max(1,int(f.height)*available/int(std::max(f.width,f.height)));
  for(int j=0;j<h;++j)for(int i=0;i<w;++i)if(const auto idx=f.pixels[(j*f.height/h)*f.width+i*f.width/w]){const auto c=colours[idx];auto* dst=out.data()+((y+(84-h)/2+j)*800+x+(84-w)/2+i)*4;dst[0]=c.r;dst[1]=c.g;dst[2]=c.b;dst[3]=255;}
 };
 for(int side=0;side<2;++side){const int id=side?skills.right:skills.left;const int x=layout.outer_x+(side?5:0)*84,y=layout.pieces[5].target.y;
  if(id==0)material(side?material_attack1.data():material_attack0.data(),x,y,84,84);
  else if(const auto* sk=s.skills.get(id)){int cls=-1;for(int i=0;i<7;++i)if(sk->cls==d2d::rules::kClassCode[i])cls=i;const auto& spr=cls>=0?s.skill_icons[cls]:s.generic_skill_icons;if(sk->icon>=0&&unsigned(sk->icon)<spr.frames_per_direction())live_icon(spr.frame(0,sk->icon),pal,x,y,68);}
 }
 for(const auto& item:items)if(item.location==d2d::d2s::item_location::kBelt&&item.column>=0&&item.column<4&&item.column<belt.boxes)if(const auto* spr=s.item_sprite(item);spr&&spr->frames_per_direction())live_icon(spr->frame(0,0),s.item_pal(item,pal),layout.core_x+item.column*84,layout.pieces[5].target.y,62);
 // Only the pedestal material is replaced; original figures and globe fills
 // above this band remain the original palette sprites.
 material(material_left_base.data(),45,586,103,14);material(material_right_base.data(),652,586,103,14);

 for(const auto&p:layout.ornaments)canvas.blit(out,p);
 for(const auto&p:layout.quick)canvas.blit(out,p);
 // A single two-pixel authored bevel joins every boundary. Trimmed source
 // rectangles omit the redundant outer seams, retaining the slot dividers.
 if(s.ctrl_panel.frames_per_direction()>1){
  const auto& f=s.ctrl_panel.frame(0,1);
  auto edge=[&](int x,int y,int sx,int sy){if(x<0||x>=800||y<0||y>=600)return;auto idx=f.pixels[sy*f.width+sx];if(!idx)return;auto c=pal[idx];auto*p=out.data()+(y*800+x)*4;p[0]=c.r;p[1]=c.g;p[2]=c.b;p[3]=255;};
  auto horizontal=[&](int x,int y,int w){for(int j=0;j<2;++j)for(int i=0;i<w;++i)edge(x+i,y+j,1+i%126,7+j);};
  horizontal(layout.outer_x,layout.top,layout.outer_w);horizontal(layout.outer_x,598,layout.outer_w);
  horizontal(layout.upper_x,layout.pieces[9].target.y-1,layout.upper_w);
  horizontal(layout.upper_x,layout.pieces[5].target.y-1,layout.upper_w);
  horizontal(layout.outer_x,layout.pieces[4].target.y-1,layout.outer_w);

  for(int y=layout.top;y<600;++y)for(int i=0;i<2;++i){edge(layout.outer_x+i,y,1+i,7+(y-layout.top)%48);edge(layout.outer_x+layout.outer_w-1-i,y,1+i,7+(y-layout.top)%48);}
 }
 // Mirror the completed left ornament, including its bevel, pixel for
 // pixel. Reversing source-piece order alone does not mirror the artwork.
 for(const auto& p:layout.ornaments)for(int y=p.target.y;y<p.target.y+p.target.h;++y)
  for(int x=p.target.x;x<p.target.x+p.target.w;++x)
   std::memcpy(out.data()+(y*800+799-x)*4,out.data()+(y*800+x)*4,4);
 material(material_gold.data(),0,598,800,2);
 // Extra belt rows fan above the quick-panel row. Their native coordinates
 // overlap that row, so render them separately instead of into the strip.
 if(popup)for(int row=1;row<belt.boxes/4;++row){
  const auto p=layout.popup(row);const auto& a=p.source;
  for(int y=a.y;y<a.y+a.h;++y)for(int x=a.x;x<a.x+a.w;++x){auto*q=canvas.rgba.data()+((y-420)*800+x)*4;std::fill_n(q,4,0);}
  canvas.bottom(s.popbelt,0,pal,421,592-row*32);
  for(const auto& item:items)if(item.location==d2d::d2s::item_location::kBelt&&item.column/4==row&&item.column<belt.boxes){const auto&b=belt.box[item.column];if(const auto* spr=s.item_sprite(item);spr&&spr->frames_per_direction()){const auto& f=spr->frame(0,0);canvas.item(f,s.item_pal(item,pal),b[0],b[2],b[1]-b[0]+1,b[3]-b[2]+1);}}
  canvas.blit(out,p);
 }
 if(popup){
  const int rows=belt.boxes/4;const int top=rows>1?layout.popup(rows-1).target.y:layout.pieces[5].target.y;
  const auto& font=s.readable_font.glyph_count()?s.readable_font:s.font;
  const std::string hint="D-PAD: Select   Y: Use   A: Back";const int width=font.measure(hint),x=std::max(0,(800-width)/2),y=std::max(0,top-font.line_height()-7);
  for(int j=y-2;j<y+font.line_height()+2;++j)for(int i=x-4;i<x+width+4;++i)if(i>=0&&i<800&&j>=0&&j<600){auto*p=out.data()+(j*800+i)*4;p[0]=p[1]=p[2]=0;p[3]=255;}
  draw_control_hint(font,out,pal,x,y,hint);
  for(int row=0;row<rows;++row)for(int col=0;col<4;++col){auto b=row?layout.popup(row).target:layout.belt_slots[col].target;b.x=layout.core_x+col*84;b.w=84;
   if(!b.contains(mx,my))continue;
   for(int j=b.y;j<b.y+b.h;++j)for(int i=b.x;i<b.x+b.w;++i)if((j<b.y+2||j>=b.y+b.h-2||i<b.x+2||i>=b.x+b.w-2)&&i>=0&&i<800&&j>=0&&j<600){auto*p=out.data()+(j*800+i)*4;p[0]=224;p[1]=190;p[2]=103;p[3]=255;}
  }
 }
 if(mx>=29&&mx<110&&my>=507&&my<588){auto t=std::to_string(stats.values[d2d::d2s::kLife]>>8)+" / "+std::to_string(stats.values[d2d::d2s::kMaxLife]>>8);s.font.draw(out,800,600,pal,layout.pieces[0].target.w/2-s.font.measure(t)/2,layout.top-22,t);}
 if(mx>=689&&mx<770&&my>=507&&my<588){auto t=std::to_string(stats.values[d2d::d2s::kMana]>>8)+" / "+std::to_string(stats.values[d2d::d2s::kMaxMana]>>8);s.font.draw(out,800,600,pal,800-layout.pieces[1].target.w/2-s.font.measure(t)/2,layout.top-22,t);}
}
}
