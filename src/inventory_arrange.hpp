// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <rules.hpp>
#include <algorithm>
#include <array>
#include <vector>
namespace d2mini {
// Transactional packing: no item moves unless the complete candidate fits.
template<class Tables,class Items> bool arrange_inventory(const Tables& rules,Items& items,int cols=10,int rows=4){
 struct Entry{std::size_t index;int w,h;};std::vector<Entry> order;
 for(std::size_t i=0;i<items.size();++i)if(items[i].location==d2d::d2s::item_location::kStored&&items[i].panel==d2d::d2s::item_panel::kInventory){auto [w,h]=d2d::rules::item_size(rules,items[i].code);if(w<=0||h<=0||w>cols||h>rows)return false;order.push_back({i,w,h});}
 if(order.empty())return true;
 int area=0,min_width=1,min_height=1;
 for(const auto& e:order){area+=e.w*e.h;min_width=std::max(min_width,e.w);min_height=std::max(min_height,e.h);}
 if(area>cols*rows)return false;
 // Pack a compact left-hand block first, preserving a full-height free
 // region on the right for the next armor/weapon pickup. Small piles stay
 // approximately square instead of spanning all ten inventory columns.
 int square=1;while(square*square<area)++square;
 const int first_width=std::min(cols,std::max({min_width,(area+rows-1)/rows,square}));
 for(int width=first_width;width<=cols;++width)for(int height=std::max(min_height,(area+width-1)/width);height<=rows;++height)for(int strategy=0;strategy<3;++strategy){
  std::stable_sort(order.begin(),order.end(),[&](const auto&a,const auto&b){
   if(strategy==0&&a.w*a.h!=b.w*b.h)return a.w*a.h>b.w*b.h;
   if(strategy==1&&a.h!=b.h)return a.h>b.h;
   if(strategy==2&&items[a.index].code!=items[b.index].code)return items[a.index].code<items[b.index].code;
   if(a.w!=b.w)return a.w>b.w;return a.index<b.index;
  });
  std::vector<bool> used(std::size_t(cols*rows));std::vector<std::array<int,2>> positions;bool complete=true;
  for(const auto&e:order){bool found=false;
   for(int y=0;y+e.h<=height&&!found;++y)for(int x=0;x+e.w<=width&&!found;++x){bool free=true;for(int j=0;j<e.h;++j)for(int i=0;i<e.w;++i)free=free&&!used[std::size_t((y+j)*cols+x+i)];if(!free)continue;
    for(int j=0;j<e.h;++j)for(int i=0;i<e.w;++i)used[std::size_t((y+j)*cols+x+i)]=true;positions.push_back({x,y});found=true;
   }if(!found){complete=false;break;}
  }
  if(complete){for(std::size_t i=0;i<order.size();++i){auto&item=items[order[i].index];item.column=positions[i][0];item.row=positions[i][1];}return true;}
 }return false;
}
}
