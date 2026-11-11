// SPDX-License-Identifier: GPL-3.0-or-later
#include <dt1.hpp>
#include <mpq.hpp>
#include <txt.hpp>
#include <fstream>
#include <set>
#include <cstdio>

void check(bool yes) { if (!yes) throw std::runtime_error("compact tile differs from original pixels"); }
int main(int argc, char** argv) try {
    for (int variant = 0; variant < 4; ++variant) {
        d2d::dt1::Tile tile;
        tile.width = 160; tile.height = 80; tile.pixels.resize(160 * 80);
        for (std::size_t i = 0; i < tile.pixels.size(); ++i)
            tile.pixels[i] = variant == 0 ? 0 : variant == 1 ? std::uint8_t(i % 255 + 1) :
                variant == 2 ? std::uint8_t(i % 2) : (i % 41 == 0 ? 19 : 0);
        const auto original = tile.pixels;
        tile.compact();
        std::vector<std::uint8_t> scratch;
        for (std::size_t y = 0; y < 80; ++y) check(std::memcmp(tile.row(y, scratch), original.data() + y * 160, 160) == 0);
    }
    // Shared archives must retain their tile storage after the source dies.
    d2d::dt1::Archive kept;
    const d2d::dt1::Tile* original_tile=nullptr;
    {
        std::vector<std::byte> bytes(0x114+96);
        auto put=[&](int at,int value){std::memcpy(bytes.data()+at,&value,4);};
        put(0,7);put(4,6);put(0x10c,1);put(0x110,0x114);
        put(0x114+8,-80);put(0x114+12,160);
        d2d::dt1::Archive source(bytes,d2d::dt1::Pixels::compact);
        original_tile=source.tiles().data();kept=source;
        check(kept.tiles().data()==original_tile);
    }
    check(kept.size()==1&&kept.tiles().data()==original_tile&&kept.tiles()[0].width==160);
    if (argc == 1) { std::puts("PASS: transparent, opaque, alternating and sparse tile rows match"); return 0; }
    d2d::mpq::Stack stack;
    if (const char* patch = std::getenv("D2_PATCH_INSTALLER")) stack.push_installer(patch);
    std::ifstream manifest(argv[1]); check(bool(manifest));
    std::string line;
    while (std::getline(manifest, line)) {
        const auto tab = line.find('\t');
        if (line.substr(0, tab) == "d2exp.mpq" || line.substr(0, tab) == "d2data.mpq") stack.push(line.substr(tab + 1));
    }
    const d2d::txt::Table types(stack.read(R"(data\global\excel\LvlTypes.txt)"));
    std::set<std::string> files{R"(Act1\Outdoors\Blank.dt1)", R"(Act1\Barracks\InvisWal.dt1)", R"(Act1\Barracks\Warp.dt1)", R"(Act1\Town\floor.dt1)"};
    for (std::size_t i = 0; i < types.size(); ++i) {
        const int type = std::atoi(std::string(types.get(i, "Id")).c_str());
        if (type < 1 || type > 4) continue;
        for (int slot = 1; slot <= 32; ++slot) {
            const auto file = types.get(i, "File " + std::to_string(slot));
            if (!file.empty() && file != "0") files.emplace(file);
        }
    }
    std::size_t tested = 0, tiles = 0, raw_size = 0, compact_size = 0;
    for (const auto& file : files) {
        auto path = R"(data\global\tiles\)" + file;
        for (auto& c : path) if (c == '/') c = '\\';
        const auto raw = stack.read(path);
        const d2d::dt1::Archive before(raw), after(raw, d2d::dt1::Pixels::compact);
        auto file_stream=stack.open(path);check(bool(file_stream));
        const auto streamed=d2d::dt1::Archive::from_stream(*file_stream);
        auto lazy_stream=stack.open(path);auto lazy=d2d::dt1::Archive::from_lazy_stream(std::move(*lazy_stream));
        for(const auto& tile:lazy.tiles())check(tile.pixel_storage()==0);

        check(before.size() == after.size()&&streamed.size()==after.size());
        for (std::size_t i = 0; i < before.size(); ++i) {
            const auto& a = before.tiles()[i]; const auto& b = after.tiles()[i];
            const auto& c=streamed.tiles()[i];check(c.packed==b.packed&&c.rows==b.rows&&c.pixels==b.pixels&&c.subtile_flags==b.subtile_flags&&c.y_shift==b.y_shift);
            check(a.width == b.width && a.height == b.height && a.y_shift == b.y_shift && a.subtile_flags == b.subtile_flags);
            std::vector<std::uint8_t> scratch;
            if (a.width > 0) for (std::size_t y = 0; y < a.pixels.size() / std::size_t(a.width); ++y)
                check(std::memcmp(a.pixels.data() + y * a.width, b.row(y, scratch), a.width) == 0);
            const auto& demand=lazy.tiles()[i];std::vector<std::uint8_t> lazy_row;
            for(int pass=0;pass<2;++pass){
                if(a.width>0)for(std::size_t y=0;y<a.pixels.size()/std::size_t(a.width);++y)check(std::memcmp(a.pixels.data()+y*a.width,demand.row(y,lazy_row),a.width)==0);
                check(demand.subtile_flags==a.subtile_flags);demand.release_pixels();check(demand.pixel_storage()==0);
            }
            raw_size += a.pixel_storage(); compact_size += b.pixel_storage(); ++tiles;
        }
        ++tested;
    }
    std::printf("PASS: %zu real DT1 files, %zu tiles, all decoded rows/flags/offsets identical; raw=%zu compact=%zu bytes\n", tested, tiles, raw_size, compact_size);
    return 0;
} catch (const std::exception& e) { std::fprintf(stderr, "FAIL: %s\n", e.what()); return 1; }
