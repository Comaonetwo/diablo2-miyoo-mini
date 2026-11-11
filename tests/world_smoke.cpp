// SPDX-License-Identifier: GPL-3.0-or-later
#include <gamedata_load.hpp>
#include <outdoor.hpp>
#include <cstdio>
#include <cstdlib>
#include <set>
int main(int argc,char**argv)try{
 if(argc<3||argc>4)return 2;setvbuf(stdout,nullptr,_IONBF,0);setenv("D2D_SERIAL_LOADING","1",1);
 auto data=d2d::game::load_game_data(argv[1],argv[2],3,argc==4);if(!data){puts("FAIL: game data");return 3;}
 printf("builder=%d act1=%d levels=%zu\n",bool(data->builder),bool(data->builder&&data->builder->act1),data->levels.size());
 data->want_level(2);data->poll_levels();const auto* field=data->level(2);
 if(!field||field->ds1.width()==0){puts("FAIL: Blood Moor missing");return 4;}
 std::set<const d2d::dt1::Tile*> seen;std::size_t logical=0,unique=0,shared=0;
 auto count=[&](const d2d::game::Level& l){for(const auto& a:l.dt1s)for(const auto& tile:a.tiles()){logical+=tile.pixel_storage();if(seen.insert(&tile).second)unique+=tile.pixel_storage();else ++shared;}};
 count(data->town);count(*field);if(shared==0){puts("FAIL: common tiles not shared");return 5;}
 printf("shared_tiles=%zu logical_pixel_bytes=%zu unique_pixel_bytes=%zu saved=%zu\n",shared,logical,unique,logical-unique);
 printf("PASS: Blood Moor %ux%u, town neighbours=%zu, field neighbours=%zu\n",field->ds1.width(),field->ds1.height(),data->town.nearby.size(),field->nearby.size());return 0;
}catch(const std::exception&e){fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
