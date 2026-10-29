#pragma once
#include "state/Parameters.h"
#include <array>
#include <cstdint>

namespace noxis {
inline constexpr double pi = 3.14159265358979323846;
double midiFrequency(int note) noexcept;
class Oscillator {
public:
    float tick(double frequency, double sampleRate, int waveform) noexcept;
    void reset(double position=0) noexcept { phase=position; }
private:
    double phase=0;
};
class Envelope {
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };
    void on() noexcept { stage=Stage::Attack; }
    void off() noexcept { if (stage!=Stage::Idle && stage!=Stage::Release) { stage=Stage::Release; releaseStart=value; } }
    void reset() noexcept { stage=Stage::Idle; value=0; }
    float tick(double sampleRate, float a, float d, float s, float r) noexcept;
    bool active() const noexcept { return stage!=Stage::Idle; }
    bool releasing() const noexcept { return stage==Stage::Release; }
    float level() const noexcept { return value; }
private:
    Stage stage=Stage::Idle;
    float value=0, releaseStart=0;
};
class LowPass {
public:
    float tick(float input, float coefficient, float damping) noexcept;
    void reset() noexcept { ic1=ic2=0; }
private:
    float ic1=0, ic2=0;
};
class Ramp {
public:
    void reset(float v) noexcept { value=target=v; remaining=0; }
    void to(float v, int samples) noexcept { if (v!=target) { target=v; remaining=samples; step=(v-value)/static_cast<float>(samples); } }
    float next() noexcept { if (remaining>0) { value+=step; if (--remaining==0) value=target; } return value; }
private:
    float value=0, target=0, step=0; int remaining=0;
};
class Engine {
public:
    static constexpr int polyphony=8;
    void prepare(double rate) noexcept;
    void setParameters(const Values& state, bool immediate=false) noexcept;
    void noteOn(int note, float velocity, int channel=1) noexcept;
    void noteOff(int note, int channel=1) noexcept;
    void sustain(bool down, int channel=1) noexcept;
    void allNotesOff() noexcept;
    float tick() noexcept;
    int activeVoices() const noexcept;
private:
    struct Voice {
        Oscillator a,b,oldA,oldB;
        Envelope env;
        LowPass filter;
        int note=-1,channel=1, waveA=0,waveB=0,oldWaveA=0,oldWaveB=0, fadeA=0,fadeB=0;
        bool held=false, panic=false;
        float velocity=0,last=0,tail=0;
        int tailSamples=0;
        std::uint64_t age=0;
    };
    std::array<Voice,polyphony> voices{};
    std::array<bool,16> pedals{};
    std::array<Ramp,parameterInfo.size()> ramps{};
    Values parameters=defaults();
    double sampleRate=48000;
    std::uint64_t age=0;
};
}
