// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
#include <string_view>
#include <cstddef>
namespace d2mini {
inline std::string effect_key(std::string_view name){
    std::string key(name);for(auto& c:key){if(c>='A'&&c<='Z')c=char(c-'A'+'a');if(c=='/')c='\\';}return key;
}
struct EffectDirections{unsigned count=2;std::size_t pixels=0;};
inline EffectDirections effect_directions(std::string_view key){
    if(key=="icebreaksmall"||key=="icebreaklarge"||key=="icebreaksmallmelt"||key=="icebreaklargemelt")return {4,512*1024};
    if(key=="chargedbolt"||key=="electricnova"||key=="frostnova"||key=="fireball"||key=="firebolt"||key=="icebolt"||key=="teethmissile")return {32,768*1024};
    return {};
}
inline constexpr std::size_t kEffectCacheBudget=5*1024*1024;
}
