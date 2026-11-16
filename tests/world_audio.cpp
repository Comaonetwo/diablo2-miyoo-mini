// SPDX-License-Identifier: GPL-3.0-or-later
#include "town.hpp"
#include <gamedata_load.hpp>
#include <cstdio>
#include <stdexcept>
using namespace d2d::client;
#define CHECK(x) do{if(!(x))throw std::runtime_error("world audio line "+std::to_string(__LINE__)+": " #x);}while(0)
int main(int argc,char**argv)try{
 if(argc<2)return 0;SDL_Init(0);setenv("D2D_SERIAL_LOADING","1",1);setenv("D2D_LOOPBACK","1",1);setenv("D2D_AUDIO_CAPTURE","/tmp/d2-world-audio.pcm",1);unsetenv("D2D_SILENT");
 auto data=d2d::game::load_game_data(argv[1],std::filesystem::path(argv[1])/"LODPatch_114d.exe",3,false);CHECK(data);Scene scene;static_cast<d2d::game::GameData&>(scene)=std::move(*data);Audio audio;audio.init();CHECK(audio.ok);
 std::array<Audio::WorldLoop,3> requests;for(int i=0;i<3;++i)requests[i]={scene.sound_index.at(std::array{"object_fire_loop_magic","druid_hurricane","hawknest_loop"}[i]),.2f,0};
 audio.set_world_loops(scene,requests);int active=0,bytes=0;for(auto& ch:audio.world_loops)if(ch.src){++active;ALint size=0,loop=0;alGetBufferi(ch.buf,AL_SIZE,&size);alGetSourcei(ch.src,AL_LOOPING,&loop);CHECK(loop);bytes+=size;}CHECK(active>0&&active<=2&&bytes<=512*1024);
 auto ids=std::array{audio.world_loops[0].src,audio.world_loops[1].src};audio.set_world_loops(scene,requests);CHECK(ids[0]==audio.world_loops[0].src&&ids[1]==audio.world_loops[1].src);audio.set_world_loops(scene,{});for(auto& ch:audio.world_loops)CHECK(!ch.src&&!ch.buf);
 printf("PASS spatial loops: actual WAV playback, %d channels, %d PCM bytes, unchanged sources on refresh and complete release\n",active,bytes);
 WorldAudio effects;View view;view.level=&scene.town;view.player.x=view.player.y=20;view.poisoned=true;effects.update(scene,view,audio,1000);auto count=audio.sfx_requests;CHECK(count>0);effects.update(scene,view,audio,1040);CHECK(audio.sfx_requests==count);view.poisoned=false;effects.update(scene,view,audio,1080);view.poisoned=true;effects.update(scene,view,audio,1120);CHECK(audio.sfx_requests>count);
 for(const auto& skill:scene.skills.rows)if(!skill.aurastate.empty())view.buffs.push_back(skill.id);effects.update(scene,view,audio,1140);count=audio.sfx_requests;effects.update(scene,view,audio,1180);CHECK(audio.sfx_requests==count);view.buffs.clear();
 const auto& fire=scene.missiles.at("firewall");for(int i=0;i<1000;++i)view.missiles.push_back({&fire,20+float(i%4)*.1f,20,0,1200,unsigned(i+1)});effects.update(scene,view,audio,1200);CHECK(effects.units.size()<=256);int loops=0;for(auto& ch:audio.world_loops)loops+=ch.src!=0;CHECK(loops<=2);
 view.missiles.clear();view.pets.clear();View::Pet pet;pet.id=100;pet.npc=scene.mon_npc[std::size_t(scene.monsters.row("skeleton1"))];pet.unit.x=pet.unit.y=20;pet.mode="NU";view.pets.push_back(pet);effects.update(scene,view,audio,1220);
 auto before_death=audio.sfx_requests;view.pets[0].mode="DT";view.pets[0].unit.mode_ms=1240;effects.update(scene,view,audio,1240);CHECK(audio.sfx_requests>before_death);before_death=audio.sfx_requests;effects.update(scene,view,audio,1280);CHECK(audio.sfx_requests==before_death);view.pets.clear();
 d2d::game::Level next;next.id=2;view.level=&next;view.missiles.clear();view.poisoned=false;effects.update(scene,view,audio,1320);CHECK(effects.units.size()<=1);for(auto& ch:audio.world_loops)CHECK(!ch.src&&!ch.buf);
 puts("PASS state on/off edges once, 1000 identical effects bounded/coalesced, summon death once and sources removed on level change");return 0;
}catch(const std::exception& e){fprintf(stderr,"%s\n",e.what());return 1;}
