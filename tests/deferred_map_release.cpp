// SPDX-License-Identifier: GPL-3.0-or-later
#include <outdoor.hpp>
#include <cstring>
#include <cstdio>
#include <stdexcept>
int main(){
 const std::array<std::uint32_t,12> words{18,0,0,0,0,0,0,1,0x12340001,0,0,0};
 std::vector<std::byte> bytes(sizeof(words));std::memcpy(bytes.data(),words.data(),sizeof(words));
 int reads=0;auto map=d2d::drlg::LazyMap::deferred([&]()->std::optional<std::vector<std::byte>>{++reads;return bytes;});auto alias=map;
 for(int generation=1;generation<=4;++generation){
  const auto* a=map.get();const auto* b=alias.get();
  if(!a||a!=b||reads!=generation||a->width()!=1||a->height()!=1)throw std::runtime_error("deferred map reload/shared identity");
  alias.release_deferred();map.release_deferred(); // duplicate preset aliases are harmless
 }
 auto encoded=d2d::drlg::LazyMap::encoded(bytes);const auto* saved=encoded.get();encoded.release_deferred();
 if(!saved||encoded.get()!=saved)throw std::runtime_error("encoded-only map must stay resident");
 d2d::ds1::Map borrowed(2,3,1,1);d2d::drlg::LazyMap ref(&borrowed);ref.release_deferred();
 if(ref.get()!=&borrowed)throw std::runtime_error("borrowed map changed");
 std::puts("PASS: deferred maps reload across game boundaries; aliases, encoded and borrowed maps preserved");
}
