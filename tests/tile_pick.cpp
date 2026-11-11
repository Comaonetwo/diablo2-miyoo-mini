// SPDX-License-Identifier: GPL-3.0-or-later
#include <tile_pick.hpp>
#include <stdexcept>
#include <cstdio>
using namespace d2d::drlg;
using Candidate=std::pair<const Dt1File*,int>;
std::vector<Candidate> original_candidates(const std::vector<const Dt1File*>& list,int o,int s,int q){
 std::vector<Candidate> out;
 for(auto*f:list){for(int i=int(f->tiles.size())-1;i>=0&&out.size()<40;--i){auto&t=f->tiles[i];if(t.orient==o&&t.style==s&&t.seq==q)out.emplace_back(f,i);}if(out.size()==40)break;}
 return out;
}
Candidate original_pick(const std::vector<const Dt1File*>&list,d2d::rules::Rng&r,int o,std::uint32_t w){
 auto c=original_candidates(list,o,w?int(w>>20&63):0,w?int(w>>8&255):0);
 if(c.empty()){c=original_candidates(list,10,0,0);return c.empty()?Candidate{nullptr,-1}:c.front();}
 int sum=0;for(auto[f,i]:c)sum+=f->tiles[i].rarity;if(sum<1)return c.front();
 int roll=int(r(sum))+1;std::size_t n=0;while(c.size()>1&&roll>0){roll-=c[n].first->tiles[c[n].second].rarity;++n;}return c[n?n-1:0];
}
int main(){
 std::array<Dt1File,3> files;
 for(int f=0;f<3;++f)for(int i=0;i<180;++i)files[f].tiles.push_back({i%3, i%2, i%5, (i+f)%7,0});
 files[2].tiles.push_back({10,0,0,0,0});
 std::vector<const Dt1File*> list{&files[0],&files[1],&files[2]};
 for(int o=0;o<12;++o)for(int s=0;s<3;++s)for(int q=0;q<7;++q){
  auto a=original_candidates(list,o,s,q);auto b=tile_candidates(list,o,s,q);
  if(a.size()!=b.size()||!std::equal(a.begin(),a.end(),b.begin()))throw std::runtime_error("candidate order changed");
  for(unsigned seed=0;seed<30;++seed){d2d::rules::Rng x{seed},y{seed};auto word=std::uint32_t(s<<20|q<<8);if(original_pick(list,x,o,word)!=pick_tile(list,y,o,word)||x.next()!=y.next())throw std::runtime_error("pick or RNG changed");}
 }
 Dt1File dense;dense.tiles.resize(100,{0,0,0,1,0});auto c=tile_candidates({&dense},0,0,0);if(c.size()!=40||c.front().second!=99||c[39].second!=60)throw std::runtime_error("40 candidate bound changed");
 puts("PASS: candidate order, 40 limit, rarity/fallback picks and RNG match vector reference");
}
