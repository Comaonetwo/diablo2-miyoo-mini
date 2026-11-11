// SPDX-License-Identifier: GPL-3.0-or-later
#include <txt.hpp>
#include <dcc.hpp>
#include <dc6.hpp>
#include <mpq.hpp>
#include <fstream>
#include <cstdio>
#include <sstream>

void check(bool yes) { if (!yes) throw std::runtime_error("compact resource comparison failed"); }
using Rows = std::vector<std::vector<std::string>>;
Rows reference(std::span<const std::byte> bytes, bool all) {
    std::istringstream input(std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
    std::string line; Rows rows;
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();
        if (line.empty()) continue;
        std::vector<std::string> cells;
        std::size_t at=0;
        for (;;) { const auto tab=line.find('\t',at);cells.push_back(line.substr(at,tab==line.npos?line.npos:tab-at));if(tab==line.npos)break;at=tab+1; }
        if (rows.empty() || all || (!cells[0].empty() && cells[0]!="Expansion")) rows.push_back(std::move(cells));
    }
    return rows;
}
void table_test(std::span<const std::byte> bytes) {
    for (bool all : {false,true}) {
        const auto expected=reference(bytes,all);
        d2d::txt::Table parsed(bytes,all);
        check(parsed.size() + (expected.empty()?0:1)==expected.size());
        const auto copied=parsed;
        auto moved=std::move(parsed);
        for (std::size_t y=1;y<expected.size();++y) for (std::size_t x=0;x<expected[y].size();++x) {
            check(moved.get(y-1,std::optional{x})==expected[y][x]);
            check(copied.get(y-1,std::optional{x})==expected[y][x]);
        }
        check(moved.get(moved.size(),std::optional<std::size_t>{0}).empty());
        if (!expected.empty()) for(std::size_t x=0;x<expected[0].size();++x) check(moved.col(expected[0][x]).has_value());
    }
}
int main(int argc,char** argv) try {
    const std::string synthetic="Name\tVALUE\tEmpty\r\nFirst\t12\t\r\nExpansion\t99\t\r\n\t0\t\r\nLast\ttext\n";
    table_test({reinterpret_cast<const std::byte*>(synthetic.data()),synthetic.size()});
    if(argc==1){std::puts("PASS: compact tables preserve rows, separators, empty cells, copy/move ownership");return 0;}
    d2d::mpq::Stack stack;
    if(const char* patch=std::getenv("D2_PATCH_INSTALLER"))stack.push_installer(patch);
    std::ifstream manifest(argv[1]);check(bool(manifest));std::string line;
    while(std::getline(manifest,line)){
        const auto tab=line.find('\t'); const auto name=line.substr(0,tab);
        if(name=="d2exp.mpq"||name=="d2data.mpq"||name=="d2char.mpq")stack.push(line.substr(tab+1));
    }
    const char* tables[]={"Skills","MonStats","LvlPrest","LvlSub","LvlTypes","Levels","Objects","Armor","Weapons","Misc","Sounds","SoundEnviron","ItemStatCost"};
    for(auto name:tables)table_test(stack.read(std::string(R"(data\global\excel\)")+name+".txt"));
    unsigned frames=0;
    for(const auto* name:{R"(data\global\CHARS\BA\HD\BAHDHLMTWHTH.dcc)",R"(data\global\CHARS\SO\S1\SOS1LITBLHTH.dcc)"}){
        const auto bytes=stack.read(name); const d2d::dcc::Sprite eager(bytes);d2d::dcc::Sprite lazy{std::vector<std::byte>(bytes)};
        std::array<std::uint8_t,256> tint{};for(unsigned i=0;i<256;++i)tint[i]=i?std::uint8_t(256-i):0;
        for(bool tinted:{false,true}){
            if(tinted)lazy.remap(tint.data());
            // Three passes force direction eviction and reload, including tint after reload.
            for(int pass=0;pass<3;++pass)for(unsigned d=0;d<eager.directions();++d)for(int f=0;f<eager.frames_per_direction();++f){
                const auto& a=eager.frame(d,f);const auto& b=lazy.frame(d,f);
                check(a.width==b.width&&a.height==b.height&&a.x_offset==b.x_offset&&a.y_offset==b.y_offset&&a.box_left==b.box_left&&a.box_top==b.box_top&&a.box_right==b.box_right&&a.box_bottom==b.box_bottom&&a.pixels.size()==b.pixels.size());
                for(std::size_t p=0;p<a.pixels.size();++p)check(b.pixels[p]==(tinted?tint[a.pixels[p]]:a.pixels[p]));
                ++frames;
            }
        }
    }
    unsigned dc6_frames=0;
    for(const auto* name:{R"(data\global\ui\FrontEnd\D2logoFireLeft.DC6)", R"(data\global\ui\FrontEnd\fire.DC6)", R"(data\global\ui\FrontEnd\gameselectscreenEXP.dc6)", R"(data\global\ui\PANEL\invchar6.dc6)", R"(data\global\ui\SPELLS\skltree_b_back.dc6)"}) {
        const auto bytes=stack.read(name); const d2d::dc6::Sprite eager(bytes);
        d2d::dc6::Sprite lazy(bytes,true);
        auto source=std::make_shared<d2d::mpq::Stack>(stack.share());
        auto deferred=d2d::dc6::Sprite::deferred([source,name]{return source->try_read(name);});
        check(eager.directions()==lazy.directions()&&eager.total_frames()==lazy.total_frames());
        for(int pass=0;pass<3;++pass) {
            lazy.release_pixels();
            deferred.release_pixels();
            auto copied=lazy;
            for(unsigned d=0;d<eager.directions();++d)for(unsigned f=0;f<eager.frames_per_direction();++f) {
                const auto& a=eager.frame(d,f);const auto& b=copied.frame(d,f);
                check(a.width==b.width&&a.height==b.height&&a.offset_x==b.offset_x&&a.offset_y==b.offset_y&&a.flipped==b.flipped&&a.pixels==b.pixels);
                const auto& c=deferred.frame(d,f);
                check(a.width==c.width&&a.height==c.height&&a.offset_x==c.offset_x&&a.offset_y==c.offset_y&&a.flipped==c.flipped&&a.pixels==c.pixels);
                ++dc6_frames;
            }
            check(copied.frames().size()==eager.frames().size());
            lazy=std::move(copied);
        }
    }
    std::printf("PASS: %u real DC6 frames compared both demand/deferred, including release/redecode/copy/move\n",dc6_frames);
    std::printf("PASS: 13 real tables, every cell/row; %u real DCC frame comparisons including cache eviction and palette remap\n",frames);
    return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
