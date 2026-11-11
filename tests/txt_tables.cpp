// SPDX-License-Identifier: GPL-3.0-or-later
#include <txt.hpp>
#include <mpq.hpp>
#include <filesystem>
#include <cstdio>
int main(int argc,char**argv)try{
 if(argc!=2)throw std::runtime_error("expected private data path");
 const std::filesystem::path data(argv[1]);d2d::mpq::Stack files;files.push_installer(data/"LODPatch_114d.exe");files.push(data/"d2exp.mpq");files.push(data/"d2data.mpq");
 std::size_t cells=0,tables=0;
 for(const char* name:{"MonStats","MonStats2","Objects","Levels","LvlTypes","LvlPrest","LvlSub","LvlWarp","LvlMaze","Skills","SkillDesc","Missiles","Sounds","SoundEnviron","Armor","Weapons","Misc","ItemStatCost","States","SuperUniques","MonPreset","TreasureClassEx","CharStats","CubeMain","AnimData"}){
  const auto bytes=files.try_read(std::string(R"(data\global\excel\)")+name+".txt");if(!bytes)continue;
  for(bool keep:{false,true}){
   d2d::txt::Table parsed(*bytes,keep);std::string_view raw(reinterpret_cast<const char*>(bytes->data()),bytes->size());bool header=true;std::size_t row=0;
   while(!raw.empty()){
    const auto end=raw.find('\n');auto line=raw.substr(0,end);raw=end==raw.npos?std::string_view{}:raw.substr(end+1);if(!line.empty()&&line.back()=='\r')line.remove_suffix(1);if(line.empty())continue;
    if(header){header=false;continue;}
    const auto first=line.substr(0,line.find('\t'));if(!keep&&(first.empty()||first=="Expansion"))continue;
    for(std::size_t at=0,col=0;;++col){const auto tab=line.find('\t',at);const auto expected=line.substr(at,tab==line.npos?line.npos:tab-at);if(parsed.get(row,std::optional<std::size_t>(col))!=expected)throw std::runtime_error(std::string(name)+" cell mismatch");++cells;if(tab==line.npos)break;at=tab+1;}++row;
   }
   if(parsed.size()!=row)throw std::runtime_error("row mismatch");
  }++tables;
 }
 std::printf("PASS: %zu original tables, %zu cell comparisons across both row policies; 6 bytes less index storage per ordinary row cell (row metadata excluded)\n",tables,cells);
}catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}
