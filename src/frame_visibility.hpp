// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "world_occlusion.hpp"
#include <cstdint>
namespace d2mini {
inline bool frame_hidden(std::int64_t left,std::int64_t top,std::int64_t right,std::int64_t bottom,int sw,int sh){
 const auto& o=world_occlusion;
 return left>=right||top>=bottom||right<=0||bottom<=0||left>=sw||top>=sh||right<=o.clip_left||left>=o.clip_right||bottom<=o.clip_top||top>=o.clip_bottom||(o.left<o.right&&left>=o.left&&top>=o.top&&right<=o.right&&bottom<=o.bottom);
}
inline bool body_hidden(int w,int h,int bx,int by,int ax,int ay,int sw,int sh){return frame_hidden(std::int64_t(ax)+bx,std::int64_t(ay)+by,std::int64_t(ax)+bx+w,std::int64_t(ay)+by+h,sw,sh);}
inline bool shadow_hidden(int w,int h,int bx,int by,int ax,int ay,int sw,int sh){
 if(w<=0||h<=0)return true;
 const auto bottom=std::int64_t(by)+h-1,left=std::int64_t(ax)+bx+bottom/2,top=std::int64_t(ay)+bottom/2,rows=(std::int64_t(h)+1)/2;
 return frame_hidden(left-(rows-1),top-(rows-1),left+w,top+1,sw,sh);
}
}
