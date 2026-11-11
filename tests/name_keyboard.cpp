// SPDX-License-Identifier: GPL-3.0-or-later
#include "name_keyboard.hpp"
#include <stdexcept>
#include <cstdio>
void check(bool x){if(!x)throw std::runtime_error("name keyboard contract");}
int main(){
 d2mini::NameKeyboard k;k.show("");k.append("-a1b c_");check(k.draft=="abc_");k.accept();check(k.open&&!k.result&&!k.notice.empty());k.erase();k.accept();check(!k.open&&k.result=="abc");
 k.show("Hero");k.append("-a_b'c");check(k.draft=="Hero-abc");k.append("defghijklmnop");check(k.draft.size()==15);k.accept();check(!k.open);
 k.show("OldName");k.append("New");std::array<bool,8> a{};a[5]=true;k.update(a,.04f);check(!k.open&&!k.result);
 k.show("");a={};a[4]=true;k.update(a,.04f);k.update(a,.04f);check(k.draft=="a");a={};k.update(a,.04f);a[7]=true;k.update(a,.04f);check(k.upper);a={};k.update(a,.04f);a[4]=true;k.update(a,.04f);check(k.draft=="aA");
 k.row=2;k.col=9;k.choose();check(k.draft=="a");k.row=3;k.col=1;k.choose();check(k.open);k.append("b");k.choose();check(!k.open&&k.result=="ab");
 k.show("Valid");a={};a[0]=true;k.update(a,.04f);check(k.row==3&&k.col==0);a={};k.update(a,.04f);a[2]=true;k.update(a,.04f);check(k.col==2);a={};k.update(a,.04f);a[4]=true;k.update(a,.04f);check(!k.open&&!k.result);
 puts("PASS: name constraints, draft commit/cancel, button edges, case and wrapped navigation");
}
