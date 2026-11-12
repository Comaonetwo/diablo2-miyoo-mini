// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
namespace d2mini {
struct RoofCutout {
 static constexpr int kRadius=110;
 int x=0,y=0,radius=0;
 int alpha(int px,int py,int base)const{
  const int dx=px-x,dy=py-y,d2=dx*dx+dy*dy;
  if(d2>=radius*radius)return base;
  const int inner=radius*2/3;
  if(d2<=inner*inner)return std::min(base,16);
  const float edge=(std::sqrt(float(d2))-float(inner))/float(radius-inner);
  return std::min(base,16+int(239.f*edge));
 }
};
}
