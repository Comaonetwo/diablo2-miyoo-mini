// SPDX-License-Identifier: GPL-3.0-or-later
#include "gold_pad.hpp"
#include <cstdio>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("gold keypad contract");}
int main(){using d2mini::GoldPad;GoldPad k;std::array<bool,9> keys{};
 k.show(false,5000);keys[4]=true;k.update(keys,.04f);check(k.draft=="5000");k.update(keys,.04f);check(k.draft=="5000");keys={};k.update(keys,.04f);keys[4]=true;k.update(keys,.04f);check(k.draft=="1");k.append("2a5");check(k.amount()==125);k.erase();check(k.amount()==12);k.all();check(k.amount()==5000);k.append("42");check(k.amount()==42);k.accept();check(!k.open&&k.result==42);
 k.show(true,2000);k.append("99999999999999999999999999999");check(k.amount()==2000&&k.draft.size()<=7);keys={};k.update(keys,.04f);keys[5]=true;k.update(keys,.04f);check(!k.open&&!k.result);
 k.show(false,0);k.accept();check(k.open&&!k.result&&!k.notice.empty());
 k.show(false,300);k.append("0");check(k.amount()==0);k.append("0075");check(k.amount()==75);k.click(470,410);check(!k.open&&k.result==75);
 k.show(true,150);k.update({},.04f);keys={};keys[8]=true;k.update(keys,.04f);check(!k.open&&k.result==150);
 k.show(false,10);k.update({},.04f);keys={};keys[0]=true;k.update(keys,.04f);check(k.row==4&&k.col==0);keys={};k.update(keys,.04f);keys[2]=true;k.update(keys,.04f);check(k.col==1);
 puts("PASS numeric keypad: opening-button guard, first-digit replacement, delete/all, cap/overflow, zero, cancellation, confirm/start and navigation");
}
