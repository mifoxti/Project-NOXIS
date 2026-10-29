#pragma once
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include "state/Routing.h"

namespace noxis {
enum class Param : std::size_t { WaveA, OctaveA, DetuneA, LevelA, WaveB, OctaveB, DetuneB, LevelB,
    Attack, Decay, Sustain, Release, Cutoff, Resonance, Drive, Output, Master,
    UnisonVoices,UnisonDetune,Spread,Width,PhaseRandom,Glide,Mono,Legato,BendRange,
    Lfo1Wave,Lfo1Rate,Lfo1Depth,Lfo1Phase,Lfo1Sync,Lfo1Retrigger,
    Lfo2Wave,Lfo2Rate,Lfo2Depth,Lfo2Phase,Lfo2Sync,Lfo2Retrigger,Bpm,
    Lfo1Pitch,Lfo1Cutoff,Lfo1Level,Lfo1Width,Lfo2Pitch,Lfo2Cutoff,Lfo2Level,Lfo2Width,EnvCutoff,
    SaturationEnabled,SaturationMix,ChorusEnabled,ChorusRate,ChorusDepth,ChorusMix,
    DelayEnabled,DelayTime,DelayFeedback,DelayMix,DelayStereo,ReverbEnabled,ReverbSize,ReverbDamping,ReverbMix,
    SubLevel,NoiseLevel,SemitoneB,Count };
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
    {"master", "Master", "dB", -60, 0, -9},
    {"unisonVoices","Voices","",1,8,1,true},{"unisonDetune","Detune","ct",0,50,10},
    {"spread","Spread","%",0,1,.8f},{"width","Width","%",0,1,1},{"phaseRandom","Phase random","%",0,1,0},
    {"glide","Glide","s",0,2,0},{"mono","Mono","",0,1,0,true},{"legato","Legato","",0,1,0,true},{"bendRange","Bend range","st",1,24,2,true},
    {"lfo1Wave","Wave","",0,4,0,true},{"lfo1Rate","Rate","Hz",.02f,20,1},{"lfo1Depth","Depth","%",0,1,1},{"lfo1Phase","Phase","%",0,1,0},{"lfo1Sync","Sync","",0,1,0,true},{"lfo1Retrigger","Retrigger","",0,1,1,true},
    {"lfo2Wave","Wave","",0,4,1,true},{"lfo2Rate","Rate","Hz",.02f,20,.3f},{"lfo2Depth","Depth","%",0,1,1},{"lfo2Phase","Phase","%",0,1,0},{"lfo2Sync","Sync","",0,1,0,true},{"lfo2Retrigger","Retrigger","",0,1,1,true},
    {"bpm","Tempo","BPM",40,240,120},
    {"lfo1Pitch","Pitch","st",-12,12,0},{"lfo1Cutoff","Cutoff","oct",-5,5,0},{"lfo1Level","Level","%",-1,1,0},{"lfo1Width","Width","%",-1,1,0},
    {"lfo2Pitch","Pitch","st",-12,12,0},{"lfo2Cutoff","Cutoff","oct",-5,5,0},{"lfo2Level","Level","%",-1,1,0},{"lfo2Width","Width","%",-1,1,0},{"envCutoff","Env > cutoff","oct",-5,5,0},
    {"saturationEnabled","Enabled","",0,1,1,true},{"saturationMix","Mix","%",0,1,1},
    {"chorusEnabled","Enabled","",0,1,0,true},{"chorusRate","Rate","Hz",.05f,5,.4f},{"chorusDepth","Depth","%",0,1,.4f},{"chorusMix","Mix","%",0,1,.25f},
    {"delayEnabled","Enabled","",0,1,0,true},{"delayTime","Time","s",.02f,1.5f,.375f},{"delayFeedback","Feedback","%",0,.85f,.35f},{"delayMix","Mix","%",0,1,.25f},{"delayStereo","Ping-pong","",0,1,1,true},
    {"reverbEnabled","Enabled","",0,1,0,true},{"reverbSize","Size","%",0,1,.6f},{"reverbDamping","Damping","%",0,1,.5f},{"reverbMix","Mix","%",0,1,.25f},
    {"subLevel","Sub","%",0,1,0},{"noiseLevel","Noise","%",0,1,0},{"semitoneB","Tune B","st",-24,24,0}
}};
inline constexpr std::size_t index(Param p) { return static_cast<std::size_t>(p); }
using Values = std::array<float, parameterInfo.size()>;
struct RealtimeState {Values values{};CompiledRouting routing{};};
inline float sanitise(Param p, float v) noexcept {
    const auto& d = parameterInfo[index(p)];
    v = std::isfinite(v) ? std::clamp(v, d.min, d.max) : d.initial;
    return d.integral ? std::round(v) : v;
}
inline Values defaults() { Values v{}; for (std::size_t i=0; i<v.size(); ++i) v[i]=parameterInfo[i].initial; return v; }
// One GUI writer; the audio reader tries once and retains its last complete snapshot.
class ParameterStore {
public:
    ParameterStore() {std::string error;compileRouting(guiGraph,guiPlan,error);replace(defaults());}
    float get(Param p) const noexcept { return values[index(p)].load(std::memory_order_relaxed); }
    void set(Param p, float v) noexcept { auto s = snapshot(); s[index(p)] = sanitise(p,v); replace(s); }
    void replace(const Values& v) noexcept {
        revision.fetch_add(1, std::memory_order_seq_cst);
        for (std::size_t i=0; i<v.size(); ++i) values[i].store(sanitise(static_cast<Param>(i),v[i]), std::memory_order_seq_cst);
        publishRouting();
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
    const RoutingGraph& graph() const noexcept {return guiGraph;} // GUI writer only.
    bool replacePatch(const Values& v,const RoutingGraph& g,std::string& error) {
        CompiledRouting plan;if(!compileRouting(g,plan,error))return false;guiGraph=g;guiPlan=plan;replace(v);return true;
    }
    bool setGraph(const RoutingGraph& g,std::string& error){return replacePatch(snapshot(),g,error);}
    bool read(RealtimeState& destination) const noexcept {
        const auto before=revision.load(std::memory_order_seq_cst);if(before&1u)return false;RealtimeState copy;
        for(std::size_t i=0;i<values.size();++i)copy.values[i]=values[i].load(std::memory_order_seq_cst);
        for(int i=0;i<audioNodeCount;++i){copy.routing.order[i]=order[i].load(std::memory_order_seq_cst);for(int j=0;j<2;++j)copy.routing.inputs[i][j]=inputs[i*2+j].load(std::memory_order_seq_cst);}
        for(int i=0;i<3;++i)copy.routing.modulationMask[i]=modulation[i].load(std::memory_order_seq_cst);
        if(before!=revision.load(std::memory_order_seq_cst))return false;destination=copy;return true;
    }
private:
    void publishRouting() noexcept {
        for(int i=0;i<audioNodeCount;++i){order[i].store(guiPlan.order[i],std::memory_order_seq_cst);for(int j=0;j<2;++j)inputs[i*2+j].store(guiPlan.inputs[i][j],std::memory_order_seq_cst);}
        for(int i=0;i<3;++i)modulation[i].store(guiPlan.modulationMask[i],std::memory_order_seq_cst);
    }
    RoutingGraph guiGraph=defaultRouting();CompiledRouting guiPlan{};
    std::array<std::atomic<int>,audioNodeCount> order{};
    std::array<std::atomic<int>,audioNodeCount*2> inputs{};
    std::array<std::atomic<int>,3> modulation{};
    std::array<std::atomic<float>, parameterInfo.size()> values{};
    std::atomic<std::uint32_t> revision{0};
};
static_assert(std::atomic<float>::is_always_lock_free);
static_assert(std::atomic<int>::is_always_lock_free);
static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
}
