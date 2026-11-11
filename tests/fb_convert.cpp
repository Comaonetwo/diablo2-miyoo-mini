// SPDX-License-Identifier: GPL-3.0-or-later
#include "fb_convert.hpp"
#include <array>
#include <cstdio>
#include <algorithm>

void check(bool yes) { if (!yes) throw std::runtime_error("framebuffer conversion test failed"); }
int main() try {
    const std::array<std::uint8_t, 16> source{255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255};
    d2mini::FrameFormat format{2,2,12,32,{16,8},{8,8},{0,8},{24,8}};
    std::array<std::uint8_t, 24> dest; dest.fill(0xAA);
    d2mini::convert_frame(source,2,2,dest,format,true);
    auto pixel = [&](unsigned at) { std::uint32_t value; std::memcpy(&value,dest.data()+at,4); return value; };
    check(pixel(0)==0xFFFFFFFF && pixel(4)==0xFF0000FF && pixel(12)==0xFF00FF00 && pixel(16)==0xFFFF0000);
    check(dest[8]==0xAA && dest[20]==0xAA); // row padding untouched
    d2mini::FrameFormat rgb565{2,2,4,16,{11,5},{5,6},{0,5},{0,0}};
    d2mini::convert_frame(source,2,2,dest,rgb565,false);
    std::uint16_t red; std::memcpy(&red,dest.data(),2); check(red==0xF800);
    std::array<std::uint8_t, 64> letterbox{};
    format.width=4;format.height=4;format.pitch=16;
    const std::array<std::uint8_t, 8> wide{255,0,0,255,0,255,0,255};
    d2mini::convert_frame(wide,2,1,letterbox,format,false);
    check(std::all_of(letterbox.begin(),letterbox.begin()+16,[](auto b){return b==0;}));
    check(letterbox[16+2]==255 && letterbox[24+1]==255);
    bool rejected=false;
    try { d2mini::convert_frame(source,2,2,std::span(dest).first(2),format); } catch (...) { rejected=true; }
    check(rejected);
    for(auto [sw,sh]:{std::pair{7u,13u},std::pair{800u,600u}})
      for(auto [dw,dh]:{std::pair{13u,7u},std::pair{640u,480u},std::pair{752u,560u}})
       for(unsigned bits:{16u,32u})for(bool rotate:{false,true}) {
        std::vector<std::uint8_t> rgba(std::size_t(sw)*sh*4);
        for(std::size_t i=0;i<rgba.size();++i)rgba[i]=std::uint8_t(i%251);
        const unsigned bpp=bits/8;
        d2mini::FrameFormat f{dw,dh,dw*bpp+12,bits,bits==16?d2mini::Channel{11,5}:d2mini::Channel{16,8},bits==16?d2mini::Channel{5,6}:d2mini::Channel{8,8},bits==16?d2mini::Channel{0,5}:d2mini::Channel{0,8},bits==16?d2mini::Channel{0,0}:d2mini::Channel{24,8}};
        std::vector<std::uint8_t> actual(std::size_t(f.pitch)*dh,0xA7),expected=actual;
        unsigned w=dw,h=std::uint64_t(w)*sh/sw;
        if(h>dh){h=dh;w=std::uint64_t(h)*sw/sh;}
        const auto bx=(dw-w)/2,by=(dh-h)/2;
        for(unsigned y=0;y<dh;++y)for(unsigned x=0;x<dw;++x) {
            std::uint32_t pixel=0;
            if(x>=bx&&x<bx+w&&y>=by&&y<by+h) {
                const auto sx=std::uint64_t(x-bx)*sw/w,sy=std::uint64_t(y-by)*sh/h;
                const auto* p=rgba.data()+(sy*sw+sx)*4;
                const auto pack=[](std::uint8_t v,d2mini::Channel c){return c.length?(std::uint32_t(v)>>(8-c.length))<<c.offset:0;};
                pixel=pack(p[0],f.red)|pack(p[1],f.green)|pack(p[2],f.blue)|pack(255,f.alpha);
            }
            auto* to=expected.data()+std::size_t(rotate?dh-1-y:y)*f.pitch+(rotate?dw-1-x:x)*bpp;
            std::memcpy(to,&pixel,bpp);
        }
        d2mini::convert_frame(rgba,sw,sh,actual,f,rotate);check(actual==expected);
       }
    // Independent exact overlap-area oracle for the physical 5:4 filter.
    // Includes padded scanout, reversed scanout and last source row/column.
    for(bool reverse:{false,true})for(unsigned padding:{0u,1u,16u}){
        std::vector<std::uint8_t> input(800*600*4);
        for(std::size_t i=0;i<input.size();++i)input[i]=std::uint8_t((i*37+i/7919)%256);
        d2mini::FrameFormat f{640,480,640*4+padding,32,{16,8},{8,8},{0,8},{24,8}};
        std::vector<std::uint8_t> actual(std::size_t(f.pitch)*480,0xA7),expected=actual;
        for(unsigned y=0;y<480;++y)for(unsigned x=0;x<640;++x){
            unsigned sums[3]{};
            // Source pixel intervals in units of 1/4; destination width5.
            for(unsigned sy=y*5/4;sy<(y*5+8)/4;++sy)
                for(unsigned sx=x*5/4;sx<(x*5+8)/4;++sx){
                    const unsigned wx=std::min((sx+1)*4,(x+1)*5)-std::max(sx*4,x*5);
                    const unsigned wy=std::min((sy+1)*4,(y+1)*5)-std::max(sy*4,y*5);
                    for(unsigned c=0;c<3;++c)sums[c]+=input[(sy*800+sx)*4+c]*wx*wy;
                }
            const std::uint32_t value=0xff000000u|(((sums[0]+12)/25)<<16)|(((sums[1]+12)/25)<<8)|((sums[2]+12)/25);
            std::memcpy(expected.data()+(reverse?479-y:y)*f.pitch+(reverse?639-x:x)*4,&value,4);
        }
        d2mini::convert_frame(input,800,600,actual,f,reverse,true);check(actual==expected);
        // Unaligned destination must retain the same bytes and guards via
        // the scalar fallback, including scanout rows with odd pitch.
        std::vector<std::uint8_t> unaligned(actual.size()+2,0xA7);
        d2mini::convert_frame(input,800,600,std::span(unaligned).subspan(1,actual.size()),f,reverse,true);
        check(std::equal(expected.begin(),expected.end(),unaligned.begin()+1)&&unaligned.front()==0xA7&&unaligned.back()==0xA7);
        std::fill(input.begin(),input.end(),255);
        d2mini::convert_frame(input,800,600,actual,f,reverse,true);
        check(actual[0]==255&&actual[1]==255&&actual[2]==255&&actual[3]==255);
        // Thin vertical line in a column dropped by nearest must survive.
        std::fill(input.begin(),input.end(),0);
        for(unsigned y=0;y<600;++y)input[(y*800+4)*4]=255;
        d2mini::convert_frame(input,800,600,actual,f,false,true);
        check(actual[3*4+2]==204); // 4/5 of a red source stroke.
    }
    std::printf("smooth_filter_available=%s\n",d2mini::frame_filter_detail::available()?"neon":"scalar");
    std::puts("PASS: RGB565/ARGB8888, 180-degree rotation, stride padding, letterboxing, bounds");
    return 0;
} catch (const std::exception& e) { std::fprintf(stderr,"FAIL: %s\n",e.what());return 1; }
