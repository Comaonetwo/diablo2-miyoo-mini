// SPDX-License-Identifier: GPL-3.0-or-later
#include "fb_convert.hpp"
#include <chrono>
#include <array>
#include <algorithm>
#include <cstdio>
int main(){
 std::vector<std::uint8_t> src(800*600*4),dst(640*480*4);
 for(unsigned i=0;i<src.size();++i)src[i]=(i*67+i/19)%251;
 const d2mini::FrameFormat f{640,480,2560,32,{16,8},{8,8},{0,8},{24,8}};
 const auto original=src;
 for(bool smooth:{false,true}){
  std::array<long long,7> samples{};std::uint64_t checksum=0;
  for(auto& ns:samples){src=original;const auto start=std::chrono::steady_clock::now();for(int i=0;i<100;++i){src[(i*7919)%src.size()]^=std::uint8_t(i);d2mini::convert_frame(src,800,600,dst,f,true,smooth);checksum+=dst[i];}ns=(std::chrono::steady_clock::now()-start).count()/100;}
  std::sort(samples.begin(),samples.end());std::printf("neon=%d smooth=%d median_ns=%lld checksum=%llu\n",int(d2mini::frame_filter_detail::available()),smooth,samples[3],(unsigned long long)checksum);
 }
}
