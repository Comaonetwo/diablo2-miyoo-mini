// SPDX-License-Identifier: GPL-3.0-or-later
#include "audio_ring.h"
#include <array>
#include <algorithm>
#include <cstdio>
#include <memory>
#include <thread>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("audio ring failed");}
int main()try{
 auto r=std::make_unique<D2AudioRing>();std::array<int16_t,512> in,out;
 r->head=r->tail=UINT32_MAX-100;
 for(unsigned i=0;i<in.size();++i)in[i]=int16_t(i);
 check(d2_audio_write(r.get(),in.data(),512)==512);check(d2_audio_read(r.get(),out.data(),512)==512&&in==out);
 check(d2_audio_read(r.get(),out.data(),512)==0);for(auto n:out)check(n==0);
 unsigned wrote=0;while(d2_audio_write(r.get(),in.data(),512))wrote+=512;
 check(wrote==D2_AUDIO_CAPACITY);while(d2_audio_read(r.get(),out.data(),512)){}
 // A single long render stall must not consume the reserve permanently.
 std::array<int16_t,D2_AUDIO_TARGET> reserve{};reserve.fill(12000);
 check(d2_audio_needed(r.get())==D2_AUDIO_TARGET);
 d2_audio_write(r.get(),reserve.data(),d2_audio_needed(r.get()));
 for(unsigned delay:{140u,180u,120u,90u}){
  unsigned left=delay*D2_AUDIO_RATE/1000;
  while(left){unsigned n=std::min<unsigned>(left,out.size());check(d2_audio_read(r.get(),out.data(),n)==n);left-=n;}
  d2_audio_write(r.get(),reserve.data(),d2_audio_needed(r.get()));
  check(d2_audio_needed(r.get())==0);
 }
 while(d2_audio_read(r.get(),out.data(),512)){}
 d2_audio_write(r.get(),in.data(),512);D2AudioEnvelope smooth{0,D2_AUDIO_FADE};
 check(d2_audio_read_smooth(r.get(),out.data(),512,&smooth)==512&&out==in);
 check(d2_audio_read_smooth(r.get(),out.data(),512,&smooth)==0);
 check(out[0]<in.back()&&out[0]>0&&out[D2_AUDIO_FADE-1]==0&&smooth.gain==0);
 for(unsigned i=1;i<D2_AUDIO_FADE;++i)check(out[i]<=out[i-1]);
 in.fill(-20000);d2_audio_write(r.get(),in.data(),512);
 check(d2_audio_read_smooth(r.get(),out.data(),512,&smooth)==512);
 check(out[0]>-20000&&out[D2_AUDIO_FADE-1]==-20000&&out[511]==-20000);
 constexpr unsigned count=1000000;
 std::thread producer([&]{for(unsigned i=0;i<count;){int16_t sample=int16_t(i);if(d2_audio_write(r.get(),&sample,1))++i;else std::this_thread::yield();}});
 bool valid=true;for(unsigned i=0;i<count;){int16_t sample;if(d2_audio_read(r.get(),&sample,1)){valid&=sample==int16_t(i);++i;}else std::this_thread::yield();}
 producer.join();check(valid);
 std::puts("PASS: audio FIFO wrap, full/empty, silence, reserve recovery, starvation envelope, concurrent sample order");return 0;
}catch(const std::exception&e){std::fprintf(stderr,"%s\n",e.what());return 1;}
