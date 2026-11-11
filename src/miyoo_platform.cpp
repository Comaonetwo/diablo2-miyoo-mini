// SPDX-License-Identifier: GPL-3.0-or-later
#include "miyoo_platform.hpp"
#include "fb_convert.hpp"
#include "crash_trace.hpp"
#if defined(__arm__) && defined(__linux__)
#include <signal.h>
#include <ucontext.h>
#endif
#include <linux/fb.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace d2mini {
namespace { std::unique_ptr<Platform> active; }
Platform* current() { return active.get(); }
void enable() {
    active = std::make_unique<Platform>();
#if defined(__arm__) && defined(__linux__)
    struct sigaction action{};
    action.sa_sigaction=[](int sig,siginfo_t* info,void* raw){
        const auto& regs=static_cast<ucontext_t*>(raw)->uc_mcontext;
        char line[512];
        const auto size=format_crash(line,{std::uintptr_t(sig),std::uintptr_t(info->si_code),reinterpret_cast<std::uintptr_t>(info->si_addr),regs.arm_pc,regs.arm_lr,regs.arm_sp,crash_level.load(std::memory_order_relaxed),crash_phase.load(std::memory_order_relaxed),crash_command.load(std::memory_order_relaxed)});
        (void)write(STDERR_FILENO,line,size);
        _exit(128+sig);
    };
    sigemptyset(&action.sa_mask);action.sa_flags=SA_SIGINFO|SA_RESETHAND;
    sigaction(SIGSEGV,&action,nullptr);sigaction(SIGBUS,&action,nullptr);sigaction(SIGILL,&action,nullptr);
#endif
}
const char* allocation_phase="startup";
void memory_checkpoint(const char* phase) {
    if (std::strcmp(phase,"C++ allocation failed before unwind") != 0)
        __atomic_store_n(&allocation_phase,phase,__ATOMIC_RELAXED);
    char prefix[192]; const int count = std::snprintf(prefix, sizeof(prefix), "memory_checkpoint: %s\n", phase);
    if (count > 0) write(STDERR_FILENO, prefix, std::min<std::size_t>(count, sizeof(prefix)-1));
    const int fd = open("/proc/self/status", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return;
    char status[4096]; const auto got = read(fd, status, sizeof(status)-1); close(fd);
    if (got <= 0) return;
    status[got] = 0;
    for (const char* p = status; *p;) {
        const char* end = std::strchr(p, '\n'); if (!end) end = p + std::strlen(p);
        if (std::strncmp(p, "Vm", 2) == 0 || std::strncmp(p, "Threads:", 8) == 0) { write(STDERR_FILENO, p, end-p); write(STDERR_FILENO, "\n", 1); }
        p = *end ? end + 1 : end;
    }
}
struct Platform::Impl {
    int fb = -1, input = -1;
    std::uint8_t* mapping = nullptr;
    fb_var_screeninfo v{};
    fb_fix_screeninfo f{};
    std::size_t offset = 0, size = 0;
    std::vector<std::uint8_t> original;
    std::array<bool, KEY_MAX + 1> keys{};
    bool resync = false;
    std::uint64_t frames = 0;
    std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
    ~Impl() {
        rusage usage{}; getrusage(RUSAGE_SELF, &usage);
        std::fprintf(stdout, "miyoo_exit peak_rss_kib=%ld\n", usage.ru_maxrss);
        if (mapping) { std::copy(original.begin(), original.end(), mapping + offset); munmap(mapping, f.smem_len); }
        if (fb >= 0) close(fb);
        if (input >= 0) close(input);
    }
    void reconcile() {
        std::array<unsigned long, (KEY_MAX + 8 * sizeof(unsigned long)) / (8 * sizeof(unsigned long))> bitmap{};
        keys.fill(false);
        if (ioctl(input, EVIOCGKEY(sizeof(bitmap)), bitmap.data()) >= 0)
            for (unsigned i = 0; i <= KEY_MAX; ++i) keys[i] = (bitmap[i / (8 * sizeof(unsigned long))] >> (i % (8 * sizeof(unsigned long)))) & 1;
    }
};

Platform::Platform() : impl_(std::make_unique<Impl>()) {
    auto& p = *impl_;
    p.fb = open("/dev/fb0", O_RDWR | O_CLOEXEC);
    if (p.fb < 0 || ioctl(p.fb, FBIOGET_VSCREENINFO, &p.v) || ioctl(p.fb, FBIOGET_FSCREENINFO, &p.f))
        throw std::runtime_error("Cannot read framebuffer geometry");
    const auto valid_channel = [](fb_bitfield c, unsigned bits) { return !c.msb_right && c.length <= 8 && c.offset + c.length <= bits; };
    if (p.f.type != FB_TYPE_PACKED_PIXELS || p.f.visual != FB_VISUAL_TRUECOLOR || p.v.nonstd || p.v.grayscale ||
        (p.v.bits_per_pixel != 16 && p.v.bits_per_pixel != 32) || !p.v.xres || !p.v.yres || p.v.xres > 2048 || p.v.yres > 2048 ||
        !valid_channel(p.v.red, p.v.bits_per_pixel) || !valid_channel(p.v.green, p.v.bits_per_pixel) || !valid_channel(p.v.blue, p.v.bits_per_pixel) || !valid_channel(p.v.transp, p.v.bits_per_pixel) ||
        !p.v.red.length || !p.v.green.length || !p.v.blue.length)
        throw std::runtime_error("Unsupported framebuffer format");
    const auto bytes = p.v.bits_per_pixel / 8;
    const auto view_offset = std::uint64_t(p.v.yoffset) * p.f.line_length + std::uint64_t(p.v.xoffset) * bytes;
    const auto view_size = std::uint64_t(p.v.yres - 1) * p.f.line_length + std::uint64_t(p.v.xres) * bytes;
    if (view_offset > p.f.smem_len || view_size > p.f.smem_len - view_offset) throw std::runtime_error("Framebuffer offsets exceed memory");
    p.offset = std::size_t(view_offset); p.size = std::size_t(view_size);
    if (p.f.smem_len == 0 || p.f.smem_len > 32 * 1024 * 1024 || p.offset > p.f.smem_len || p.size > p.f.smem_len - p.offset ||
        (std::uint64_t(p.v.xoffset) + p.v.xres) * bytes > p.f.line_length)
        throw std::runtime_error("Framebuffer view exceeds mapped memory");
    void* map = mmap(nullptr, p.f.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, p.fb, 0);
    if (map == MAP_FAILED) throw std::runtime_error("Cannot map framebuffer");
    // Allocate before publishing mapping, so an allocation failure cannot restore
    // an incomplete backup into the display.
    try { p.original.assign(static_cast<std::uint8_t*>(map) + p.offset, static_cast<std::uint8_t*>(map) + p.offset + p.size); }
    catch (...) { munmap(map, p.f.smem_len); throw; }
    p.mapping = static_cast<std::uint8_t*>(map);
    p.input = open("/dev/input/event0", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (p.input < 0) throw std::runtime_error("Cannot open physical keys");
    p.reconcile();
    std::printf("display=%ux%u bpp=%u pitch=%u rotate=180 input=event0\n", p.v.xres, p.v.yres, p.v.bits_per_pixel, p.f.line_length);
    std::printf("smooth_filter_available=%s\n",frame_filter_detail::available()?"neon":"scalar");
    std::printf("framebuffer_channels=r%u:%u,g%u:%u,b%u:%u,a%u:%u\n",p.v.red.offset,p.v.red.length,p.v.green.offset,p.v.green.length,p.v.blue.offset,p.v.blue.length,p.v.transp.offset,p.v.transp.length);
}
Platform::~Platform() = default;

void Platform::poll() {
#if defined(__arm__) && !defined(__aarch64__)
    // Miyoo's old ARM32 kernel uses two 32-bit time fields. musl time_t is 64-bit.
    struct Event { std::uint32_t sec, usec; std::uint16_t type, code; std::int32_t value; };
    static_assert(sizeof(Event) == 16);
#else
    using Event = input_event;
#endif
    Event event{};
    for (;;) {
        const auto bytes = read(impl_->input, &event, sizeof(event));
        if (bytes < 0 && errno == EINTR) continue;
        if (bytes != sizeof(event)) break;
        if (event.type == EV_SYN && event.code == SYN_DROPPED) { impl_->resync = true; continue; }
        if (impl_->resync) {
            if (event.type == EV_SYN && event.code == SYN_REPORT) { impl_->reconcile(); impl_->resync = false; }
            continue;
        }
        if (event.type == EV_KEY && event.code <= KEY_MAX && event.value >= 0 && event.value <= 2)
            impl_->keys[event.code] = event.value != 0;
    }
}
bool Platform::held(unsigned key) const { return key <= KEY_MAX && impl_->keys[key]; }
bool Platform::exiting() const { return held(KEY_ESC) && held(KEY_RIGHTCTRL); }

void Platform::present(std::span<const std::uint8_t> rgba, unsigned width, unsigned height) {
    if (!width || !height || rgba.size() != std::uint64_t(width) * height * 4) throw std::runtime_error("Invalid source framebuffer");
    auto& p = *impl_;
    const auto channel = [](fb_bitfield c) { return Channel{c.offset, c.length}; };
    convert_frame(rgba, width, height, {p.mapping + p.offset, p.size},
        {p.v.xres, p.v.yres, p.f.line_length, p.v.bits_per_pixel,
         channel(p.v.red), channel(p.v.green), channel(p.v.blue), channel(p.v.transp)},true,g_smooth_frame);
    if (++p.frames % 125 == 0) {
        rusage usage{}; getrusage(RUSAGE_SELF, &usage);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - p.started).count();
        std::printf("miyoo frames=%llu elapsed_ms=%lld peak_rss_kib=%ld\n", (unsigned long long)p.frames, (long long)ms, usage.ru_maxrss);
        std::ifstream status("/proc/self/status"); std::string line;
        while (std::getline(status, line))
            if (line.starts_with("VmPeak:") || line.starts_with("VmHWM:") || line.starts_with("VmRSS:") || line.starts_with("VmSize:")) std::puts(line.c_str());
        std::fflush(stdout);
    }

}
}
