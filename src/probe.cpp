// SPDX-License-Identifier: GPL-3.0-or-later
#include <iso9660.hpp>
#include <mpq.hpp>
#include <dc6.hpp>
#include <dcc.hpp>
#include <palette.hpp>
#include <chrono>
#include <cstdio>
#include <map>
#include <sys/resource.h>

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

int main(int argc, char** argv) try {
    if (argc != 3 && argc != 4) {
        std::fprintf(stderr, "Usage: d2-probe <disc-directory> <LODPatch_114d.exe>\n"); return 2;
    }
    const auto started = Clock::now();
    std::map<std::string, std::string> archives;
    for (const auto& file : fs::recursive_directory_iterator(argv[1])) {
        if (!file.is_regular_file()) continue;
        auto ext = file.path().extension().string();
        for (auto& c : ext) c = char(std::tolower(static_cast<unsigned char>(c)));
        if (ext != ".img" && ext != ".iso") continue;
        auto reader = iso9660::Reader::open(file.path());
        if (!reader) throw std::runtime_error("Unreadable disc: " + file.path().filename().string());
        std::printf("disc=%s sector=%u payload=%u\n", file.path().filename().c_str(), reader->sector_stride(), reader->payload_offset());
        for (const auto& entry : reader->entries()) {
            std::string name = fs::path(entry.path).filename().string();
            for (auto& c : name) c = char(std::tolower(static_cast<unsigned char>(c)));
            if (entry.directory || !name.ends_with(".mpq") || name == "setup.mpq" || name == "playd2.mpq") continue;
            const auto spec = "d2disc:" + std::to_string(reader->sector_stride()) + ":" + std::to_string(reader->payload_offset()) + ":" +
                std::to_string(entry.lba) + ":" + std::to_string(entry.size) + ":" + fs::absolute(file.path()).string();
            if (!archives.emplace(name, spec).second) throw std::runtime_error("Duplicate archive: " + name);
            std::printf("archive=%s bytes=%u\n", name.c_str(), entry.size);
        }
    }
    const char* required[] = {"d2exp.mpq", "d2data.mpq", "d2char.mpq", "d2sfx.mpq", "d2speech.mpq", "d2music.mpq", "d2video.mpq", "d2xtalk.mpq", "d2xmusic.mpq", "d2xvideo.mpq"};
    d2d::mpq::Stack stack;
    stack.push_installer(argv[2]);
    for (auto name : required) {
        const auto it = archives.find(name);
        if (it == archives.end()) throw std::runtime_error(std::string("Missing archive: ") + name);
        stack.push(it->second);
    }
    // Validate a real delta against the original file, not Stack's permissive fallback.
    const auto patch = d2d::mpq::Archive::installer(argv[2]);
    const char* table = R"(data\global\excel\armor.txt)";
    const auto delta = patch.installer_entry(table);
    std::optional<std::vector<std::byte>> base;
    for (auto name : required) {
        d2d::mpq::Archive archive(archives.at(name));
        base = archive.try_read(table);
        if (base) break;
    }
    if (!delta || !base) throw std::runtime_error("Patch/base armor table missing");
    const auto applied = d2d::mpq::bnpatch::apply(*delta, *base);
    if (!applied || *applied != stack.read(table)) throw std::runtime_error("Patch delta validation failed");
    std::printf("patch_delta=OK table_bytes=%zu crc32=%08x\n", applied->size(), d2d::mpq::bnpatch::crc32(*applied));

    const auto image_bytes = stack.read(R"(data\global\ui\FrontEnd\gameselectscreenEXP.dc6)");
    const d2d::dc6::Sprite background(image_bytes);
    const d2d::palette::Palette palette(stack.read(R"(data\global\palette\sky\pal.dat)"));
    const auto anim_bytes = stack.read(R"(data\global\CHARS\BA\HD\BAHDHLMTWHTH.dcc)");
    const d2d::dcc::Sprite anim(anim_bytes);
    std::size_t decoded = 0;
    for (const auto& frame : background.frames()) decoded += frame.pixels.size();
    for (const auto& frame : anim.frames()) decoded += frame.pixels.size();
    std::printf("decoded_background_frames=%zu animation_directions=%u frames_per_direction=%d decoded_pixel_bytes=%zu\n",
        background.frames().size(), anim.directions(), anim.frames_per_direction(), decoded);
    if (!anim.directions() || !anim.frames_per_direction() || background.frames().empty()) throw std::runtime_error("Empty decoded resource");
    constexpr int width = 800, height = 600;
    std::vector<std::uint8_t> framebuffer(width * height * 4);
    auto draw = [&](const auto& f, int left, int top) {
        for (int y = 0; y < int(f.height); ++y) for (int x = 0; x < int(f.width); ++x) {
            const int xx = left + x, yy = top + y;
            if (xx < 0 || yy < 0 || xx >= width || yy >= height) continue;
            const auto index = f.pixels[std::size_t(y) * f.width + x];
            if (!index) continue;
            const auto c = palette[index];
            auto* out = framebuffer.data() + (yy * width + xx) * 4;
            out[0] = c.r; out[1] = c.g; out[2] = c.b; out[3] = 255;
        }
    };
    const auto render_start = Clock::now();
    std::uint64_t checksum = 0;
    for (int tick = 0; tick < 120; ++tick) {
        std::fill(framebuffer.begin(), framebuffer.end(), 0);
        int x = 0, y = 0;
        for (const auto& f : background.frames()) {
            draw(f, x, y); x += int(f.width);
            if (x >= width) { x = 0; y += int(f.height); }
        }
        const auto& f = anim.frame(0, tick % anim.frames_per_direction());
        draw(f, width / 2 + f.x_offset, height / 2 + f.y_offset);
        for (auto byte : framebuffer) checksum = (checksum ^ byte) * 1099511628211ull;
    }
    const auto millis = [](auto duration) { return std::chrono::duration<double, std::milli>(duration).count(); };
    struct rusage usage{}; getrusage(RUSAGE_SELF, &usage);
    std::printf("cpu_draw_120_frames_ms=%.3f (includes checksum; excludes display) checksum=%llu peak_rss_native_units=%ld total_ms=%.3f\n",
        millis(Clock::now() - render_start), static_cast<unsigned long long>(checksum), usage.ru_maxrss, millis(Clock::now() - started));
    std::puts("PASS: read-only disc -> MPQ -> patch delta -> DC6/DCC -> CPU framebuffer; no game assets written");
    if (argc == 4) {
        std::ofstream manifest(argv[3]);
        for (const auto& [name, spec] : archives) manifest << name << '\t' << spec << '\n';
        if (!manifest) throw std::runtime_error("Cannot write disc reference manifest");
    }
    return 0;
} catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL: %s\n", error.what()); return 1;
}
