#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace noxis {
inline float randomBipolar(std::uint32_t& state) noexcept {
    state^=state<<13;state^=state>>17;state^=state<<5;
    return static_cast<float>(state>>8)*(2.0f/16777215.0f)-1;
}
class Lfo {
public:
    void reset(double position=0) noexcept {phase=position-std::floor(position);previous=phase;held=randomBipolar(random);}
    float tick(double frequency,double sampleRate,int waveform,double offset=0) noexcept {
        const double position=phase+offset-std::floor(phase+offset);
        if(position<previous)held=randomBipolar(random);previous=position;
        float value=0;
        switch(waveform){
            case 1:value=static_cast<float>(1-4*std::abs(position-.5));break;
            case 2:value=static_cast<float>(2*position-1);break;
            case 3:value=position<.5?1.0f:-1.0f;break;
            case 4:value=held;break;
            default:value=static_cast<float>(std::sin(position*6.2831853071795864769));break;
        }
        phase+=std::clamp(frequency,0.0,100.0)/sampleRate;phase-=std::floor(phase);return value;
    }
    static double synchronisedRate(double requested,double bpm) noexcept {
        constexpr double choices[]{.0625,.125,.25,.5,1,2,4,8,16};
        double closest=choices[0],distance=1000;
        for(double choice:choices){const double delta=std::abs(std::log2(std::max(requested,.001)/choice));if(delta<distance){distance=delta;closest=choice;}}
        return closest*std::clamp(bpm,40.0,240.0)/60;
    }
private:double phase=0,previous=0;float held=0;std::uint32_t random=0x7a42e9b1u;
};
}
