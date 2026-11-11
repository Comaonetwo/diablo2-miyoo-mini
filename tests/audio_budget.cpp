// SPDX-License-Identifier: GPL-3.0-or-later
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#ifdef __APPLE__
#include <malloc/malloc.h>
#endif
static std::size_t heap(){
#ifdef __APPLE__
 malloc_statistics_t s{};malloc_zone_statistics(nullptr,&s);return s.size_in_use;
#else
 return 0;
#endif
}
int main(){
 const auto open=reinterpret_cast<LPALCLOOPBACKOPENDEVICESOFT>(alcGetProcAddress(nullptr,"alcLoopbackOpenDeviceSOFT"));
 const auto render=reinterpret_cast<LPALCRENDERSAMPLESSOFT>(alcGetProcAddress(nullptr,"alcRenderSamplesSOFT"));
 if(!open||!render)return 1;
 auto* dev=open(nullptr);if(!dev)return 2;auto before=heap();
 const ALCint attrs[]={ALC_FREQUENCY,22050,ALC_FORMAT_CHANNELS_SOFT,ALC_MONO_SOFT,ALC_FORMAT_TYPE_SOFT,ALC_SHORT_SOFT,0};
 auto*ctx=alcCreateContext(dev,attrs);if(!ctx||!alcMakeContextCurrent(ctx))return 3;
 auto after=heap();std::printf("context_heap_before=%zu after=%zu delta=%zu\n",before,after,after-before);std::fflush(stdout);
 std::array<ALuint,48> sources{};ALuint buffer{};alGenSources(sources.size(),sources.data());alGenBuffers(1,&buffer);
 std::array<std::int16_t,4410> wave{};for(std::size_t i=0;i<wave.size();++i)wave[i]=std::int16_t(200*std::sin(i*6.283185307179586*440/22050));
 alBufferData(buffer,AL_FORMAT_MONO16,wave.data(),wave.size()*2,22050);
 for(auto source:sources){alSourcei(source,AL_BUFFER,buffer);alSourcei(source,AL_LOOPING,AL_TRUE);alSourcePlay(source);}
 if(alGetError()!=AL_NO_ERROR)return 4;
 std::array<std::int16_t,4096> pcm{};unsigned nz=0;int peak=0;
 for(int r=0;r<3;++r){render(dev,pcm.data(),pcm.size());for(int x:pcm){nz+=x!=0;peak=std::max(peak,std::abs(x));}}
 for(auto source:sources)alSourceStop(source);alDeleteSources(sources.size(),sources.data());alDeleteBuffers(1,&buffer);
 bool good=alGetError()==AL_NO_ERROR&&alcGetError(dev)==ALC_NO_ERROR&&nz>1000&&peak>1000;
 alcMakeContextCurrent(nullptr);alcDestroyContext(ctx);alcCloseDevice(dev);
 std::printf("%s: 48 simultaneous loopback sources, nonzero=%u peak=%d\n",good?"PASS":"FAIL",nz,peak);return good?0:5;
}
