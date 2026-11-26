// SPDX-License-Identifier: GPL-3.0-or-later
#include <gamedata_load.hpp>
#include <ai.hpp>
#include <cstdio>
#include <stdexcept>
using namespace d2d::game;
#define CHECK(x) do{if(!(x))throw std::runtime_error("andariel_poison line "+std::to_string(__LINE__)+": " #x);}while(0)
int main(int argc,char**argv)try{
 // Exercise the damage oracle without archives as well: rate and duration
 // are independent quantities, even when the missile deals no physical damage.
 {
  d2d::rules::MonStats source;source.level=15;source.to_hit=1000;
  source.poison_min=source.poison_max=80;source.poison_len=50;
  for(int resist:{0,75})for(int shorten:{0,50}){
   d2d::rules::Fighter defender;defender.res[3]=resist;defender.plr=shorten;
   d2d::rules::Rng rng{3};int hits=0;
   for(int n=0;n<64;++n){
    auto hit=d2d::rules::monster_blow(defender,15,d2d::rules::Motion::standing,source,true,rng,true);
    if(!hit.hit)continue;
    ++hits;CHECK(hit.poison==80*(100-resist)/100&&hit.poison_ticks==50*(100-shorten)/100);
   }
   CHECK(hits>0);
  }
 }
 if(argc<2)return 0;setenv("D2D_SERIAL_LOADING","1",1);auto d=load_game_data(argv[1],std::filesystem::path(argv[1])/"LODPatch_114d.exe",3,false);CHECK(d);d2d::rules::Rng rng{3};
 Level a;a.id=37;a.ds1=d2d::ds1::Map(64,64,0,1);a.walk.resize(320*320);Crowd crowd;
 for(int diff=0;diff<3;++diff)for(int skill:{164,201}){
  auto m=make_monster(*d,d->monsters.row("andariel"),20,20,rng,diff);if(diff==0)CHECK(m.stats.a1_min==6&&m.stats.a1_max==19);
  m.mode=skill==164?"SC":"A1";m.unit.mode_ms=1000;m.mode_until=4000;m.skill=skill;m.skill_frame=3;m.skill_x=110;m.skill_y=100;
  Foe foe;foe.x=22;foe.y=20;foe.alive=true;foe.level=15;std::vector<Missile> shots;
  monster_update(*d,a,m,{&foe,1},rng,skill==164?1240:2000,.04f,crowd,shots);CHECK(!shots.empty());
  for(const auto& shot:shots){const auto& r=*shot.info;CHECK(r.name==(skill==164?"andarielspray":"andypoisonbolt"));
   const auto damage=d2d::rules::row_damage(r.etype,r.emin,r.emax,r.emin_lev,r.emax_lev,r.hitshift,r.elen,r.elen_lev,shot.level);
   for(int resistance:{0,75}){
    d2d::rules::Fighter defender;defender.res[3]=resistance;defender.plr=resistance?50:0;
    int hits=0;
    for(int n=0;n<64;++n){const auto hit=d2d::rules::monster_blow(defender,15,d2d::rules::Motion::standing,shot.src,true,rng,true);
     if(!hit.hit)continue;++hits;CHECK(hit.poison>=d2d::rules::resisted(damage.elo,resistance)&&hit.poison<=d2d::rules::resisted(damage.ehi,resistance));CHECK(hit.poison_ticks==d2d::rules::resisted(damage.elen,defender.plr));CHECK(hit.physical>=shot.src.a2_min&&hit.physical<=shot.src.a2_max);
    }
    CHECK(hits>0);
   }
   printf("PASS difficulty=%d %s level=%d poison=%d..%d per-frame/256 duration=%d frames; resistance and PLR\n",diff,r.name.c_str(),shot.level,damage.elo,damage.ehi,damage.elen);break;
  }
 }
 return 0;
}catch(const std::exception&e){fprintf(stderr,"%s\n",e.what());return 1;}
