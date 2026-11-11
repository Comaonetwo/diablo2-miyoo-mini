// SPDX-License-Identifier: GPL-3.0-or-later
#include "audio.hpp"
#include <cstdio>
#include <cstring>
using d2d::client::Audio;
int main() try {
 SDL_Init(0);
 setenv("D2D_LOOPBACK","1",1);setenv("D2D_AUDIO_CAPTURE","/tmp/d2-audio-lifecycle.pcm",1);unsetenv("D2D_SILENT");
 Audio audio;audio.init();if(!audio.ok)return 1;
 ALCint output_rate=0;alcGetIntegerv(audio.dev,ALC_FREQUENCY,1,&output_rate);if(output_rate!=D2_AUDIO_RATE)return 36;
 // A real SDL WAV decode, then ownership is retained without copying PCM.
 std::vector<std::byte> wav(44+2205*2);auto u16=[&](int p,unsigned v){wav[p]=std::byte(v);wav[p+1]=std::byte(v>>8);};auto u32=[&](int p,unsigned v){u16(p,v);u16(p+2,v>>16);};
 std::memcpy(wav.data(),"RIFF",4);u32(4,wav.size()-8);std::memcpy(wav.data()+8,"WAVEfmt ",8);u32(16,16);u16(20,1);u16(22,1);u32(24,22050);u32(28,44100);u16(32,2);u16(34,16);std::memcpy(wav.data()+36,"data",4);u32(40,wav.size()-44);
 for(int i=0;i<2205;++i)u16(44+i*2,(i%50<25?1000:unsigned(-1000)));
 auto decoded=Audio::decode(wav);if(!decoded||decoded->size!=4410||decoded->freq!=22050)return 2;
 // Real source gains: muting effects must not mute the soundtrack.
 if(!audio.start(audio.music,*decoded,1.f,true,1)||!audio.start(audio.ambience,*decoded,1.f,true,2))return 40;
 audio.master_volume=0;audio.music_volume=75;audio.apply_volume();ALfloat song_gain=0,effects_gain=1,listener_gain=0;
 alGetSourcef(audio.music.src,AL_GAIN,&song_gain);alGetSourcef(audio.ambience.src,AL_GAIN,&effects_gain);alGetListenerf(AL_GAIN,&listener_gain);
 if(std::abs(song_gain-.75f)>.001f||effects_gain!=0||listener_gain<=0)return 41;
 audio.stop(audio.music);audio.stop(audio.ambience);audio.master_volume=100;audio.music_volume=50;audio.apply_volume();
 for(auto& ch:audio.sfx)if(!audio.start(ch,*decoded,1.f,false,1))return 3;
 std::array<std::int16_t,D2_AUDIO_RATE/5> pcm{};audio.render_samples(audio.dev,pcm.data(),pcm.size());audio.update();
 for(const auto& ch:audio.sfx)if(ch.src||ch.buf)return 4;
 if(!audio.start(audio.ambience,*decoded,1.f,true,2))return 5;
 audio.render_samples(audio.dev,pcm.data(),pcm.size());audio.update();
 if(!audio.ambience.src)return 6;
 bool audible=false;for(auto x:pcm)audible|=x!=0;if(!audible)return 7;
 // A short WAV forces repeated end-of-file wraps while queued music plays.
 const auto archive_path=std::filesystem::temp_directory_path()/"d2-music-stream-test.mpq";std::filesystem::remove(archive_path);
 HANDLE archive{},file{};
 if(!SFileCreateArchive(archive_path.c_str(),MPQ_CREATE_ARCHIVE_V1,16,&archive))return 10;
 if(!SFileCreateFile(archive,"music.wav",0,DWORD(wav.size()),0,MPQ_FILE_COMPRESS,&file)||!SFileWriteFile(file,wav.data(),DWORD(wav.size()),MPQ_COMPRESSION_ZLIB)||!SFileFinishFile(file)||!SFileCloseArchive(archive))return 11;
 d2d::mpq::Stack music_mpq;music_mpq.push(archive_path);auto stream=music_mpq.open("music.wav");
 if(!stream||!audio.start_music_stream(std::move(*stream),123,.5f,true))return 12;
 bool heard=false;
 for(int i=0;i<300;++i){audio.render_samples(audio.dev,pcm.data(),pcm.size());for(auto x:pcm)heard|=x!=0;audio.update();if(!audio.music.src||audio.music.sound!=123)return 13;}
 if(!heard||audio.music_buffers.size()!=4)return 14;
 // A transient source underrun must not destroy a looping streamed song.
 const auto looping_source=audio.music.src;alSourceStop(looping_source);audio.update();
 if(audio.music.src!=looping_source||!audio.music_file)return 34;
 audio.update_music_stream();ALint recovered=0;alGetSourcei(looping_source,AL_SOURCE_STATE,&recovered);
 if(recovered!=AL_PLAYING||!audio.music_restarts.load())return 35;
 // Music refill and PCM generation must survive an 800ms stalled game thread.
 const auto mixed_before=audio.mixed_samples.load();if(!audio.start_mixer())return 30;
 SDL_Delay(800);audio.mixer.stop();
 if(audio.mixed_samples.load()-mixed_before<D2_AUDIO_RATE*7/10||audio.mixer.failed())return 31;
 ALint state=0;alGetSourcei(audio.music.src,AL_SOURCE_STATE,&state);if(state!=AL_PLAYING)return 32;
 if(!audio.start_mixer())return 33;SDL_Delay(40);audio.mixer.stop();
 audio.set_enabled(false);if(audio.music.src||audio.music_file)return 15;
 for(auto buffer:audio.music_buffers)if(buffer)return 16;
 std::filesystem::remove(archive_path);
 audio.set_enabled(true);for(auto& ch:audio.sfx)audio.stop(ch);
 const auto old=audio.sfx_duplicates;
 if(!audio.start(audio.sfx[0],*decoded,1.f,false,91))return 20;
 audio.sfx[0].sfx_started=100;audio.sfx[0].sfx_priority=1.f;
 const auto source=audio.sfx[0].src;
 for(int i=0;i<30;++i)if(audio.sfx_channel(91,1.f,110))return 21;
 if(audio.sfx_duplicates-old!=30||audio.sfx[0].src!=source)return 22;
 for(int i=1;i<4;++i){if(!audio.start(audio.sfx[i],*decoded,1.f,false,91+i))return 23;audio.sfx[i].sfx_priority=.1f*i;audio.sfx[i].sfx_started=100;}
 if(audio.sfx_channel(99,.05f,110))return 24;
 if(audio.sfx_channel(99,.9f,110)!=&audio.sfx[1]||audio.sfx_steals!=1)return 25;
 audio.render_samples(audio.dev,pcm.data(),pcm.size());audio.update();
 if(!audio.sfx_channel(99,.1f,150))return 26;
 std::puts("PASS: WAV ownership, 12 one-shot buffers reclaimed, ambience and bounded looping music produce PCM, disable releases streaming buffers; duplicate bursts and quiet interruptions are bounded");return 0;
}catch(const std::exception&e){std::fprintf(stderr,"%s\n",e.what());return 8;}
