// SPDX-License-Identifier: GPL-3.0-or-later
#include <dcc.hpp>
#include <cstdio>
#include <stdexcept>
void check(bool yes){if(!yes)throw std::runtime_error("lazy composite frame mismatch");}
int main(){
 int reads=0;
 auto supplier=[&](std::uint8_t dir,std::int32_t frame){++reads;return d2d::dcc::Frame{2,2,3,4,3,3,5,5,{0,std::uint8_t(dir*8+frame+1),2,3}};};
 auto sprite=d2d::dcc::Sprite::from_frame_loader(2,4,supplier);
 check(reads==0&&sprite.directions()==2&&sprite.frames_per_direction()==4);
 for(int n:{0,1,0,2,1,7,6,7,0}){
  const auto& f=sprite.frame(n/4,n%4);check(f.width==2&&f.height==2&&f.box_top==3&&f.pixels==supplier(n/4,n%4).pixels);--reads;
  int cached=0;for(const auto& stored:sprite.frames())cached+=!stored.pixels.empty();check(cached<=2);
 }
 std::array<std::uint8_t,256> map{};for(unsigned i=0;i<256;++i)map[i]=std::uint8_t(i+1);sprite.remap(map.data());
 check(sprite.frame(0,0).pixels[0]==0&&sprite.frame(0,0).pixels[1]==2);
 check(sprite.frame(1,2).pixels[1]==12);
 auto copied=sprite;sprite.release_pixels();for(const auto& f:sprite.frames())check(f.pixels.empty());
 check(copied.frame(1,2).pixels[1]==12&&sprite.frame(1,2).pixels[1]==12);
 int tries=0;auto retry=d2d::dcc::Sprite::from_frame_loader(1,1,[&](auto,auto){if(!tries++)throw std::runtime_error("temporary read failure");return d2d::dcc::Frame{1,1,0,0,0,0,1,1,{7}};});
 try{retry.frame(0,0);check(false);}catch(const std::runtime_error&){}
 check(retry.frame(0,0).pixels[0]==7&&tries==2);
 puts("PASS: lazy composite frames, bounded cache, palette, release/copy and failed-read retry");
}
