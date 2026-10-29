#include "synth/Engine.h"
#include <algorithm>
#include <cmath>

namespace noxis {
double midiFrequency(int note) noexcept { return 440.0 * std::exp2((note-69)/12.0); }
static double blep(double phase, double dt) noexcept {
    if (phase<dt) { const auto t=phase/dt; return t+t-t*t-1; }
    if (phase>1-dt) { const auto t=(phase-1)/dt; return t*t+t+t+1; }
    return 0;
}
float Oscillator::tick(double hz, double rate, int waveform) noexcept {
    const double dt=std::clamp(hz/rate, .0000001, .45);
    double out=0;
    switch(waveform) {
        case 1: out=2*phase-1-blep(phase,dt); break;
        case 2: out=(phase<.5 ? 1.0 : -1.0)+blep(phase,dt)-blep(std::fmod(phase+.5,1.0),dt); break;
        case 3: {
            // A finite odd-harmonic series avoids the triangle's high-frequency aliasing.
            const int limit=std::min(31,static_cast<int>(.48/dt));
            for(int k=1;k<=limit;k+=2) out+=((k%4==1)?1.0:-1.0)*std::sin(2*pi*phase*k)/(k*k);
            out*=8/(pi*pi); break;
        }
        default: out=std::sin(2*pi*phase); break;
    }
    phase+=dt; if(phase>=1) phase-=1;
    return static_cast<float>(out);
}
float Envelope::tick(double rate,float a,float d,float s,float r) noexcept {
    switch(stage) {
        case Stage::Attack: value+=static_cast<float>(1/(rate*std::max(.002f,a))); if(value>=1) { value=1; stage=Stage::Decay; } break;
        case Stage::Decay: {
            const float step=static_cast<float>((1-s)/(rate*std::max(.01f,d)));
            if(value>s) value=std::max(s,value-step); else value=std::min(s,value+static_cast<float>(1/(rate*.02)));
            if(std::abs(value-s)<.00001f) stage=Stage::Sustain; break;
        }
        case Stage::Sustain: value+=std::clamp(s-value,static_cast<float>(-1/(rate*.02)),static_cast<float>(1/(rate*.02))); break;
        case Stage::Release: value-=static_cast<float>(releaseStart/(rate*std::max(.005f,r))); if(value<=.000001f) reset(); break;
        case Stage::Idle: break;
    }
    return value;
}
float LowPass::tick(float x,float g,float k) noexcept {
    const float a1=1/(1+g*(g+k)), v1=a1*(ic1+g*(x-ic2)), v2=ic2+g*v1;
    ic1=2*v1-ic1; ic2=2*v2-ic2;
    return v2;
}
void Engine::prepare(double rate) noexcept {
    sampleRate=std::isfinite(rate) && rate>=8000 ? rate : 48000;
    voices={}; pedals={}; age=0; setParameters(parameters,true);
}
void Engine::setParameters(const Values& v,bool immediate) noexcept {
    for(std::size_t i=0;i<v.size();++i) {
        parameters[i]=sanitise(static_cast<Param>(i),v[i]);
        if(immediate) ramps[i].reset(parameters[i]); else ramps[i].to(parameters[i],std::max(1,static_cast<int>(sampleRate*.02)));
    }
}
void Engine::noteOn(int note,float velocity,int channel) noexcept {
    if(note<0 || note>127 || channel<1 || channel>16) return;
    if(velocity<=0) { noteOff(note,channel); return; }
    Voice* selected=nullptr;
    for(auto& v:voices) if(!v.env.active()) { selected=&v; break; }
    if(!selected) {
        selected=&*std::min_element(voices.begin(),voices.end(),[](const Voice& a,const Voice& b) {
            if(a.env.releasing()!=b.env.releasing()) return a.env.releasing();
            return a.age<b.age;
        });
    }
    const float tail=selected->last;
    *selected=Voice{}; selected->tail=tail; selected->tailSamples=static_cast<int>(sampleRate*.003);
    selected->note=note; selected->channel=channel; selected->velocity=std::clamp(velocity,0.0f,1.0f); selected->held=true;
    selected->age=++age; selected->waveA=static_cast<int>(parameters[index(Param::WaveA)]);
    selected->waveB=static_cast<int>(parameters[index(Param::WaveB)]);
    selected->b.reset(.19); selected->env.on();
}
void Engine::noteOff(int note,int channel) noexcept {
    for(auto& v:voices) if(v.note==note && v.channel==channel && v.held) { v.held=false; if(!pedals[static_cast<std::size_t>(channel-1)]) v.env.off(); }
}
void Engine::sustain(bool down,int channel) noexcept {
    if(channel<1 || channel>16) return;
    pedals[static_cast<std::size_t>(channel-1)]=down;
    if(!down) for(auto& v:voices) if(v.channel==channel && !v.held) v.env.off();
}
void Engine::allNotesOff() noexcept { pedals={}; for(auto& v:voices) { v.held=false; v.panic=true; v.env.off(); } }
int Engine::activeVoices() const noexcept { int count=0; for(const auto& v:voices) if(v.env.active()) ++count; return count; }
float Engine::tick() noexcept {
    Values p{}; for(std::size_t i=0;i<p.size();++i) p[i]=ramps[i].next();
    const auto get=[&](Param id){return p[index(id)];};
    const float g=static_cast<float>(std::tan(pi*std::min(static_cast<double>(get(Param::Cutoff)),sampleRate*.45)/sampleRate));
    const float damping=2-1.85f*get(Param::Resonance);
    const double ratioA=std::exp2(get(Param::OctaveA)+get(Param::DetuneA)/1200.0);
    const double ratioB=std::exp2(get(Param::OctaveB)+get(Param::DetuneB)/1200.0);
    float sum=0;
    for(auto& v:voices) {
        float sample=0;
        if(v.env.active()) {
            const double hz=midiFrequency(v.note);
            const int waveA=static_cast<int>(parameters[index(Param::WaveA)]),waveB=static_cast<int>(parameters[index(Param::WaveB)]);
            if(waveA!=v.waveA) { v.oldA=v.a; v.oldWaveA=v.waveA; v.fadeA=static_cast<int>(sampleRate*.005); v.waveA=waveA; }
            if(waveB!=v.waveB) { v.oldB=v.b; v.oldWaveB=v.waveB; v.fadeB=static_cast<int>(sampleRate*.005); v.waveB=waveB; }
            float a=v.a.tick(hz*ratioA,sampleRate,v.waveA), b=v.b.tick(hz*ratioB,sampleRate,v.waveB);
            if(v.fadeA>0) { const float mix=static_cast<float>(v.fadeA--)/static_cast<float>(sampleRate*.005); a=a*(1-mix)+v.oldA.tick(hz*ratioA,sampleRate,v.oldWaveA)*mix; }
            if(v.fadeB>0) { const float mix=static_cast<float>(v.fadeB--)/static_cast<float>(sampleRate*.005); b=b*(1-mix)+v.oldB.tick(hz*ratioB,sampleRate,v.oldWaveB)*mix; }
            const float env=v.env.tick(sampleRate,get(Param::Attack),get(Param::Decay),get(Param::Sustain),v.panic?.005f:get(Param::Release));
            sample=v.filter.tick(a*get(Param::LevelA)+b*get(Param::LevelB),g,damping)*env*v.velocity*.16f;
        }
        if(v.tailSamples>0) { sample+=v.tail*static_cast<float>(v.tailSamples--)/static_cast<float>(sampleRate*.003); }
        v.last=sample; sum+=sample;
    }
    const float drive=std::pow(10.0f,get(Param::Drive)/20), output=std::pow(10.0f,(get(Param::Output)+get(Param::Master))/20);
    const float saturated=std::tanh(sum*drive)/std::sqrt(drive);
    const float out=.96f*std::tanh(saturated*output);
    return std::isfinite(out)?out:0;
}
}
