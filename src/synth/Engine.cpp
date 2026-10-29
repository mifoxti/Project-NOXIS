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
            // Рекурсия нечётных гармоник сохраняет полосу без 16 вызовов sin на осциллятор.
            const int limit=std::min(31,static_cast<int>(.48/dt));
            const double fundamental=std::sin(2*pi*phase),twiceCosine=2*(1-2*fundamental*fundamental);
            double previous=-fundamental,current=fundamental;
            for(int k=1;k<=limit;k+=2) {out+=((k%4==1)?1.0:-1.0)*current/(k*k);const double next=twiceCosine*current-previous;previous=current;current=next;}
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
void Engine::prepare(double rate) {
    sampleRate=std::isfinite(rate)?std::clamp(rate,8000.0,192000.0):48000;
    voices={};pedals={};pressed={};sustained={};heldAge={};previousValid={};envelopeLevels={};age=0;
    randomState=0x6a09e667u;lfos={};lfoValues={};controlCounter=0;
    for(auto& bend:bends)bend.reset(0);
    chorus.prepare(sampleRate);delay.prepare(sampleRate);reverb.prepare(sampleRate);
    routingFadeLength=std::max(1,static_cast<int>(sampleRate*.005));
    if(!hasRouting){std::string error;compileRouting(defaultRouting(),routing,error);pendingRouting=routing;hasRouting=true;}
    routingFade=0;panicGain.reset(1);clearFxAfter=0;resumeAfterPanic=false;setParameters(parameters,true);
}
void Engine::setParameters(const Values& values,bool immediate) noexcept {
    const bool modeChanged=parameters[index(Param::Mono)]!=sanitise(Param::Mono,values[index(Param::Mono)]);
    bool changed=false;
    const int smoothing=std::max(1,static_cast<int>(sampleRate*.02));
    for(std::size_t i=0;i<values.size();++i){const float value=sanitise(static_cast<Param>(i),values[i]);changed=changed||value!=parameters[i];parameters[i]=value;if(immediate)ramps[i].reset(value);else ramps[i].to(value,smoothing);}
    const int count=static_cast<int>(parameters[index(Param::UnisonVoices)]);
    for(int i=0;i<8;++i){const float weight=i<count?1.0f:0.0f,position=count>1&&i<count?-1.0f+2.0f*static_cast<float>(i)/static_cast<float>(count-1):0;
        if(immediate){laneWeight[i].reset(weight);lanePosition[i].reset(position);}else{laneWeight[i].to(weight,smoothing);lanePosition[i].to(position,smoothing);}}
    if(changed||immediate)controlCounter=0;
    if(modeChanged)allNotesOff();
}
void Engine::setRouting(const CompiledRouting& plan,bool immediate) noexcept {
    if(immediate||!hasRouting){routing=pendingRouting=plan;routingFade=0;hasRouting=true;controlCounter=0;return;}
    if(plan==pendingRouting)return;
    pendingRouting=plan;routingFade=routingFadeLength*2;
}
void Engine::configureGlide(Voice& voice,float target,bool fromPrevious) noexcept {
    const float time=parameters[index(Param::Glide)];
    voice.targetMidi=target;
    if(fromPrevious&&time>.00001f){voice.glideSamples=std::max(1,static_cast<int>(time*sampleRate));voice.glideStep=(target-voice.currentMidi)/static_cast<float>(voice.glideSamples);}
    else {voice.currentMidi=target;voice.glideSamples=0;voice.glideStep=0;}
}
void Engine::startVoice(Voice& voice,int note,float velocity,int channel,bool preserve) noexcept {
    const bool wasActive=voice.env.active();
    if(!preserve){const auto tail=voice.last;voice=Voice{};voice.tail=tail;voice.tailSamples=static_cast<int>(sampleRate*.003);voice.currentMidi=previousMidi[channel-1];
        voice.waveA=static_cast<int>(parameters[index(Param::WaveA)]);voice.waveB=static_cast<int>(parameters[index(Param::WaveB)]);
        const float randomAmount=parameters[index(Param::PhaseRandom)];
        for(int i=0;i<8;++i){voice.a[i].reset((randomBipolar(randomState)+1)*.5*randomAmount);voice.b[i].reset(std::fmod(.19+(randomBipolar(randomState)+1)*.5*randomAmount,1.0));}
        voice.noiseState=randomState?randomState:1;voice.env.on();
    }
    voice.note=note;voice.channel=channel;voice.velocity=std::isfinite(velocity)?std::clamp(velocity,0.0f,1.0f):0;voice.held=true;voice.panic=false;voice.age=++age;
    configureGlide(voice,static_cast<float>(note),preserve||previousValid[channel-1]);
    if(!wasActive&&!preserve)voice.frequency=midiFrequency(static_cast<int>(std::round(voice.currentMidi)));
    previousMidi[channel-1]=static_cast<float>(note);previousValid[channel-1]=true;controlCounter=0;
}
void Engine::noteOn(int note,float velocity,int channel) noexcept {
    if(note<0||note>127||channel<1||channel>16)return;
    if(!std::isfinite(velocity)||velocity<=0){noteOff(note,channel);return;}
    if(clearFxAfter>0)resumeAfterPanic=true;else panicGain.to(1,std::max(1,static_cast<int>(sampleRate*.005)));
    const auto key=static_cast<std::size_t>((channel-1)*128+note);pressed[key]=true;sustained[key]=false;heldVelocity[key]=velocity;heldAge[key]=++age;
    for(int i=0;i<2;++i){const auto p=i==0?Param::Lfo1Retrigger:Param::Lfo2Retrigger;if(parameters[index(p)]>.5f)lfos[i].reset();}
    if(parameters[index(Param::Mono)]>.5f){const bool preserve=voices[0].env.active()&&parameters[index(Param::Legato)]>.5f;startVoice(voices[0],note,velocity,channel,preserve);return;}
    Voice* selected=nullptr;
    for(auto& voice:voices)if(voice.note==note&&voice.channel==channel&&voice.held){selected=&voice;break;}
    if(!selected)for(auto& voice:voices)if(!voice.env.active()){selected=&voice;break;}
    if(!selected)selected=&*std::min_element(voices.begin(),voices.end(),[](const Voice& a,const Voice& b){if(a.env.releasing()!=b.env.releasing())return a.env.releasing();return a.age<b.age;});
    startVoice(*selected,note,velocity,channel);
}
void Engine::updateMonoTarget() noexcept {
    std::size_t candidate=pressed.size();std::uint64_t newest=0;
    for(std::size_t i=0;i<pressed.size();++i)if((pressed[i]||sustained[i])&&heldAge[i]>newest){candidate=i;newest=heldAge[i];}
    auto& voice=voices[0];
    if(candidate==pressed.size()){voice.held=false;voice.env.off();return;}
    const int note=static_cast<int>(candidate%128),channel=static_cast<int>(candidate/128)+1;
    if(voice.note==note&&voice.channel==channel&&voice.env.active())return;
    startVoice(voice,note,heldVelocity[candidate],channel,voice.env.active()&&parameters[index(Param::Legato)]>.5f);
}
void Engine::noteOff(int note,int channel) noexcept {
    if(note<0||note>127||channel<1||channel>16)return;
    const auto key=static_cast<std::size_t>((channel-1)*128+note);pressed[key]=false;sustained[key]=pedals[channel-1];
    if(parameters[index(Param::Mono)]>.5f){updateMonoTarget();return;}
    for(auto& voice:voices)if(voice.note==note&&voice.channel==channel&&voice.held){voice.held=false;if(!pedals[channel-1])voice.env.off();}
}
void Engine::sustain(bool down,int channel) noexcept {
    if(channel<1||channel>16)return;pedals[channel-1]=down;
    if(!down){for(int n=0;n<128;++n)sustained[static_cast<std::size_t>((channel-1)*128+n)]=false;
        if(parameters[index(Param::Mono)]>.5f)updateMonoTarget();
        else for(auto& voice:voices)if(voice.channel==channel&&!voice.held)voice.env.off();}
}
void Engine::pitchBend(float value,int channel) noexcept {if(channel<1||channel>16)return;bends[channel-1].to(std::isfinite(value)?std::clamp(value,-1.0f,1.0f):0,std::max(1,static_cast<int>(sampleRate*.002)));}
void Engine::allNotesOff() noexcept {pedals={};pressed={};sustained={};previousValid={};for(auto& voice:voices){voice.held=false;voice.panic=true;voice.env.off();}clearFxAfter=std::max(1,static_cast<int>(sampleRate*.005));panicGain.to(0,clearFxAfter);resumeAfterPanic=false;}
int Engine::activeVoices() const noexcept {int count=0;for(const auto& voice:voices)if(voice.env.active())++count;return count;}
double Engine::currentFrequency(int slot) const noexcept {if(slot<0||slot>=polyphony)return 0;const auto& voice=voices[slot];if(voice.note<0)return 0;return 440*std::exp2((voice.currentMidi-69+bends[voice.channel-1].current()*parameters[index(Param::BendRange)])/12.0);}
StereoFrame Engine::processGraph(const VoiceFrames& oscA,const VoiceFrames& oscB,const Values& p) noexcept {
    const auto get=[&](Param id){return p[index(id)];};
    std::array<VoiceFrames,audioNodeCount> buffers{};buffers[0]=oscA;buffers[1]=oscB;
    for(int node:routing.order){
        if(node<2)continue;
        auto& output=buffers[node];const int source=routing.inputs[node][0];if(source>=0)output=buffers[source];
        if(node==2){const int other=routing.inputs[node][1];if(other>=0)for(int v=0;v<polyphony;++v)output[v]+=buffers[other][v];
            float level=1;for(int i=0;i<2;++i)if(routing.modulationMask[i]&4)level+=lfoValues[i]*get(i==0?Param::Lfo1Level:Param::Lfo2Level);
            level=std::clamp(level,0.0f,2.0f);for(auto& voice:output)voice=voice*level;
        }else if(node==3){const float damping=2-1.85f*get(Param::Resonance);for(int v=0;v<polyphony;++v){output[v].left=voices[v].filter[0].tick(output[v].left,voices[v].filterG,damping);output[v].right=voices[v].filter[1].tick(output[v].right,voices[v].filterG,damping);}}
        else if(node>=4&&node<=7){StereoFrame dry{};for(auto voice:output)dry+=voice;StereoFrame processed=dry;float mix=0;
            if(node==4){const float drive=std::pow(10.0f,get(Param::Drive)/20),normal=1/std::sqrt(drive);mix=get(Param::SaturationEnabled)*get(Param::SaturationMix);processed=blend(dry,{std::tanh(dry.left*drive)*normal,std::tanh(dry.right*drive)*normal},mix);}
            else if(node==5){mix=get(Param::ChorusEnabled)*get(Param::ChorusMix);processed=chorus.process(dry,get(Param::ChorusRate),get(Param::ChorusDepth),mix);}
            else if(node==6){mix=get(Param::DelayEnabled)*get(Param::DelayMix);processed=delay.process(dry,get(Param::DelayTime),get(Param::DelayFeedback),mix,parameters[index(Param::DelayStereo)]>.5f);}
            else {mix=get(Param::ReverbEnabled)*get(Param::ReverbMix);processed=reverb.process(dry,get(Param::ReverbSize),get(Param::ReverbDamping),mix);}
            // Сухие голоса остаются раздельными; общий wet-поток проходит через первый вход.
            for(auto& voice:output)voice=voice*(1-mix);output[0]+=processed-dry*(1-mix);
        }
    }
    StereoFrame result{};for(int v=0;v<polyphony;++v){voices[v].last=buffers[8][v];result+=buffers[8][v];}return result;
}
StereoFrame Engine::tickStereo() noexcept {
    if(clearFxAfter>0&&--clearFxAfter==0){chorus.reset();delay.reset();reverb.reset();for(auto& voice:voices){for(auto& filter:voice.filter)filter.reset();voice.tailSamples=0;}if(resumeAfterPanic)panicGain.to(1,std::max(1,static_cast<int>(sampleRate*.005)));}
    Values p{};for(std::size_t i=0;i<p.size();++i)p[i]=ramps[i].next();
    const auto get=[&](Param id){return p[index(id)];};
    for(auto& bend:bends)bend.next();
    float normal=0;for(int i=0;i<8;++i){weights[i]=laneWeight[i].next();positions[i]=lanePosition[i].next();normal+=weights[i];}normal=1/std::max(1.0f,normal);
    for(int i=0;i<2;++i){const bool first=i==0;const auto rateId=first?Param::Lfo1Rate:Param::Lfo2Rate;double hz=get(rateId);
        if(parameters[index(first?Param::Lfo1Sync:Param::Lfo2Sync)]>.5f)hz=Lfo::synchronisedRate(hz,get(Param::Bpm));
        const float target=lfos[i].tick(hz,sampleRate,static_cast<int>(parameters[index(first?Param::Lfo1Wave:Param::Lfo2Wave)]),get(first?Param::Lfo1Phase:Param::Lfo2Phase))*get(first?Param::Lfo1Depth:Param::Lfo2Depth);
        const float step=static_cast<float>(1/(sampleRate*.002));lfoValues[i]+=std::clamp(target-lfoValues[i],-step,step);
    }
    if(routingFade==routingFadeLength){routing=pendingRouting;controlCounter=0;}
    const bool control=controlCounter==0;controlCounter=(controlCounter+1)%8;
    float pitchMod=0,cutoffMod=0,width=get(Param::Width);
    for(int i=0;i<2;++i){if(routing.modulationMask[i]&1)pitchMod+=lfoValues[i]*get(i==0?Param::Lfo1Pitch:Param::Lfo2Pitch);if(routing.modulationMask[i]&2)cutoffMod+=lfoValues[i]*get(i==0?Param::Lfo1Cutoff:Param::Lfo2Cutoff);if(routing.modulationMask[i]&8)width+=lfoValues[i]*get(i==0?Param::Lfo1Width:Param::Lfo2Width);}
    width=std::clamp(width,0.0f,1.0f);
    if(control)for(int i=0;i<8;++i){const double cents=positions[i]*get(Param::UnisonDetune);ratioA[i]=std::exp2(get(Param::OctaveA)+(get(Param::DetuneA)+cents)/1200.0);ratioB[i]=std::exp2(get(Param::OctaveB)+get(Param::SemitoneB)/12.0+(get(Param::DetuneB)+cents)/1200.0);
        const double angle=(positions[i]*get(Param::Spread)*width+1)*pi*.25;panLeft[i]=static_cast<float>(std::cos(angle)*std::sqrt(2.0));panRight[i]=static_cast<float>(std::sin(angle)*std::sqrt(2.0));}
    VoiceFrames a{},b{};StereoFrame stealTail{};
    for(int n=0;n<polyphony;++n){auto& voice=voices[n];
        if(voice.glideSamples>0){voice.currentMidi+=voice.glideStep;if(--voice.glideSamples==0)voice.currentMidi=voice.targetMidi;}
        const float env=voice.env.tick(sampleRate,get(Param::Attack),get(Param::Decay),get(Param::Sustain),voice.panic?.005f:get(Param::Release));envelopeLevels[n]=env;
        if(control){const double frequency=440*std::exp2((voice.currentMidi-69+pitchMod+bends[voice.channel-1].current()*get(Param::BendRange))/12.0);voice.frequencyStep=(frequency-voice.frequency)/8;
            const float envMod=(routing.modulationMask[2]&2)?env*get(Param::EnvCutoff):0;
            const double cutoff=std::clamp(static_cast<double>(get(Param::Cutoff))*std::exp2(cutoffMod+envMod),15.0,sampleRate*.45);
            const float g=static_cast<float>(std::tan(pi*cutoff/sampleRate));voice.filterStep=(g-voice.filterG)/8;
        }
        voice.frequency+=voice.frequencyStep;voice.filterG=std::max(0.0f,voice.filterG+voice.filterStep);
        if(voice.env.active()||env>0){
            const int waveA=static_cast<int>(parameters[index(Param::WaveA)]),waveB=static_cast<int>(parameters[index(Param::WaveB)]);
            if(waveA!=voice.waveA){voice.oldA=voice.a;voice.oldWaveA=voice.waveA;voice.fadeA=std::max(1,static_cast<int>(sampleRate*.005));voice.waveA=waveA;}
            if(waveB!=voice.waveB){voice.oldB=voice.b;voice.oldWaveB=voice.waveB;voice.fadeB=std::max(1,static_cast<int>(sampleRate*.005));voice.waveB=waveB;}
            const float fadeA=static_cast<float>(voice.fadeA)/static_cast<float>(sampleRate*.005),fadeB=static_cast<float>(voice.fadeB)/static_cast<float>(sampleRate*.005);
            for(int lane=0;lane<8;++lane)if(weights[lane]>.000001f){float sa=voice.a[lane].tick(voice.frequency*ratioA[lane],sampleRate,voice.waveA),sb=voice.b[lane].tick(voice.frequency*ratioB[lane],sampleRate,voice.waveB);
                if(voice.fadeA>0)sa=sa*(1-fadeA)+voice.oldA[lane].tick(voice.frequency*ratioA[lane],sampleRate,voice.oldWaveA)*fadeA;
                if(voice.fadeB>0)sb=sb*(1-fadeB)+voice.oldB[lane].tick(voice.frequency*ratioB[lane],sampleRate,voice.oldWaveB)*fadeB;
                const float amplitude=weights[lane]*normal*.16f*env*voice.velocity;
                a[n]+={sa*get(Param::LevelA)*panLeft[lane]*amplitude,sa*get(Param::LevelA)*panRight[lane]*amplitude};
                b[n]+={sb*get(Param::LevelB)*panLeft[lane]*amplitude,sb*get(Param::LevelB)*panRight[lane]*amplitude};
            }
            const float sub=voice.sub.tick(voice.frequency*.5,sampleRate,0)*get(Param::SubLevel);
            const float noise=randomBipolar(voice.noiseState)*get(Param::NoiseLevel);
            const float extra=(sub+noise)*.16f*env*voice.velocity;a[n]+={extra,extra};
            if(voice.fadeA>0)--voice.fadeA;if(voice.fadeB>0)--voice.fadeB;
        }
        if(voice.tailSamples>0){stealTail+=voice.tail*(static_cast<float>(voice.tailSamples--)/static_cast<float>(sampleRate*.003));}
    }
    StereoFrame value=processGraph(a,b,p)+stealTail;
    float fade=1;if(routingFade>0){fade=routingFade>routingFadeLength?static_cast<float>(routingFade-routingFadeLength)/routingFadeLength:1-static_cast<float>(routingFade)/routingFadeLength;--routingFade;}
    const float gain=std::pow(10.0f,(get(Param::Output)+get(Param::Master))/20)*fade*panicGain.next();
    value.left=.96f*std::tanh(value.left*gain);value.right=.96f*std::tanh(value.right*gain);
    if(!std::isfinite(value.left))value.left=0;if(!std::isfinite(value.right))value.right=0;return value;
}
float Engine::tick() noexcept {const auto frame=tickStereo();return (frame.left+frame.right)*.5f;}
}
