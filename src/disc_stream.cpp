// SPDX-License-Identifier: GPL-3.0-or-later
// Read-only StormLib view of a single MPQ extent inside an ISO/raw CD image.
// No extracted files, mapping of whole archives, or filesystem mounts.
#include <StormLib.h>
#include <FileStream.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

namespace {
static_assert(sizeof(off_t) >= 8, "Disc streams require 64-bit file offsets");
struct DiscStream : TFileStream {
    int fd;
    std::uint64_t start;
    std::uint32_t stride, payload;
};

bool read_disc(TFileStream* base, ULONGLONG* offset, void* buffer, DWORD count) {
    auto* s = static_cast<DiscStream*>(base);
    auto pos = offset ? *offset : s->StreamPos;
    if (pos > s->StreamSize || count > s->StreamSize - pos) {
        SetLastError(ERROR_HANDLE_EOF); return false;
    }
    auto* out = static_cast<unsigned char*>(buffer);
    while (count) {
        const auto logical = s->start + pos;
        const auto within = logical % 2048;
        const auto bytes = std::min<DWORD>(count, 2048 - within);
        const auto physical = logical / 2048 * s->stride + s->payload + within;
        ssize_t got;
        do { got = pread(s->fd, out, bytes, static_cast<off_t>(physical)); } while (got < 0 && errno == EINTR);
        if (got <= 0) { SetLastError(ERROR_HANDLE_EOF); return false; }
        out += got; pos += got; count -= static_cast<DWORD>(got);
    }
    s->StreamPos = pos;
    return true;
}
bool get_size(TFileStream* s, ULONGLONG* out) { *out = s->StreamSize; return true; }
bool get_pos(TFileStream* s, ULONGLONG* out) { *out = s->StreamPos; return true; }
void close_disc(TFileStream* s) { close(static_cast<DiscStream*>(s)->fd); }
}

TFileStream* D2Disc_Open(const TCHAR* filename, DWORD flags) {
    if (!(flags & STREAM_FLAG_READ_ONLY)) { SetLastError(ERROR_ACCESS_DENIED); return nullptr; }
    std::string_view spec(filename);
    spec.remove_prefix(7);
    // d2disc:<stride>:<payload>:<lba>:<byte-size>:<absolute-image-path>
    std::uint64_t fields[4]{};
    for (auto& field : fields) {
        auto end = spec.find(':');
        if (end == spec.npos) { SetLastError(ERROR_INVALID_PARAMETER); return nullptr; }
        const auto parsed = std::from_chars(spec.data(), spec.data() + end, field);
        if (parsed.ec != std::errc{} || parsed.ptr != spec.data() + end) {
            SetLastError(ERROR_INVALID_PARAMETER); return nullptr;
        }
        spec.remove_prefix(end + 1);
    }
    const auto [stride, payload, lba, size] = std::array{fields[0], fields[1], fields[2], fields[3]};
    if (!((stride == 2048 && payload == 0) || (stride == 2352 && (payload == 16 || payload == 24))) ||
        lba > UINT32_MAX || size == 0 || size > UINT32_MAX || spec.empty() || spec.front() != '/') {
        SetLastError(ERROR_INVALID_PARAMETER); return nullptr;
    }
    const int fd = open(spec.data(), O_RDONLY);
    if (fd < 0) { SetLastError(ERROR_FILE_NOT_FOUND); return nullptr; }
    struct stat st{};
    const auto last = lba * 2048 + size - 1;
    const auto physical_end = last / 2048 * stride + payload + last % 2048 + 1;
    if (fstat(fd, &st) || st.st_size < 0 || physical_end > static_cast<std::uint64_t>(st.st_size)) {
        close(fd); SetLastError(ERROR_HANDLE_EOF); return nullptr;
    }
    const auto name_bytes = std::strlen(filename) + 1;
    auto* s = static_cast<DiscStream*>(std::calloc(1, sizeof(DiscStream) + name_bytes));
    if (!s) { close(fd); SetLastError(ERROR_NOT_ENOUGH_MEMORY); return nullptr; }
    s->szFileName = reinterpret_cast<char*>(s + 1);
    std::memcpy(s->szFileName, filename, name_bytes);
    s->fd = fd; s->start = lba * 2048;
    s->stride = static_cast<std::uint32_t>(stride); s->payload = static_cast<std::uint32_t>(payload);
    s->dwFlags = STREAM_FLAG_READ_ONLY;
    s->StreamSize = size; s->Base.File.FileSize = size;
    s->StreamRead = read_disc; s->StreamGetSize = get_size;
    s->StreamGetPos = get_pos; s->StreamClose = close_disc;
    return s;
}
