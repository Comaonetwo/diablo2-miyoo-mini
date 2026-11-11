// SPDX-License-Identifier: GPL-3.0-or-later
// Exercise real allocation failures inside a partially decoded direction.
#include <dcc.hpp>
#include <mpq.hpp>
#include <cstdlib>
#include <cstdio>
static int fail_after=-1;static bool injected=false;
void* operator new(std::size_t n){if(fail_after==0){fail_after=-1;injected=true;throw std::bad_alloc{};}if(fail_after>0)--fail_after;auto*p=std::malloc(n?n:1);if(!p)throw std::bad_alloc{};return p;}
void operator delete(void*p)noexcept{std::free(p);}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete[](void*p)noexcept{std::free(p);}
int main(int argc,char**argv)try{
 if(argc!=2||!std::filesystem::exists(std::filesystem::path(argv[1])/"d2char.mpq")){puts("SKIP: owner archive required");return 0;}
 d2d::mpq::Archive archive(std::filesystem::path(argv[1])/"d2char.mpq");auto raw=archive.read(R"(data\global\CHARS\BA\HD\BAHDHLMTWHTH.dcc)");
 unsigned rejected=0;for(std::size_t n=0;n<raw.size();n+=std::max<std::size_t>(1,raw.size()/40)){
  try{d2d::dcc::Sprite truncated(std::span<const std::byte>(raw.data(),n));}catch(const std::exception&){++rejected;}
 }
 if(rejected<30)return 6;
 d2d::dcc::Sprite reference(raw);if(reference.directions()<3)return 2;unsigned failures=0;
 d2d::dcc::Sprite geometry{std::vector<std::byte>(raw)};d2d::dcc::decode_profile={true,0,0};
 for(unsigned dir=0;dir<geometry.directions();++dir)for(int f=0;f<geometry.frames_per_direction();++f){const auto&a=reference.frame(dir,f);const auto&b=geometry.metadata(dir,f);if(a.width!=b.width||a.height!=b.height||a.box_left!=b.box_left||a.box_top!=b.box_top||a.box_right!=b.box_right||a.box_bottom!=b.box_bottom||!b.pixels.empty())return 7;}
 if(d2d::dcc::decode_profile.calls)return 8;
 for(unsigned dir:{0u,2u,1u})if(geometry.frame(dir,0).pixels!=reference.frame(dir,0).pixels)return 9;
 for(int f=0;f<geometry.frames_per_direction();++f)if(geometry.metadata(0,f).pixels.capacity()!=0)return 11;
 const auto calls=d2d::dcc::decode_profile.calls;geometry.release_pixels();if(geometry.metadata(0,0).width!=reference.frame(0,0).width||!geometry.metadata(0,0).pixels.empty()||d2d::dcc::decode_profile.calls!=calls)return 10;
 for(const auto&f:geometry.frames())if(f.pixels.capacity()!=0)return 12;
 d2d::dcc::decode_profile.enabled=false;

 for(bool tinted:{false,true})for(int point=0;point<500;++point){
  d2d::dcc::Sprite sprite{std::vector<std::byte>(raw)};
  std::array<std::uint8_t,256> map;for(unsigned i=0;i<256;++i)map[i]=std::uint8_t(i*3+7);
  if(tinted)sprite.remap(map.data());(void)sprite.frame(0,0);(void)sprite.frame(2,0);
  injected=false;fail_after=point;(void)sprite.frame(1,0);fail_after=-1;
  for(unsigned dir:{1u,0u,2u})for(int f=0;f<reference.frames_per_direction();++f){const auto&a=reference.frame(dir,f);const auto&b=sprite.frame(dir,f);
   if(a.width!=b.width||a.height!=b.height||a.x_offset!=b.x_offset||a.y_offset!=b.y_offset||a.pixels.size()!=b.pixels.size())return 3;
   for(std::size_t i=0;i<a.pixels.size();++i)if(b.pixels[i]!=(tinted&&a.pixels[i]?map[a.pixels[i]]:a.pixels[i]))return 4;
  }
  if(!injected)break;++failures;
 }
 if(failures<20)return 5;printf("PASS: %u injected allocation failures, retry, eviction, partial frame cleanup and palette mapping\n",failures);return 0;
}catch(const std::exception&e){fail_after=-1;fprintf(stderr,"%s\n",e.what());return 1;}
