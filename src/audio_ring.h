/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef D2_AUDIO_RING_H
#define D2_AUDIO_RING_H
#include <stdint.h>
#include <string.h>
#define D2_AUDIO_ABI 0x44324102u
#define D2_AUDIO_RATE 44100u
#define D2_AUDIO_CAPACITY 32768u
struct D2AudioRing { uint32_t abi, ready, quit, head, tail, underruns; int16_t samples[D2_AUDIO_CAPACITY]; };
/* One producer and one consumer, using lock-free 32-bit ARMv7 atomics. */
static inline unsigned d2_audio_write(struct D2AudioRing* r,const int16_t* pcm,unsigned n) {
 uint32_t head=__atomic_load_n(&r->head,__ATOMIC_RELAXED),tail=__atomic_load_n(&r->tail,__ATOMIC_ACQUIRE);
 unsigned used=head-tail;if(used>D2_AUDIO_CAPACITY)return 0;
 if(n>D2_AUDIO_CAPACITY-used)n=D2_AUDIO_CAPACITY-used;
 for(unsigned i=0;i<n;++i)r->samples[(head+i)&(D2_AUDIO_CAPACITY-1)]=pcm[i];
 __atomic_store_n(&r->head,head+n,__ATOMIC_RELEASE);return n;
}
/* Queue depth belongs to the consumer clock, not render-frame elapsed time.
 * Restore a 250 ms reserve after a stall instead of permanently running empty. */
#define D2_AUDIO_TARGET (D2_AUDIO_RATE / 4u)
static inline unsigned d2_audio_needed(const struct D2AudioRing* r) {
 uint32_t head=__atomic_load_n(&r->head,__ATOMIC_ACQUIRE),tail=__atomic_load_n(&r->tail,__ATOMIC_ACQUIRE);
 unsigned ready=head-tail;return ready<D2_AUDIO_TARGET?D2_AUDIO_TARGET-ready:0;
}
static inline unsigned d2_audio_read(struct D2AudioRing* r,int16_t* pcm,unsigned n) {
 uint32_t tail=__atomic_load_n(&r->tail,__ATOMIC_RELAXED),head=__atomic_load_n(&r->head,__ATOMIC_ACQUIRE);
 unsigned ready=head-tail;if(ready>D2_AUDIO_CAPACITY)ready=0;
 unsigned count=n<ready?n:ready;
 for(unsigned i=0;i<count;++i)pcm[i]=r->samples[(tail+i)&(D2_AUDIO_CAPACITY-1)];
 memset(pcm+count,0,(n-count)*sizeof(*pcm));
 __atomic_store_n(&r->tail,tail+count,__ATOMIC_RELEASE);
 if(count<n)__atomic_fetch_add(&r->underruns,1,__ATOMIC_RELAXED);
 return count;
}
/* Only the device consumer uses this envelope. Normal full-buffer audio is
 * unchanged; starvation fades the held sample over 5 ms, then fades recovery. */
struct D2AudioEnvelope { int32_t last;unsigned gain; };
#define D2_AUDIO_FADE (D2_AUDIO_RATE / 200u)
static inline unsigned d2_audio_read_smooth(struct D2AudioRing* r,int16_t* pcm,unsigned n,struct D2AudioEnvelope* state) {
 unsigned count=d2_audio_read(r,pcm,n);
 for(unsigned i=0;i<n;++i){
  if(i<count){state->last=pcm[i];if(state->gain<D2_AUDIO_FADE)++state->gain;}
  else if(state->gain)--state->gain;
  pcm[i]=(int16_t)(state->last*(int32_t)state->gain/(int32_t)D2_AUDIO_FADE);
 }
 return count;
}
#endif
