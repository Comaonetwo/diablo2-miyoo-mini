// SPDX-License-Identifier: GPL-3.0-or-later
#include "panel_zoom.hpp"
#include "port_ui.hpp"
#include "common.hpp"
#include "control_hint.hpp"
#include <cstring>
#include <screenshot.hpp>
namespace d2mini {
using namespace d2d::client;
std::string inspect_hud_assets(const Scene& s,const std::filesystem::path& dir) {
 std::filesystem::create_directories(dir);std::string out;
 const auto& pal=s.act_pal.entries().empty()?s.pal:s.act_pal;
 const std::pair<const char*,const d2d::dc6::Sprite*> assets[]{
 {"ctrl",&s.ctrl_panel},{"globes",&s.globes},{"glass",&s.globe_glass},{"mini",&s.minipanel},{"mini-button",&s.minipanel_btn},{"menu",&s.menu_button},{"level-socket",&s.level_socket},{"level-button",&s.level_button},{"run",&s.run_button},{"popbelt",&s.popbelt}};
 for(auto [name,spr]:assets)for(unsigned i=0;i<spr->frames_per_direction();++i){
  const auto& f=spr->frame(0,i);std::vector<std::uint8_t> rgba(std::size_t(f.width)*f.height*4);
  for(std::size_t j=0;j<f.pixels.size();++j)if(auto idx=f.pixels[j]){auto c=pal[idx];rgba[j*4]=c.r;rgba[j*4+1]=c.g;rgba[j*4+2]=c.b;rgba[j*4+3]=255;}
  auto n=std::string(name)+"-"+std::to_string(i);
  d2d::screenshot::save_png(dir/(n+".png"),rgba,f.width,f.height);
  out+=n+" "+std::to_string(f.width)+"x"+std::to_string(f.height)+" offset="+std::to_string(f.offset_x)+","+std::to_string(f.offset_y)+"\n";
 }
 return out+"ok\n";
}
namespace {
std::array<std::string_view,22> control_lines(bool belt_sequential){
 return {"D-PAD: Move / select","B: Attack / talk / confirm","Y: Loot / portal / entrance / item","X: Right skill   A: Back / skills",belt_sequential?"L: Slots left to right; R: right to left":"L: Heal   R: Mana (any belt slot)","SELECT + L / R: Belt slots 3 / 4","SELECT + Y: Move inventory item to belt","SELECT + X: Drop inventory item","SELECT + Y: Swap weapons in the field","L2: Stats   R2: Inventory","SELECT + UP: Skill tree","SELECT + DOWN: Quests","SELECT + LEFT: Expanded belt","Belt: D-pad select; Y use; X to inventory","SELECT + RIGHT: Mercenary","SELECT + X: Run / walk","Tap SELECT: Map   Hold 1s: Cursor","Y: Buy / sell in shop  L/R: Shop tabs","SELECT + Y: Fill consumables in shop","SELECT + START: Arrange inventory",
  "MENU: Port menu","MENU + SELECT: Exit game"};
}
constexpr int control_visible=8;
constexpr int option_count=14;
}
void PortUI::input(const pad::Sample& sample,std::span<const SDL_Keycode> keys,float seconds) {
    changed=false;quit=false;return_menu=false;
    if(closing){if(SDL_GetTicks()-fade_start>=100){open=false;quit=close_quit;return_menu=close_return;}previous=sample.held;return;}
    auto pressed=[&](pad::Button b){return sample[b]&&!previous[std::size_t(b)];};
    auto key=[&](SDL_Keycode k){return std::ranges::find(keys,k)!=keys.end();};
    const pad::Button dirs[]{pad::Button::DpadUp,pad::Button::DpadDown,pad::Button::DpadLeft,pad::Button::DpadRight};
    std::array<bool,4> step{};
    for(unsigned i=0;i<4;++i) {
        if(!sample[dirs[i]])repeats[i]=0;
        else if(pressed(dirs[i])){step[i]=true;repeats[i]=.35f;}
        else if((repeats[i]-=seconds)<=0){step[i]=true;repeats[i]=.12f;}
    }
    step[0]|=key(SDLK_UP);step[1]|=key(SDLK_DOWN);step[2]|=key(SDLK_LEFT);step[3]|=key(SDLK_RIGHT);
    const bool accept=pressed(pad::Button::South)||key(SDLK_RETURN);
    const bool back=pressed(pad::Button::East)||key(SDLK_ESCAPE);
    if(controls){if(step[0])scroll=std::max(0,scroll-1);if(step[1])scroll=std::min(int(control_lines(belt_sequential).size())-control_visible,scroll+1);if(back){controls=false;row=1;scroll=0;}previous=sample.held;return;}
    if(back){if(options){options=false;row=0;}else request_close();previous=sample.held;return;}
    const int count=options?option_count:5;
    if(step[0])row=(row+count-1)%count;if(step[1])row=(row+1)%count;
    if(!options){
        if(accept){if(row==0){options=true;row=0;scroll=0;}else if(row==1){controls=true;scroll=0;}else if(row==2)request_close();else if(row==3)request_close(false,true);else request_close(true,false);}
        previous=sample.held;return;
    }
    const int direction=step[2]?-1:(step[3]||accept)?1:0;
    if(direction){
        if(row==0||row==1){int& value=row==0?text_scale:ui_scale;value=value>100?100:(row==0?200:125);}
        else if(row==2)compact=!compact;
        else if(row==3)sound=!sound;
        else if(row==4)world_big=(world_big+direction+3)%3;
        else if(row==5)dim_loot=!dim_loot;
        else if(row==6)map_opacity=((map_opacity/25-1+direction+4)%4+1)*25;
        else if(row==7)readable_font=!readable_font;
        else if(row==8)belt_sequential=!belt_sequential;
        else if(row==9)belt_refill=!belt_refill;
        else if(row==10)sfx_volume=std::clamp(sfx_volume+direction*5,0,100);
        else if(row==11)music_volume=std::clamp(music_volume+direction*5,0,100);
        else if(row==12)smooth_camera=!smooth_camera;
        else if(row==13)fast=!fast;
        smooth=sharp_panel_text=true;changed=true;
    }
    previous=sample.held;
}
namespace {
void rect(std::vector<std::uint8_t>& f,int x,int y,int w,int h,std::array<std::uint8_t,3> c) {
 for(int j=std::max(0,y);j<std::min(600,y+h);++j)for(int i=std::max(0,x);i<std::min(800,x+w);++i){auto*p=f.data()+(j*800+i)*4;p[0]=c[0];p[1]=c[1];p[2]=c[2];p[3]=255;}
}
}
void PortUI::capture(const std::vector<std::uint8_t>& frame){
 backdrop=frame;if(backdrop.size()!=800*600*4){backdrop.clear();return;}

}
void PortUI::draw(std::vector<std::uint8_t>& f,const Scene& scene) const {
 const d2d::font::TextScaleScope fixed(100);
 const d2d::font::TextWeightScope medium(!scene.use_readable());
 // Keep the original glyph shading. A white palette made every nonzero
 // texel opaque white, including the dark outline and fine inner detail.
 const d2d::font::TextContrastScope original_weight(false);
 const d2d::palette::Palette pal=[&]{
  std::array<d2d::palette::Rgba,256> entries;
  for(int i=0;i<256;++i){const auto c=scene.pal[i];const int base=std::max({int(c.r),int(c.g),int(c.b)});const int light=std::min(255,(base*340+98)/196);entries[i]={std::uint8_t(light),std::uint8_t(light),std::uint8_t(light),255};}
  return d2d::palette::Palette(entries);
 }();
 const auto& font=scene.readable(scene.font24.line_height()?scene.font24:scene.font);
 const auto& footer=scene.readable_font.glyph_count()?scene.readable_font:scene.font;
 if(backdrop.size()==f.size()){f=backdrop;for(std::size_t i=0;i<f.size();i+=4)for(int c=0;c<3;++c)f[i+c]=std::uint8_t(int(f[i+c])*3/10);}else rect(f,0,0,800,600,{12,10,9});
 struct FadeFrame {std::vector<std::uint8_t>& f;const std::vector<std::uint8_t>& bg;int a;~FadeFrame(){if(a>=255||f.size()!=bg.size())return;for(std::size_t i=0;i<f.size();i+=4)for(int c=0;c<3;++c)f[i+c]=std::uint8_t((int(f[i+c])*a+int(bg[i+c])*(255-a)+127)/255);}} fade{f,backdrop,int(fade_alpha()*255)};
 rect(f,36,30,728,3,{137,112,70});rect(f,36,567,728,3,{137,112,70});rect(f,36,30,3,540,{137,112,70});rect(f,761,30,3,540,{137,112,70});
 font.draw_tinted(f,800,600,pal,72,58,controls?"CONTROLS":options?"OPTIONS":"",199,179,119);
 if(controls){
  const auto lines=control_lines(belt_sequential);
  const int first=std::clamp(scroll,0,int(std::size(lines))-8);
  for(int i=first;i<std::min(int(std::size(lines)),first+8);++i)draw_control_hint(footer,f,pal,72,105+(i-first)*49,lines[i]);
  rect(f,742,100,3,390,{80,67,45});rect(f,740,100+first*390/int(std::size(lines)),7,8*390/int(std::size(lines)),{199,179,119});
  rect(f,52,521,696,34,{0,0,0});draw_control_hint(footer,f,pal,72,531,"D-PAD: Scroll  A: Back");return;
 }
 if(!options){
  bool animated=false;
  try{
   const auto count=std::min({scene.logo_bl.frames_per_direction(),scene.logo_br.frames_per_direction(),scene.logo_fl.frames_per_direction(),scene.logo_fr.frames_per_direction()});
   if(count){const unsigned frame=unsigned(SDL_GetTicks()/40)%count;
    blit_at_anchor(f,scene.logo_bl.frame(0,frame),scene.pal,400,140);blit_at_anchor(f,scene.logo_br.frame(0,frame),scene.pal,400,140);
    blit_additive(f,scene.logo_fl.frame(0,frame),scene.pal,&scene.sky_pl2,400,140);blit_additive(f,scene.logo_fr.frame(0,frame),scene.pal,&scene.sky_pl2,400,140);
    for(const auto* sprite:{&scene.logo_bl,&scene.logo_br,&scene.logo_fl,&scene.logo_fr})sprite->retain_frame(frame);animated=true;
   }
  }catch(const std::bad_alloc&){for(const auto* sprite:{&scene.logo_bl,&scene.logo_br,&scene.logo_fl,&scene.logo_fr})sprite->release_pixels();}
  if(!animated){const std::string title="DIABLO II";const int width=font.measure(title);font.draw_tinted(f,800,600,pal,(800-width)/2,130,title,199,179,119);}
  const char* labels[]{"Options","Controls","Resume Game","Save and return to menu","Exit Game"};
  for(int i=0;i<5;++i){const int y=275+i*47;if(i==row)rect(f,180,y-8,440,36,{64,51,33});const int width=font.measure(labels[i]);font.draw_tinted(f,800,600,pal,(800-width)/2,y,labels[i],i==row?255:220,i==row?214:214,i==row?139:205);}
 }else{
  const std::string labels[]{std::string("Text: ")+(text_scale>100?"Big":"Default"),std::string("Panels: ")+(ui_scale>100?"Big":"Default"),std::string("HUD: ")+(compact?"Compact":"Original"),std::string("Sound: ")+(sound?"On":"Off"),std::string("World: ")+(world_big==2?"Closer (480)":world_big==1?"Big (640)":"Default (800)"),std::string("Combat loot: ")+(dim_loot?"Dim":"Normal"),std::string("Map opacity: ")+std::to_string(map_opacity)+"%",std::string("Font: ")+(readable_font?"Readable":"Original"),std::string("Belt: ")+(belt_sequential?"Sequential":"Potions"),std::string("Belt refill: ")+(belt_refill?"On":"Off"),std::string("SFX volume: ")+std::to_string(sfx_volume)+"%",std::string("Music volume: ")+std::to_string(music_volume)+"%",std::string("Smooth camera: ")+(smooth_camera?"Experimental":"Default"),std::string("Lighting: ")+(fast?"Fast":"Quality")};
  static_assert(std::size(labels)==option_count);
  const int first=std::clamp(row-3,0,option_count-7);
  for(int i=first;i<std::min(option_count,first+7);++i){const int y=108+(i-first)*56;if(i==row)rect(f,58,y-5,350,30,{64,51,33});font.draw_tinted(f,800,600,pal,79,y,labels[i],i==row?255:220,i==row?214:214,i==row?139:205);}
  const char* descriptions[][3]={
   {"Text size","Default or larger labels","for items and game panels."},
   {"Panel size","Default overlays or tall","panels for easier reading."},
   {"HUD layout","Compact belt and globes","or the original interface."},
   {"Game audio","Enable or mute music,","voices and sound effects."},
   {"World scale","800: wide; 640: closer.","480: largest world details."},
   {"Loot labels in combat","Dim labels behind fighters.","Selected loot stays bright."},
   {"Automap opacity","Change map transparency.","Lower shows more world."},
   {"Panel font","Readable uses bold Forum.","Original keeps Diablo glyphs."},
   {"Belt controls","Potions: L = HP, R = mana.","Sequential: L/R scan slots."},
   {"Automatic refill","Replace a used belt item","with the same inventory item."},
   {"Sound effects volume","Adjust voices and effects.","Music has its own volume."},
   {"Music volume","Adjust the soundtrack.","Effects keep their volume."},
   {"Smooth visual motion","Up to 50 FPS; logic 25 Hz.","May reduce performance."},
   {"Lighting quality","Quality: color and shadows.","Slower in busy scenes."}};
  static_assert(std::size(descriptions)==option_count);
  const int top=210;
  rect(f,414,100,3,390,{80,67,45});rect(f,412,100+first*390/option_count,7,7*390/option_count,{199,179,119});
  rect(f,428,top,310,150,{137,112,70});rect(f,430,top+2,306,146,{15,13,11});
  for(int line=0;line<3;++line)footer.draw_tinted(f,800,600,pal,444,top+15+line*34,descriptions[row][line],line?220:255,line?214:214,line?205:139);
 }
 rect(f,52,521,696,34,{0,0,0});
 const char* hint=options?"D-PAD: Select / change   B: Confirm   A: Back":"D-PAD: Select   B: Confirm   A: Back";
 draw_control_hint(footer,f,pal,options?72:(800-footer.measure(hint))/2,531,hint);
}
void InterfaceView::present(std::vector<std::uint8_t>& f,int percent,bool left,bool right,int mx,int my,bool tree) {
 if(percent>100)large_panels(f,left,right,tree);
}
}
