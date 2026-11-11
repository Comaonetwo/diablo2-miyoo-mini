/* SPDX-License-Identifier: GPL-3.0-or-later
 * Isolated glibc process using the console's installed SDL 1.2 audio ABI.
 * No vendor libraries are linked into or redistributed with the game.
 */
#define _GNU_SOURCE
#include "audio_ring.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <signal.h>
typedef struct {int freq;uint16_t format;uint8_t channels,silence;uint16_t samples,padding;uint32_t size;void(*callback)(void*,uint8_t*,int);void*userdata;} AudioSpec;
static volatile sig_atomic_t stopped;
static void stop(int sig){(void)sig;stopped=1;}
static struct D2AudioEnvelope envelope={0,D2_AUDIO_FADE};
static void callback(void* p,uint8_t* bytes,int n){if(n<0)return;d2_audio_read_smooth(p,(int16_t*)bytes,(unsigned)n/2,&envelope);}
int main(int argc,char**argv){
 if(argc!=2)return 2;
 pid_t parent=getppid();prctl(PR_SET_PDEATHSIG,SIGTERM);if(getppid()!=parent)return 2;
 signal(SIGTERM,stop);signal(SIGINT,stop);
 int fd=open(argv[1],O_RDWR);if(fd<0)return 3;
 if(lseek(fd,0,SEEK_END)!=(off_t)sizeof(struct D2AudioRing)){close(fd);return 3;}
 struct D2AudioRing*r=mmap(0,sizeof(*r),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);close(fd);
 if(r==MAP_FAILED)return 3;
 if(r->abi!=D2_AUDIO_ABI){munmap(r,sizeof(*r));return 3;}
 void*lib=dlopen("/mnt/SDCARD/miyoo/lib/libSDL-1.2.so.0",RTLD_NOW|RTLD_LOCAL);
 if(!lib){fprintf(stderr,"d2_audio: SDL library: %s\n",dlerror());__atomic_store_n(&r->ready,2,__ATOMIC_RELEASE);return 4;}
 int(*init)(uint32_t)=dlsym(lib,"SDL_Init");int(*open_audio)(AudioSpec*,AudioSpec*)=dlsym(lib,"SDL_OpenAudio");
 void(*pause_audio)(int)=dlsym(lib,"SDL_PauseAudio");void(*close_audio)(void)=dlsym(lib,"SDL_CloseAudio");void(*quit)(void)=dlsym(lib,"SDL_Quit");
 const char*(*error)(void)=dlsym(lib,"SDL_GetError");
 AudioSpec want={0},got={0};want.freq=D2_AUDIO_RATE;want.format=0x8010;want.channels=1;want.samples=1024;want.callback=callback;want.userdata=r;
 if(!init||!open_audio||!pause_audio||!close_audio||!quit||init(0x10)||open_audio(&want,&got)||got.freq!=D2_AUDIO_RATE||got.format!=0x8010||got.channels!=1){
  fprintf(stderr,"d2_audio: device failed: %s\n",error?error():"missing symbols");__atomic_store_n(&r->ready,2,__ATOMIC_RELEASE);return 5;}
 fprintf(stderr,"d2_audio: SDL output %d Hz mono, buffer=%u\n",got.freq,got.samples);
 __atomic_store_n(&r->ready,1,__ATOMIC_RELEASE);pause_audio(0);
 while(!stopped&&!__atomic_load_n(&r->quit,__ATOMIC_ACQUIRE)&&getppid()==parent)usleep(10000);
 pause_audio(1);close_audio();quit();fprintf(stderr,"d2_audio: shutdown, underruns=%u\n",r->underruns);
 munmap(r,sizeof(*r));dlclose(lib);return 0;
}
