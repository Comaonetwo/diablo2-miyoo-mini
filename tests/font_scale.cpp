// SPDX-License-Identifier: GPL-3.0-or-later
#include <font.hpp>
#include "text_layout.hpp"
#include <cstdio>
#include <cstring>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("font scale failed");}
int main()try{
 std::vector<std::byte>dc6(140),tbl(12+256*14),palette(768);
 auto put=[&](unsigned a,uint32_t n){std::memcpy(dc6.data()+a,&n,4);};
 put(0,6);put(16,1);put(20,1);put(24,28);put(32,8);put(36,8);put(56,80);
 for(unsigned y=0;y<8;++y){dc6[60+y*10]=std::byte{8};for(unsigned x=0;x<8;++x)dc6[61+y*10+x]=std::byte{1};dc6[69+y*10]=std::byte{0x80};}
 std::memcpy(tbl.data(),"Woo!\1",5);tbl[12+65*14]=std::byte{65};tbl[12+65*14+3]=std::byte{8};tbl[12+65*14+4]=std::byte{3};
 palette[3]=palette[4]=palette[5]=std::byte{255};
 d2d::font::Font font(tbl,d2d::dc6::Sprite(dc6));d2d::palette::Palette pal(palette);
 check(font.measure("AA")==16&&font.line_height()==8);const auto old_height=font.line_height();
 d2d::font::g_text_scale=125;check(font.measure("AA")==20&&font.line_height()==(old_height*125+50)/100);
 std::vector<uint8_t>fb(64*64*4);check(font.draw(fb,64,64,pal,40,40,"A")==50);
 unsigned count=0;for(unsigned i=0;i<64*64;++i)if(fb[i*4])++count;check(count==100);
 font.draw(fb,64,64,pal,-5,60,"A");d2d::font::g_text_scale=100;check(font.measure("AA")==16);
 palette[3]=palette[4]=palette[5]=std::byte{40};d2d::palette::Palette dim(palette);
 std::fill(fb.begin(),fb.end(),90);
 {const d2d::font::TextContrastScope readable(true);check(font.measure("AA")==16);check(font.draw(fb,64,64,dim,4,4,"A")==12);check(fb[(4*64+4)*4]==60);check(fb[(12*64+12)*4]==0);}
 check(!d2d::font::g_readable_text);font.draw(fb,64,64,dim,20,20,"A");check(fb[(20*64+20)*4]==40);
 for(int percent:{100,125,150,175,200,225}){
  const auto wrapped=d2mini::fit_text(font,"AAAA AAAA AAAA AAAA",40,48,percent,75,true);
  check(wrapped.fits&&wrapped.width<=40&&wrapped.height<=48);
  const auto single=d2mini::fit_text(font,"AAAAAAAAAAAA",100,24,percent,75,false);
  check(single.fits&&single.lines.size()==1&&single.width<=100&&single.height<=24);
 }
 // Coverage is opacity, not an opaque grey rectangle over the panel.
 for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x)dc6[61+y*10+x]=std::byte{128};
 d2d::font::Font coverage(tbl,d2d::dc6::Sprite(dc6),true);
 std::fill(fb.begin(),fb.end(),40);
 coverage.draw_tinted(fb,64,64,pal,0,0,"A",200,100,50);
 check(fb[0]==120&&fb[1]==70&&fb[2]==45);
 check(fb[8*4]>40); // Forum keeps its bold edge outside a TextWeightScope.
 check(coverage.ink_width("A")==9);
 {const d2d::font::TextScaleScope scale(75);
  std::fill(fb.begin(),fb.end(),40);coverage.draw_tinted(fb,64,64,pal,0,0,"A",200,100,50);
  check(fb[0]==120&&fb[1]==70&&fb[2]==45);check(fb[(6*64+6)*4]==40);
 }
 // A narrow stroke between nearest samples must still contribute to a reduced glyph.
 for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x)dc6[61+y*10+x]=std::byte{x==1?std::uint8_t(255):std::uint8_t(0)};
 d2d::font::Font thin(tbl,d2d::dc6::Sprite(dc6),true);
 {const d2d::font::TextScaleScope scale(50);std::fill(fb.begin(),fb.end(),0);thin.draw(fb,64,64,pal,0,0,"A");check(fb[0]==128);}
 // Deferred panel text must own its label and equal a single final-size draw.
 std::vector<uint8_t> deferred(800*600*4,40),direct=deferred;
 {const d2d::font::PanelTextScope panel(true,false);const d2d::font::TextScaleScope scale(80);const d2d::font::TextContrastScope contrast(true);
  std::string label="A";coverage.draw(deferred,800,600,pal,88,68,label);label="B";
  check(deferred==direct&&d2d::font::g_pending_panel_text.size()==1);
 }
 d2d::font::draw_pending_panel_text(deferred);
 {const d2d::font::TextScaleScope scale(100);const d2d::font::TextContrastScope contrast(true);coverage.draw_tinted(direct,800,600,pal,10,10,"A",255,255,255,0,600);}
 check(deferred==direct&&d2d::font::g_pending_panel_text.empty());
 // Original palette fonts defer too, preserving their palette and actual scale.
 std::fill(deferred.begin(),deferred.end(),40);direct=deferred;
 {const d2d::font::PanelTextScope panel(true,true);font.draw(deferred,800,600,dim,408,68,"A");check(deferred==direct&&d2d::font::g_pending_panel_text.size()==1);}
 d2d::font::draw_pending_panel_text(deferred);
 {const d2d::font::PanelGlyphScaleScope scale(125);font.draw_tinted(direct,800,600,dim,410,10,"A",255,255,255,0,600);}
 check(deferred==direct&&d2d::font::g_pending_panel_text.empty()&&d2d::font::g_panel_glyph_scale==0);
 {const d2d::font::PanelTextScope panel(false,true);coverage.draw(direct,800,600,pal,410,80,"A");check(d2d::font::g_pending_panel_text.empty());}
 {const d2d::font::PanelTextScope left(true,false);font.draw(direct,800,600,pal,88,68,"A");}
 {const d2d::font::PanelTextScope right(true,true);font.draw(direct,800,600,pal,408,68,"A");}
 check(d2d::font::g_pending_panel_text.size()==2);d2d::font::cover_panel_text(false);
 check(d2d::font::g_pending_panel_text.size()==1&&d2d::font::g_pending_panel_text[0].transform.sx==400);d2d::font::draw_pending_panel_text(direct);
 const auto long_name=d2mini::fit_text(font,std::string(200,'A'),100,24,225,75,false);
 check(long_name.fits&&long_name.width<=100&&long_name.lines.front().size()<200);
 std::puts("PASS: scaled metrics, glyph extent, framebuffer clipping, restore");return 0;
}catch(const std::exception&e){std::fprintf(stderr,"%s\n",e.what());return 1;}
