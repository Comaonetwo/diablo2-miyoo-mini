// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <memory>
#include <span>
namespace d2mini {
class AudioSink {
public:
 AudioSink(); ~AudioSink();
 AudioSink(const AudioSink&)=delete;AudioSink&operator=(const AudioSink&)=delete;
 struct Status{unsigned queued=0,underruns=0;};
 Status status()const;
 bool device_open()const;
 std::size_t samples_needed(std::size_t elapsed_samples) const;
 bool open(); void write(std::span<const std::int16_t> samples);
private: struct Impl;std::unique_ptr<Impl> impl_;
};
}
