// SPDX-License-Identifier: GPL-3.0-or-later
// Test utility: project the game screenshot through the actual device converter.
#include "fb_convert.hpp"
#include <iostream>
int main(int argc,char** argv){
 std::vector<std::uint8_t> rgba(800*600*4),bgra(640*480*4),rgb(640*480*3);
 if(!std::cin.read(reinterpret_cast<char*>(rgba.data()),rgba.size()))return 1;
 d2mini::convert_frame(rgba,800,600,bgra,{640,480,2560,32,{16,8},{8,8},{0,8},{24,8}},false,argc<2||argv[1][0]!='0');
 for(unsigned i=0;i<640*480;++i){rgb[i*3]=bgra[i*4+2];rgb[i*3+1]=bgra[i*4+1];rgb[i*3+2]=bgra[i*4];}
 std::cout.write(reinterpret_cast<char*>(rgb.data()),rgb.size());return !std::cout;
}
