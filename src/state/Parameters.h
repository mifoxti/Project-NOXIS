#pragma once
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace noxis {
enum class Param : std::size_t { WaveA, OctaveA, DetuneA, LevelA, WaveB, OctaveB, DetuneB, LevelB,
    Attack, Decay, Sustain, Release, Cutoff, Resonance, Drive, Output, Master, Count };
struct ParamInfo { std::string_view id, label, unit; float min, max, initial; bool integral = false; };
inline constexpr std::array<ParamInfo, static_cast<std::size_t>(Param::Count)> parameterInfo {{
    {"waveA", "Wave", "", 0, 3, 1, true}, {"octaveA", "Octave", "oct", -3, 3, 0, true},
    {"detuneA", "Detune", "ct", -50, 50, 0}, {"levelA", "Level", "%", 0, 1, .7f},
    {"waveB", "Wave", "", 0, 3, 0, true}, {"octaveB", "Octave", "oct", -3, 3, 0, true},
    {"detuneB", "Detune", "ct", -50, 50, 0}, {"levelB", "Level", "%", 0, 1, .25f},
    {"attack", "Attack", "s", .002f, 4, .012f}, {"decay", "Decay", "s", .01f, 4, .25f},
    {"sustain", "Sustain", "%", 0, 1, .7f}, {"release", "Release", "s", .005f, 6, .3f},
    {"cutoff", "Cutoff", "Hz", 25, 18000, 7000}, {"resonance", "Resonance", "%", 0, 1, .12f},
    {"drive", "Drive", "dB", 0, 24, 0}, {"output", "Output", "dB", -24, 6, -3},
    {"master", "Master", "dB", -60, 0, -9}
}};
inline constexpr std::size_t index(Param p) { return static_cast<std::size_t>(p); }
using Values = std::array<float, parameterInfo.size()>;
inline float sanitise(Param p, float v) noexcept {
    const auto& d = parameterInfo[index(p)];
    v = std::isfinite(v) ? std::clamp(v, d.min, d.max) : d.initial;
    return d.integral ? std::round(v) : v;
}
inline Values defaults() { Values v{}; for (std::size_t i=0; i<v.size(); ++i) v[i]=parameterInfo[i].initial; return v; }
// One GUI writer; the audio reader tries once and retains its last complete snapshot.
class ParameterStore {
public:
    ParameterStore() { for (std::size_t i=0; i<values.size(); ++i) values[i].store(parameterInfo[i].initial); }
    float get(Param p) const noexcept { return values[index(p)].load(std::memory_order_relaxed); }
    void set(Param p, float v) noexcept { auto s = snapshot(); s[index(p)] = sanitise(p,v); replace(s); }
    void replace(const Values& v) noexcept {
        revision.fetch_add(1, std::memory_order_seq_cst);
        for (std::size_t i=0; i<v.size(); ++i) values[i].store(sanitise(static_cast<Param>(i),v[i]), std::memory_order_seq_cst);
        revision.fetch_add(1, std::memory_order_seq_cst);
    }
    bool read(Values& destination) const noexcept {
        const auto before=revision.load(std::memory_order_seq_cst);
        if ((before & 1u)!=0) return false;
        Values copy{};
        for (std::size_t i=0; i<copy.size(); ++i) copy[i]=values[i].load(std::memory_order_seq_cst);
        if (before != revision.load(std::memory_order_seq_cst)) return false;
        destination=copy; return true;
    }
    Values snapshot() const noexcept { auto v=defaults(); read(v); return v; }
private:
    std::array<std::atomic<float>, parameterInfo.size()> values{};
    std::atomic<std::uint32_t> revision{0};
};
static_assert(std::atomic<float>::is_always_lock_free);
static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
}
