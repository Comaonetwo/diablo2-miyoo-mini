// SPDX-License-Identifier: GPL-3.0-or-later
#include <txt.hpp>
#include <cstdio>
#include <string>
#include <vector>
void check(bool value){if(!value)throw std::runtime_error("compact TXT storage mismatch");}
int main(){
 for(bool crlf:{false,true})for(bool tail:{false,true})for(bool all:{false,true}){
  const std::string ending=crlf?"\r\n":"\n";
  std::string source="Name\tA\tEmpty\tLast"+ending+"Alpha\t12\t\tend"+ending+"Expansion\t0\t\t"+ending+"\t3\t\t"+ending+"Beta\tlong text\t\t";
  if(tail)source+=ending;
  d2d::txt::Table parsed(std::as_bytes(std::span(source)),all);
  auto copy=parsed;auto moved=std::move(copy);source.clear();source.shrink_to_fit();
  for(const auto* t:{&parsed,&moved}){
   check(t->size()==(all?4:2));check(t->col("nAmE")==0);check(t->get(0,"a")=="12");check(t->get(0,"Empty").empty());check(t->get(0,"Last")=="end");
   const auto row=all?3:1;check(t->get(row,"name")=="Beta");check(t->get(row,"A")=="long text");check(t->get(row,"Last").empty());check(t->get(99,"A").empty());check(t->get(0,"missing").empty());
  }
 }
 std::string wide="Name\tValue\tEnd\nX\t"+std::string(70000,'z')+"\tlast\n";
 d2d::txt::Table long_row(std::as_bytes(std::span(wide)));check(long_row.get(0,"Value")==std::string(70000,'z'));check(long_row.get(0,"End")=="last");
 std::string internal="Name\tValue\tEnd\nX\ta\rb\tlast\r\n";d2d::txt::Table cr(std::as_bytes(std::span(internal)));check(cr.get(0,"Value")=="a\rb");check(cr.get(0,"End")=="last");
 puts("PASS: compact TXT offsets preserve empty fields, final fields, CRLF/LF, section rows, case lookup, copy and move");
}
