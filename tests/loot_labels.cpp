// SPDX-License-Identifier: GPL-3.0-or-later
#include "loot_labels.hpp"
#include <cassert>
#include <cstdio>
int main(){
 d2mini::LootFade fade;assert(fade.step(100,false)==0);assert(fade.step(250,true)==.5f);assert(fade.step(400,true)==1);assert(fade.step(550,false)==.5f);assert(fade.step(700,false)==0);
 for(bool selected:{false,true}){
  std::vector<std::uint8_t> pixels(8*600*4,20);for(std::size_t i=3;i<pixels.size();i+=4)pixels[i]=255;
  auto* actor=pixels.data()+(3*8+3)*4;actor[0]=90;actor[1]=70;actor[2]=50;actor[3]=254;
  {const d2mini::LootLabelComposite blend(pixels,8,{0,0,8,8},1,selected);for(int y=0;y<8;++y)for(int x=0;x<8;++x)for(int c=0;c<3;++c)pixels[(y*8+x)*4+c]=255;}
  assert(actor[0]==90&&actor[1]==70&&actor[2]==50&&actor[3]==254);
  assert(selected?pixels[0]==255:pixels[0]>20&&pixels[0]<130);
 }
 std::vector<std::uint8_t> frame(16*600*4,100);{const d2mini::LootLabelComposite blend(frame,16,{-20,-30,7,6},.5f,false);frame[0]=200;}assert(frame[0]>100&&frame[0]<200);
 std::puts("PASS loot combat fade: 300 ms both directions, selected label readability, exact actor preservation, faded background and clipped labels");
}
