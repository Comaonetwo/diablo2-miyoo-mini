// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
namespace d2mini {
// One opaque HUD rectangle. No full-screen mask or additional framebuffer.
struct WorldOcclusion {
 int left=0,top=600,right=0,bottom=600;
 int clip_left=0,clip_top=0,clip_right=100000,clip_bottom=600;
 bool hides(int x,int y,int w,int h)const{return x+w<=clip_left||x>=clip_right||y+h<=clip_top||y>=clip_bottom||(left<right&&x>=left&&y>=top&&x+w<=right&&y+h<=bottom);}
 template<class Fn> void visible(int x0,int x1,int y,Fn&& draw)const {
  if(y<clip_top||y>=clip_bottom)return;
  x0=std::max(x0,clip_left);x1=std::min(x1,clip_right);if(x0>=x1)return;
  if(y<top||y>=bottom||left>=right||x1<=left||x0>=right){if(x0<x1)draw(x0,x1);return;}
  if(x0<left)draw(x0,std::min(x1,left));
  if(x1>right)draw(std::max(x0,right),x1);
 }
};
inline thread_local WorldOcclusion world_occlusion;
struct WorldOcclusionScope {
 WorldOcclusion previous;
 explicit WorldOcclusionScope(WorldOcclusion value):previous(world_occlusion){world_occlusion=value;}
 ~WorldOcclusionScope(){world_occlusion=previous;}
 WorldOcclusionScope(const WorldOcclusionScope&)=delete;
};
}
