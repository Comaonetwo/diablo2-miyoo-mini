// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>
#include <atomic>
namespace d2mini {
// Small breadcrumbs only. No allocation, strings, formatting or stack unwinding
// are attempted inside a fatal-signal handler.
static_assert(std::atomic<unsigned>::is_always_lock_free);
inline std::atomic<unsigned> crash_phase{0},crash_level{0},crash_command{0};
struct CrashRecord {std::uintptr_t signal,code,address,pc,lr,sp,level,phase,command;};
inline std::size_t format_crash(char* out,const CrashRecord& r){
 char* p=out;
 const auto text=[&](const char* s){while(*s)*p++=*s++;};
 const auto field=[&](const char* label,std::uintptr_t value){text(label);text("0x");bool started=false;for(int shift=int(sizeof(value)*8)-4;shift>=0;shift-=4){const unsigned digit=(value>>shift)&15;if(digit||started||shift==0){*p++="0123456789abcdef"[digit];started=true;}}};
 text("fatal");field(" signal=",r.signal);field(" code=",r.code);field(" address=",r.address);field(" pc=",r.pc);field(" lr=",r.lr);field(" sp=",r.sp);field(" level=",r.level);field(" phase=",r.phase);field(" command=",r.command);*p++='\n';return std::size_t(p-out);
}
}
