// SPDX-License-Identifier: GPL-3.0-or-later
#include <dc6.hpp>
#include <array>
#include <cstdio>
void check(bool ok){if(!ok)throw std::runtime_error("deferred DC6 check failed");}
int main()try{
    std::vector<std::byte> bytes(110);
    auto put=[&](std::size_t at,std::uint32_t v){std::memcpy(bytes.data()+at,&v,4);};
    put(0,6);put(16,1);put(20,2);put(24,32);put(28,71);
    for(unsigned i=0;i<2;++i){const unsigned at=32+i*39;put(at+4,2);put(at+8,1);put(at+12,i+3);put(at+16,8);put(at+28,4);bytes[at+32]=std::byte{2};bytes[at+33]=std::byte(i+1);bytes[at+34]=std::byte(i+2);bytes[at+35]=std::byte{0x80};}
    int reads=0;auto lazy=d2d::dc6::Sprite::deferred([&]()->std::optional<std::vector<std::byte>>{++reads;return bytes;});
    check(reads==0);check(lazy.total_frames()==2&&reads==1);
    auto copy=lazy;check(copy.frame(0,1).pixels==std::vector<std::uint8_t>({2,3})&&reads==1);
    check(copy.frame(0,1).offset_x==4);
    lazy.release_pixels();check(reads==1);check(copy.frame(0,0).pixels==std::vector<std::uint8_t>({1,2})&&reads==2);
    auto moved=std::move(copy);check(moved.directions()==1&&moved.frames_per_direction()==2);
    check(moved.frames().size()==2&&reads==2);
    int attempts=0;auto retry=d2d::dc6::Sprite::deferred([&]()->std::optional<std::vector<std::byte>>{if(++attempts==1)throw std::bad_alloc{};return bytes;});
    bool failed=false;try{retry.total_frames();}catch(const std::bad_alloc&){failed=true;}
    check(failed&&retry.frame(0,0).pixels.size()==2&&attempts==2);
    auto missing=d2d::dc6::Sprite::deferred([]()->std::optional<std::vector<std::byte>>{return {};});
    check(missing.total_frames()==0&&missing.frames().empty());
    std::puts("PASS: no read before use, shared copy/move, release and reload, failed-allocation retry, absent optional sprite");return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
