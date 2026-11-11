// SPDX-License-Identifier: GPL-3.0-or-later
#include "audio_sink.hpp"
#include "audio_ring.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <cerrno>
namespace d2mini {
struct AudioSink::Impl {
 D2AudioRing* ring=nullptr;pid_t child=-1;std::string path;FILE*capture=nullptr;
 ~Impl(){
  if(ring)__atomic_store_n(&ring->quit,1,__ATOMIC_RELEASE);
  if(child>0){int status=0;bool done=false;for(int i=0;i<50;++i){const auto r=waitpid(child,&status,WNOHANG);if(r==child||(r<0&&errno==ECHILD)){done=true;break;}usleep(10000);}if(!done){kill(child,SIGTERM);for(int i=0;i<20;++i){const auto r=waitpid(child,&status,WNOHANG);if(r==child||(r<0&&errno==ECHILD)){done=true;break;}usleep(10000);}if(!done){kill(child,SIGKILL);while(waitpid(child,&status,0)<0&&errno==EINTR){}}}}
  if(ring)munmap(ring,sizeof(*ring));if(!path.empty())unlink(path.c_str());if(capture)std::fclose(capture);
 }
};
AudioSink::AudioSink():impl_(std::make_unique<Impl>()){} AudioSink::~AudioSink()=default;
bool AudioSink::open(){
 if(const char*p=std::getenv("D2D_AUDIO_CAPTURE")){impl_->capture=std::fopen(p,"wb");return impl_->capture;}
 const char*helper=std::getenv("D2D_AUDIO_HELPER");if(!helper||!*helper)return false;
 char temp[]="/tmp/d2-audio-XXXXXX";int fd=mkstemp(temp);if(fd<0)return false;impl_->path=temp;
 if(ftruncate(fd,sizeof(D2AudioRing))){close(fd);return false;}
 auto*r=static_cast<D2AudioRing*>(mmap(nullptr,sizeof(D2AudioRing),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0));close(fd);
 if(r==MAP_FAILED)return false;impl_->ring=r;std::memset(r,0,sizeof(*r));r->abi=D2_AUDIO_ABI;
 // Prime the same reserve that pump maintains after loading stalls.
 std::int16_t silence[D2_AUDIO_TARGET]{};d2_audio_write(r,silence,D2_AUDIO_TARGET);
 // The helper reopens the IPC pathname. Keep game MPQ/fb/input descriptors
 // out of the vendor SDL process; fork otherwise shares their file offsets.
 const long open_limit=sysconf(_SC_OPEN_MAX);
 const int close_limit=open_limit>0&&open_limit<65536?int(open_limit):65536;
 pid_t child=fork();if(child==0){for(int fd=3;fd<close_limit;++fd)close(fd);execl(helper,helper,temp,(char*)nullptr);_exit(127);}if(child<0)return false;impl_->child=child;
 for(int i=0;i<150;++i){const auto ready=__atomic_load_n(&r->ready,__ATOMIC_ACQUIRE);if(ready)return ready==1;int status;if(waitpid(child,&status,WNOHANG)==child){impl_->child=-1;return false;}usleep(10000);}return false;
}
bool AudioSink::device_open()const{return impl_->ring!=nullptr;}
AudioSink::Status AudioSink::status()const{
 if(!impl_->ring)return {};
 const auto head=__atomic_load_n(&impl_->ring->head,__ATOMIC_ACQUIRE),tail=__atomic_load_n(&impl_->ring->tail,__ATOMIC_ACQUIRE);
 return {head-tail,__atomic_load_n(&impl_->ring->underruns,__ATOMIC_RELAXED)};
}
std::size_t AudioSink::samples_needed(std::size_t elapsed_samples)const{
 if(impl_->ring&&__atomic_load_n(&impl_->ring->ready,__ATOMIC_ACQUIRE)==1)return d2_audio_needed(impl_->ring);
 return elapsed_samples; // Captures retain their wall-clock duration.
}
void AudioSink::write(std::span<const std::int16_t>s){if(impl_->capture)std::fwrite(s.data(),2,s.size(),impl_->capture);if(impl_->ring&&__atomic_load_n(&impl_->ring->ready,__ATOMIC_ACQUIRE)==1)d2_audio_write(impl_->ring,s.data(),unsigned(s.size()));}
}
