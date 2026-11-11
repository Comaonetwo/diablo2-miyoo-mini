// SPDX-License-Identifier: GPL-3.0-or-later
// Golden hashes produced with the 0.2.34 decoder; no game image data included.
#include <dcc.hpp>
#include <mpq.hpp>
#include <fstream>
#include <cstdio>
int main(int argc,char**argv)try{
 if(argc!=3||!std::filesystem::exists(std::filesystem::path(argv[1])/"d2char.mpq")){puts("SKIP: owner game archives required");return 0;}
 d2d::mpq::Stack stack;for(const char* f:{"d2exp.mpq","d2data.mpq","d2char.mpq"})stack.push(std::filesystem::path(argv[1])/f);
 std::ifstream input(argv[2]);if(!input)throw std::runtime_error("missing DCC reference hashes");
 unsigned files=0,frames=0;std::string line;
 while(std::getline(input,line)){
  const auto tab=line.find('\t');if(tab==line.npos)throw std::runtime_error("invalid hash fixture");
  const auto path=line.substr(0,tab);const auto bytes=stack.read(path);d2d::dcc::Sprite sprite(bytes);d2d::dcc::Sprite geometry{std::vector<std::byte>(bytes)};
  d2d::dcc::decode_profile.enabled=true;const auto calls_before=d2d::dcc::decode_profile.calls;
  std::uint64_t hash=1469598103934665603ull;const auto add=[&](std::uint8_t v){hash=(hash^v)*1099511628211ull;};
  for(unsigned d=0;d<sprite.directions();++d)for(int f=0;f<sprite.frames_per_direction();++f){const auto&a=sprite.frame(d,f);const auto&b=geometry.metadata(d,f);
   if(a.width!=b.width||a.height!=b.height||a.x_offset!=b.x_offset||a.y_offset!=b.y_offset||a.box_left!=b.box_left||a.box_top!=b.box_top||a.box_right!=b.box_right||a.box_bottom!=b.box_bottom||!b.pixels.empty())throw std::runtime_error("DCC geometry changed");
   for(int v:{a.width,a.height,a.x_offset,a.y_offset,a.box_left,a.box_top,a.box_right,a.box_bottom})for(int k=0;k<4;++k)add(std::uint8_t(std::uint32_t(v)>>(k*8)));
   for(auto v:a.pixels)add(v);++frames;
  }
  if(d2d::dcc::decode_profile.calls!=calls_before)throw std::runtime_error("geometry decoded pixels");
  for(int d=0;d<std::min<int>(4,geometry.directions());++d)if(geometry.frame(d,0).pixels!=sprite.frame(d,0).pixels)throw std::runtime_error("pixel read after geometry mismatch");
  const auto calls_after=d2d::dcc::decode_profile.calls;
  if(geometry.metadata(0,0).width!=sprite.frame(0,0).width||d2d::dcc::decode_profile.calls!=calls_after)throw std::runtime_error("evicted geometry mismatch");
  geometry.release_pixels();for(const auto&f:geometry.frames())if(f.pixels.capacity())throw std::runtime_error("released pixels retained capacity");if(geometry.metadata(0,0).width!=sprite.frame(0,0).width||!geometry.metadata(0,0).pixels.empty()||d2d::dcc::decode_profile.calls!=calls_after)throw std::runtime_error("released geometry mismatch");
  d2d::dcc::decode_profile.enabled=false;
  if(hash!=std::stoull(line.substr(tab+1)))throw std::runtime_error("DCC changed: "+path);++files;
 }
 if(files!=1161||frames!=106629)throw std::runtime_error("incomplete DCC corpus");
 printf("PASS: %u sprites, %u frames including all metadata/pixels match 0.2.34 golden hashes\n",files,frames);return 0;
}catch(const std::exception&e){fprintf(stderr,"%s\n",e.what());return 1;}
