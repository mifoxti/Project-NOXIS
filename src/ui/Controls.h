#pragma once
#include "audio/AudioHost.h"
#include "state/Preset.h"

namespace noxis::ui {
inline juce::String utf8(const char* value){return juce::String::fromUTF8(value);}
inline juce::Colour background(){return juce::Colour(0xff101018);}
inline juce::Colour panelColour(){return juce::Colour(0xff1b1b29);}
inline juce::Colour surface(){return juce::Colour(0xff242436);}
inline juce::Colour purple(){return juce::Colour(0xffb48aff);}
inline juce::Colour pink(){return juce::Colour(0xffef83c5);}
inline juce::Colour text(){return juce::Colour(0xfff0ecf8);}
inline juce::Colour muted(){return juce::Colour(0xff9894ab);}
class NoxisLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    NoxisLookAndFeel();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,const juce::Slider::SliderStyle,juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
};
class ParameterLabel : public juce::Label {
public:explicit ParameterLabel(const juce::String& title){setText(title,juce::dontSendNotification);setColour(textColourId,muted());setFont(juce::FontOptions(12));setJustificationType(juce::Justification::centred);}
};
class Knob : public juce::Component {
public:
    Knob(ParameterStore&,Param,const juce::String& title={});
    void resized() override;
    void sync();
    juce::Slider slider;
private:ParameterStore& store;Param parameter;ParameterLabel label;
};
class VerticalSlider final : public Knob {
public:VerticalSlider(ParameterStore& s,Param p):Knob(s,p){slider.setSliderStyle(juce::Slider::LinearVertical);}
};
class WaveformSelector final : public juce::Component {
public:
    WaveformSelector(ParameterStore&,Param);void resized() override;void sync();
private:ParameterStore& store;Param parameter;std::array<juce::TextButton,4> buttons;
};
class Panel final : public juce::Component {
public:Panel(juce::String title,juce::String subtitle={}):name(std::move(title)),detail(std::move(subtitle)){}void paint(juce::Graphics&) override;
private:juce::String name,detail;
};
class TabBar final : public juce::Component {
public:TabBar();void resized() override;void select(int);std::function<void(int)> onSelect;
private:std::array<juce::TextButton,4> buttons;
};
class PresetSelector final : public juce::ComboBox {public:PresetSelector();};
class AudioMeter final : public juce::Component {
public:void setLevel(float peak,float rms);void paint(juce::Graphics&) override;
private:float maximum=0,average=0;
};
class Scope final : public juce::Component {
public:explicit Scope(ScopeBuffer& b):buffer(b){}void update();void paint(juce::Graphics&) override;
private:ScopeBuffer& buffer;std::array<float,1024> history{};std::size_t cursor=0;float amplitude=0;
};
}
