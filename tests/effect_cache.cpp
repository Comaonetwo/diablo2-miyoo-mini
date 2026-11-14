// SPDX-License-Identifier: GPL-3.0-or-later
#include "effect_cache.hpp"
#include <gamedata_load.hpp>
#include <dcc.hpp>
#include <cstdio>
#include <stdexcept>
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(int argc,char**argv)try{
 if(argc!=2){puts("SKIP: owner resources required");return 0;}
 auto data=d2d::game::load_game_data(argv[1],std::filesystem::path(argv[1])/"LODPatch_114d.exe",3,false);CHECK(data);
 CHECK(d2mini::effect_key("Expansion/ICEBolt")=="expansion\\icebolt");
 CHECK(d2mini::effect_key(data->missiles.at("icebolt").cel_file)==d2mini::effect_key(data->missiles.at("frozenorbbolt").cel_file));
 int checked=0;for(auto name:{"chargedbolt","nova","frostnova","fireball","hydra","icebolt","teeth","icebreaksmall","icebreaklarge","firewall","diablight"}){
  const auto& info=data->missiles.at(name);const auto key=d2mini::effect_key(info.cel_file);auto raw=data->mpqs.read(std::string(R"(data\global\missiles\)")+key+".dcc");
  const auto p=d2mini::effect_directions(key);d2d::dcc::Sprite original{std::vector<std::byte>(raw)},cached{std::move(raw)};cached.cache_directions(p.count,p.pixels);
  d2d::dcc::decode_profile={true,0,0};const auto reservation=cached.cache_reservation_bytes();CHECK(d2d::dcc::decode_profile.calls==0);
  for(int tick=0;tick<24;++tick)for(unsigned d=0;d<cached.directions();++d){
   const unsigned dir=d;const int frame=tick%cached.frames_per_direction();
   CHECK(cached.frame(dir,frame).pixels==original.frame(dir,frame).pixels);CHECK(cached.storage_bytes()<=reservation);
  }
  const auto warm=cached.storage_bytes();cached.release_pixels();CHECK(cached.storage_bytes()<=warm);CHECK(cached.cache_reservation_bytes()==reservation);
  ++checked;
 }
 printf("PASS %d original effect assets: unchanged pixels, predecode reservation upper bound, release/reuse, case aliases\n",checked);
 return 0;
}catch(const std::exception&e){fprintf(stderr,"%s\n",e.what());return 1;}
