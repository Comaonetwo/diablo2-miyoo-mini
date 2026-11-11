// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace d2mini {
// Device access is opt-in: constructing this object is the only place that
// opens the console framebuffer and physical keys. No SD/OS configuration writes.
class Platform {
public:
    Platform();
    ~Platform();
    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;
    void poll();
    bool held(unsigned key) const;
    bool exiting() const;
    void present(std::span<const std::uint8_t> rgba, unsigned width, unsigned height);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
Platform* current();
void enable();
void memory_checkpoint(const char* phase);
}
