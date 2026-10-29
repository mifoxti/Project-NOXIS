#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace noxis {
struct StereoFrame {
    float left=0,right=0;
    StereoFrame& operator+=(StereoFrame other) noexcept {left+=other.left;right+=other.right;return *this;}
};
inline StereoFrame operator+(StereoFrame a,StereoFrame b) noexcept {return {a.left+b.left,a.right+b.right};}
inline StereoFrame operator-(StereoFrame a,StereoFrame b) noexcept {return {a.left-b.left,a.right-b.right};}
inline StereoFrame operator*(StereoFrame a,float gain) noexcept {return {a.left*gain,a.right*gain};}
inline StereoFrame blend(StereoFrame dry,StereoFrame wet,float mix) noexcept {return dry*(1-mix)+wet*mix;}

// Все буферы создаются в prepare; чтение и запись не выделяют память.
class DelayLine {
public:
    void prepare(std::size_t capacity){data.assign(std::max<std::size_t>(4,capacity),0);write=valid=0;}
    // Логический сброс O(1): старые samples недоступны до новой записи.
    void reset() noexcept {write=valid=0;}
    float read(float distance) const noexcept {
        if(data.empty())return 0;
        distance=std::clamp(distance,1.0f,static_cast<float>(data.size()-2));
        const auto newer=static_cast<std::size_t>(distance);const float fraction=distance-static_cast<float>(newer);
        const auto tap=[&](std::size_t age){return age>valid?0.0f:data[(write+data.size()-age)%data.size()];};
        return tap(newer)*(1-fraction)+tap(newer+1)*fraction;
    }
    void push(float value) noexcept {if(data.empty())return;data[write]=std::clamp(value,-16.0f,16.0f);if(++write==data.size())write=0;if(valid<data.size())++valid;}
private:std::vector<float> data;std::size_t write=0,valid=0;
};
class StereoDelay {
public:
    void prepare(double sampleRate){rate=sampleRate;for(auto& line:lines)line.prepare(static_cast<std::size_t>(rate*1.51)+4);}
    void reset() noexcept {for(auto& line:lines)line.reset();}
    StereoFrame process(StereoFrame input,float time,float feedback,float mix,bool pingPong) noexcept {
        const float distance=std::clamp(time,.02f,1.5f)*static_cast<float>(rate);
        const StereoFrame delayed{lines[0].read(distance),lines[1].read(distance)};
        feedback=std::clamp(feedback,0.0f,.85f);
        lines[0].push(input.left+(pingPong?delayed.right:delayed.left)*feedback);
        lines[1].push(input.right+(pingPong?delayed.left:delayed.right)*feedback);
        return blend(input,delayed,std::clamp(mix,0.0f,1.0f));
    }
private:std::array<DelayLine,2> lines;double rate=48000;
};
class StereoChorus {
public:
    void prepare(double sampleRate){rate=sampleRate;phase=0;for(auto& line:lines)line.prepare(static_cast<std::size_t>(rate*.04)+4);}
    void reset() noexcept {for(auto& line:lines)line.reset();phase=0;}
    StereoFrame process(StereoFrame input,float speed,float depth,float mix) noexcept {
        constexpr double twoPi=6.2831853071795864769;
        const float leftDelay=static_cast<float>(rate)*(.014f+.006f*depth*static_cast<float>(std::sin(phase*twoPi)));
        const float rightDelay=static_cast<float>(rate)*(.014f+.006f*depth*static_cast<float>(std::sin((phase+.25)*twoPi)));
        const StereoFrame delayed{lines[0].read(leftDelay),lines[1].read(rightDelay)};
        lines[0].push(input.left);lines[1].push(input.right);
        phase+=std::clamp(static_cast<double>(speed),.05,5.0)/rate;if(phase>=1)phase-=1;
        return blend(input,delayed,std::clamp(mix,0.0f,1.0f));
    }
private:std::array<DelayLine,2> lines;double rate=48000,phase=0;
};
class StereoReverb {
public:
    void prepare(double sampleRate) {
        constexpr std::array<double,4> seconds{.0297,.0371,.0411,.0437};
        for(std::size_t i=0;i<lines.size();++i){lengths[i]=static_cast<float>(std::round(sampleRate*seconds[i]));lines[i].prepare(static_cast<std::size_t>(lengths[i])+4);}
        constexpr std::array<double,4> diffusionSeconds{.0051,.0017,.0063,.0023};
        for(std::size_t i=0;i<diffusers.size();++i){diffusionLengths[i]=static_cast<float>(std::round(sampleRate*diffusionSeconds[i]));diffusers[i].prepare(static_cast<std::size_t>(diffusionLengths[i])+4);}
        dampingState={};
    }
    void reset() noexcept {for(auto& line:lines)line.reset();for(auto& line:diffusers)line.reset();dampingState={};}
    StereoFrame process(StereoFrame input,float size,float damping,float mix) noexcept {
        std::array<float,4> tap{};
        const float pole=.15f+.8f*std::clamp(damping,0.0f,1.0f);
        for(std::size_t i=0;i<lines.size();++i){const float sample=lines[i].read(lengths[i]);dampingState[i]=(1-pole)*sample+pole*dampingState[i];tap[i]=dampingState[i];}
        const float feedback=.42f+.46f*std::clamp(size,0.0f,1.0f);
        const std::array<float,4> mixed{
            (tap[0]+tap[1]+tap[2]+tap[3])*.5f,(tap[0]-tap[1]+tap[2]-tap[3])*.5f,
            (tap[0]+tap[1]-tap[2]-tap[3])*.5f,(tap[0]-tap[1]-tap[2]+tap[3])*.5f};
        const std::array<float,4> injection{input.left*.45f,input.right*.45f,(input.left+input.right)*.225f,(input.left-input.right)*.225f};
        for(std::size_t i=0;i<lines.size();++i)lines[i].push(injection[i]+mixed[i]*feedback);
        StereoFrame wet{(tap[0]+tap[1]-tap[2]+tap[3])*.45f,(tap[0]-tap[1]+tap[2]+tap[3])*.45f};
        auto diffuse=[&](float value,std::size_t stage){const float delayed=diffusers[stage].read(diffusionLengths[stage]);const float result=delayed-value*.5f;diffusers[stage].push(value+result*.5f);return result;};
        wet.left=diffuse(diffuse(wet.left,0),1);wet.right=diffuse(diffuse(wet.right,2),3);
        return blend(input,wet,std::clamp(mix,0.0f,1.0f));
    }
private:
    std::array<DelayLine,4> lines,diffusers;
    std::array<float,4> lengths{},diffusionLengths{},dampingState{};
};
}
