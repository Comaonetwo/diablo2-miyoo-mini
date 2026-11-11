// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <pthread.h>
#include <time.h>
namespace d2mini {
// A bounded stack avoids the multi-megabyte default thread reserve on Miyoo.
class AudioMixer {
 std::atomic<bool> quit_{false},failed_{false};pthread_t thread_{};bool running_=false;
 void(*tick_)(void*)=nullptr;void* context_=nullptr;
 static void* run(void* data){auto& self=*static_cast<AudioMixer*>(data);while(!self.quit_.load(std::memory_order_acquire)){try{self.tick_(self.context_);}catch(...){self.failed_.store(true,std::memory_order_release);break;}timespec delay{0,10000000};nanosleep(&delay,nullptr);}return nullptr;}
public:
 AudioMixer()=default;AudioMixer(const AudioMixer&)=delete;AudioMixer&operator=(const AudioMixer&)=delete;
 ~AudioMixer(){stop();}
 bool start(void(*tick)(void*),void* context){if(running_)return true;tick_=tick;context_=context;quit_=false;failed_=false;pthread_attr_t attr;if(pthread_attr_init(&attr))return false;
  int result=pthread_attr_setstacksize(&attr,256*1024);if(!result)result=pthread_create(&thread_,&attr,run,this);pthread_attr_destroy(&attr);running_=result==0;return running_;}
 void stop(){if(!running_)return;quit_.store(true,std::memory_order_release);pthread_join(thread_,nullptr);running_=false;}
 bool running()const{return running_;}
 bool failed()const{return failed_.load(std::memory_order_acquire);}
};
}
