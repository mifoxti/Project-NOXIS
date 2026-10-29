#pragma once
#include "state/Parameters.h"
#include "synth/Effects.h"
#include "synth/Modulation.h"
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
    float current() const noexcept {return value;}
private:
    float value=0, target=0, step=0; int remaining=0;
};
class Engine {
public:
    static constexpr int polyphony=8;
    void prepare(double rate);
    void setParameters(const Values& state, bool immediate=false) noexcept;
    void setRouting(const CompiledRouting&,bool immediate=false) noexcept;
    void noteOn(int note, float velocity, int channel=1) noexcept;
    void noteOff(int note, int channel=1) noexcept;
    void sustain(bool down, int channel=1) noexcept;
    void pitchBend(float normalised,int channel=1) noexcept;
    void allNotesOff() noexcept;
    float tick() noexcept;
    StereoFrame tickStereo() noexcept;
    int activeVoices() const noexcept;
    double currentFrequency(int slot=0) const noexcept;
    std::array<float,2> modulationValues() const noexcept {return lfoValues;}
    float envelopeLevel() const noexcept {float maximum=0;for(float level:envelopeLevels)maximum=std::max(maximum,level);return maximum;}
private:
    struct Voice {
        std::array<Oscillator,8> a,b,oldA,oldB;
        Oscillator sub;
        Envelope env;
        std::array<LowPass,2> filter;
        int note=-1,channel=1, waveA=0,waveB=0,oldWaveA=0,oldWaveB=0, fadeA=0,fadeB=0;
        bool held=false, panic=false;
        float velocity=0,currentMidi=0,targetMidi=0,glideStep=0;
        double frequency=440,frequencyStep=0;
        StereoFrame last{},tail{};
        float filterG=0,filterStep=0;
        std::uint32_t noiseState=0x4a937b21u;
        int glideSamples=0;
        int tailSamples=0;
        std::uint64_t age=0;
    };
    void startVoice(Voice&,int,float,int,bool preserve=false) noexcept;
    void updateMonoTarget() noexcept;
    void configureGlide(Voice&,float,bool) noexcept;
    using VoiceFrames=std::array<StereoFrame,polyphony>;
    StereoFrame processGraph(const VoiceFrames&,const VoiceFrames&,const Values&) noexcept;
    std::array<Voice,polyphony> voices{};
    std::array<bool,16> pedals{};
    std::array<Ramp,16> bends{};
    std::array<float,16> previousMidi{};
    std::array<bool,16> previousValid{};
    std::array<bool,16*128> pressed{},sustained{};
    std::array<float,16*128> heldVelocity{};
    std::array<std::uint64_t,16*128> heldAge{};
    std::array<Ramp,parameterInfo.size()> ramps{};
    std::array<Ramp,8> laneWeight{},lanePosition{};
    std::array<float,8> weights{},positions{};
    std::array<double,8> ratioA{},ratioB{};
    std::array<float,8> panLeft{},panRight{};
    std::array<float,polyphony> envelopeLevels{};
    std::array<Lfo,2> lfos{};
    std::array<float,2> lfoValues{};
    StereoChorus chorus;
    StereoDelay delay;
    StereoReverb reverb;
    CompiledRouting routing{},pendingRouting{};
    int routingFade=0,routingFadeLength=240,controlCounter=0;
    Ramp panicGain;
    int clearFxAfter=0;bool resumeAfterPanic=false;
    bool hasRouting=false;
    std::uint32_t randomState=0x6a09e667u;
    Values parameters=defaults();
    double sampleRate=48000;
    std::uint64_t age=0;
};
}
