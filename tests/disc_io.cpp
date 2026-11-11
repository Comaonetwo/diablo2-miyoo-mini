// SPDX-License-Identifier: GPL-3.0-or-later
// Synthetic fixtures only. No original game files are used or copied.
#include <StormLib.h>
#include <FileStream.h>
#include <iso9660.hpp>
#include <array>
#include <cstdio>
#include <fstream>
#include <stdexcept>

TFileStream* D2Disc_Open(const TCHAR*, DWORD);
void check(bool ok) { if (!ok) throw std::runtime_error("disc I/O check failed"); }
int main(int argc, char** argv) try {
    check(argc == 2);
    namespace fs = std::filesystem;
    const fs::path dir(argv[1]); fs::create_directories(dir);
    std::vector<std::byte> logical(24 * 2048);
    auto* pvd = logical.data() + 16 * 2048;
    pvd[0] = std::byte{1}; std::memcpy(pvd + 1, "CD001", 5); pvd[6] = std::byte{1};
    auto u32 = [](std::byte* out, unsigned n) { for (int i = 0; i < 4; ++i) out[i] = std::byte((n >> (i * 8)) & 255); };
    pvd[156] = std::byte{34}; u32(pvd + 158, 20); u32(pvd + 166, 2048);
    auto* end = logical.data() + 17 * 2048;
    end[0] = std::byte{255}; std::memcpy(end + 1, "CD001", 5);
    for (std::size_t i = 21 * 2048; i < logical.size(); ++i) logical[i] = std::byte(i % 251);
    const auto payload = logical.data() + 21 * 2048;
    for (auto [stride, offset] : {std::pair{2048u, 0u}, {2352u, 16u}, {2352u, 24u}}) {
        auto file = dir / (std::to_string(stride) + "-" + std::to_string(offset) + ".img");
        std::vector<std::byte> physical(24 * stride, std::byte{0xCC});
        for (int i = 0; i < 24; ++i) std::memcpy(physical.data() + i * stride + offset, logical.data() + i * 2048, 2048);
        { std::ofstream out(file, std::ios::binary); out.write(reinterpret_cast<const char*>(physical.data()), physical.size()); check(bool(out)); }
        auto reader = iso9660::Reader::open(file); check(reader.has_value());
        check(reader->sector_stride() == stride && reader->payload_offset() == offset);
        std::array<std::byte, 4099> actual{};
        check(reader->read_bytes(21 * 2048 + 3, actual.size(), actual.data()));
        check(std::memcmp(actual.data(), payload + 3, actual.size()) == 0);
        const auto spec = "d2disc:" + std::to_string(stride) + ":" + std::to_string(offset) + ":21:6144:" + fs::absolute(file).string();
        auto* stream = D2Disc_Open(spec.c_str(), STREAM_FLAG_READ_ONLY); check(stream != nullptr);
        ULONGLONG at = 2039;
        check(FileStream_Read(stream, &at, actual.data(), 2111));
        check(std::memcmp(actual.data(), payload + at, 2111) == 0);
        check(FileStream_Read(stream, nullptr, actual.data(), 7));
        check(std::memcmp(actual.data(), payload + at + 2111, 7) == 0);
        at = 6140; check(!FileStream_Read(stream, &at, actual.data(), 5));
        check(!FileStream_Write(stream, nullptr, actual.data(), 1));
        check(!FileStream_SetSize(stream, 1));
        FileStream_Close(stream);
        check(D2Disc_Open(spec.c_str(), 0) == nullptr);
        fs::resize_file(file, 16 * stride + offset + 100);
        check(!iso9660::Reader::open(file));
        check(D2Disc_Open(spec.c_str(), STREAM_FLAG_READ_ONLY) == nullptr);
        fs::remove(file);
    }
    check(D2Disc_Open("d2disc:2352:999:21:6144:/none", STREAM_FLAG_READ_ONLY) == nullptr);
    std::puts("PASS: ISO/raw mode1/raw mode2; unaligned/cross-sector/sequential reads; EOF; truncated input; writes and resize denied");
    return 0;
} catch (const std::exception& e) { std::fprintf(stderr, "FAIL: %s\n", e.what()); return 1; }
