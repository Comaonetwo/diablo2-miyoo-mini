// SPDX-License-Identifier: GPL-3.0-or-later
#include "crash_trace.hpp"
#include <string_view>
#include <cstdio>
int main(){char data[512];const auto n=d2mini::format_crash(data,{11,1,0,0xdeadbeef,0x123,0xffffffff,13,3,5});
 if(std::string_view(data,n)!="fatal signal=0xb code=0x1 address=0x0 pc=0xdeadbeef lr=0x123 sp=0xffffffff level=0xd phase=0x3 command=0x5\n")return 1;
 const auto maximum=std::uintptr_t(-1);if(d2mini::format_crash(data,{maximum,maximum,maximum,maximum,maximum,maximum,maximum,maximum,maximum})>=sizeof(data))return 2;
 std::puts("PASS: bounded fatal record formatting, zero and maximum pointer values");}
