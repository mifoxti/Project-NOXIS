#pragma once
#include "ui/Controls.h"
#include "ui/KeyboardMap.h"
#include <memory>
#include <vector>

namespace noxis::ui {
class MainComponent final : public juce::Component,private juce::Timer,private juce::MidiKeyboardStateListener,private juce::KeyListener {
public:
    MainComponent();~MainComponent() override;
    void paint(juce::Graphics&) override;void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    bool keyStateChanged(bool) override;
    bool keyPressed(const juce::KeyPress& key,juce::Component*) override {return keyPressed(key);}
    bool keyStateChanged(bool down,juce::Component*) override {return keyStateChanged(down);}
    void parentHierarchyChanged() override;
    AudioHost host;
    void applyPreset(int);
private:
    void timerCallback() override;
    void handleNoteOn(juce::MidiKeyboardState*,int,int,float) override;
    void handleNoteOff(juce::MidiKeyboardState*,int,int,float) override;
    void choosePresetFile(bool save);
    void setMode(bool);void changeOctave(int);
    Knob& addKnob(juce::Component&,Param,const juce::String& title={});
    void sync();void layoutControls();
    NoxisLookAndFeel look;
    juce::TooltipWindow tooltips{this,700};
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboard{keyboardState,juce::MidiKeyboardComponent::horizontalKeyboard};
    PresetSelector presets;
    juce::TextButton previous{"<"},next{">"},save{utf8("Сохранить")},load{utf8("Загрузить")},noob{"NOOB"},pro{"PRO"},settings{utf8("Аудио")},panic{"PANIC"},octDown{utf8("−")},octUp{"+"};
    Knob master{host.parameters,Param::Master};
    TabBar tabs;
    Panel oscA{"OSC A",utf8("Основной осциллятор")},oscB{"OSC B",utf8("Второй осциллятор")},filter{"FILTER",utf8("Резонансный low-pass")},envelope{"ENVELOPE",utf8("Амплитуда · ADSR")},saturation{"SATURATION",utf8("Мягкое насыщение")},scopePanel{"OUTPUT",utf8("Осциллограф · реальный сигнал")},macros{"SOUND SHAPING",utf8("Общие параметры движка")};
    juce::Component synthView,routeView,fxView,presetView;
    Scope scope{host.scope};
    WaveformSelector waveA{host.parameters,Param::WaveA},waveB{host.parameters,Param::WaveB};
    std::vector<std::unique_ptr<Knob>> knobs;
    std::array<Knob*,3> oscAKnobs{},oscBKnobs{};
    std::array<Knob*,2> filterKnobs{},satKnobs{},fxKnobs{};
    std::array<Knob*,4> envKnobs{};
    Knob* tone=nullptr;Knob* drive=nullptr;
    juce::Slider movement;
    ParameterLabel movementLabel{utf8("Movement · detune")};
    AudioMeter meter;
    juce::Label status,octave,voiceCount,levelText;
    std::array<juce::TextButton,4> presetCards;
    std::unique_ptr<juce::FileChooser> chooser;
    bool simple=false;int currentTab=0,currentPreset=0,baseOctave=5;
    juce::Component::SafePointer<juce::DialogWindow> audioDialog;
    KeyboardTracker computerKeyboard;
    juce::String presetName{"Init"};Values savedValues=defaults();
};
}
