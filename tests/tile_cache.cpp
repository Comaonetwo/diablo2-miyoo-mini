// SPDX-License-Identifier: GPL-3.0-or-later
#include <gamedata.hpp>
#include <cstring>
#include <cstdio>
struct Stream {
 std::shared_ptr<std::vector<std::byte>> bytes;std::size_t pos=0;
 std::size_t size()const{return bytes->size();}std::size_t seek(std::size_t n){return pos=n;}
 std::size_t read(void* out,std::size_t n){n=std::min(n,size()-pos);std::memcpy(out,bytes->data()+pos,n);pos+=n;return n;}
};
void check(bool b){if(!b)throw std::runtime_error("tile cache resource/lifetime failure");}
int main(){
 auto bytes=std::make_shared<std::vector<std::byte>>(0x114+96+20+256);
 const auto put=[&](int p,int v){std::memcpy(bytes->data()+p,&v,4);};
 put(0,7);put(4,6);put(0x10c,1);put(0x110,0x114);put(0x114+8,80);put(0x114+12,160);(*bytes)[0x114+40]=std::byte{0x27};
 put(0x114+72,0x114+96);put(0x114+80,1);put(0x114+96+8,1);put(0x114+96+10,256);put(0x114+96+16,20);
 for(int i=0;i<256;++i)(*bytes)[0x114+96+20+i]=std::byte{42};
 auto data=std::make_unique<d2d::game::GameData>();
 data->tile_cache->archives.emplace("test",d2d::dt1::Archive::from_lazy_stream(Stream{bytes}));
 const auto* tile=data->tile_cache->archives.at("test").tiles().data();check(tile->pixel_storage()==0);
 std::vector<std::uint8_t> row;const auto* decoded=tile->row(0,row);std::vector<std::uint8_t> original(decoded,decoded+160);
 check(tile->pixel_storage()>0&&tile->subtile_flags[0]==0x27);
 for(int i=0;i<40;++i){data->trim_tile_pixels(0);check(tile->pixel_storage()==0&&tile->subtile_flags[0]==0x27);check(std::memcmp(tile->row(0,row),original.data(),160)==0);check(tile==data->tile_cache->archives.at("test").tiles().data());}
 puts("PASS: zero-budget eviction/reload keeps tile addresses, collision and exact pixels through 40 cycles");
}
