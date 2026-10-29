#pragma once
#include "ui/ParameterPanel.h"
#include "ui/PresetBrowser.h"
#include "ui/RoutingEditor.h"
#include "ui/KeyboardMap.h"
#include <memory>

namespace noxis::ui {
class MainComponent final : public juce::Component,private juce::Timer,private juce::MidiKeyboardStateListener,private juce::KeyListener {
public:
    MainComponent();~MainComponent() override;
    void paint(juce::Graphics&) override;void resized() override;
    bool keyPressed(const juce::KeyPress&) override;bool keyStateChanged(bool) override;
    bool keyPressed(const juce::KeyPress& k,juce::Component*) override{return keyPressed(k);}
    bool keyStateChanged(bool b,juce::Component*) override{return keyStateChanged(b);}
    void parentHierarchyChanged() override;
    AudioHost host;
    void applyPreset(int);
private:
    void timerCallback() override;
    void handleNoteOn(juce::MidiKeyboardState*,int,int,float) override;
    void handleNoteOff(juce::MidiKeyboardState*,int,int,float) override;
    void choosePresetFile(bool);void setMode(bool);void changeOctave(int);void previewPreset(int);
    void sync();void layoutPages();void selectNode(int);void stopNotes();void rebuildPresetSelector();
    NoxisLookAndFeel look;juce::TooltipWindow tooltips{this,600};
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboard{keyboardState,juce::MidiKeyboardComponent::horizontalKeyboard};
    juce::ComboBox presets,rootNote;TabBar tabs;
    juce::TextButton previous{"<"},next{">"},save{utf8("Сохранить")},load{utf8("Открыть")},noob{"NOOB"},pro{"PRO"},settings{utf8("Аудио")},panic{"PANIC"},octDown{"-"},octUp{"+"},keyboardEnabled{"KEYS"};
    Knob master{host.parameters,Param::Master};
    juce::Component synthPage,routingPage,fxPage,modPage;
    ParameterPanel oscA{host.parameters,"OSC A",{Param::WaveA,Param::OctaveA,Param::DetuneA,Param::LevelA,Param::SubLevel},5};
    ParameterPanel oscB{host.parameters,"OSC B",{Param::WaveB,Param::OctaveB,Param::SemitoneB,Param::DetuneB,Param::LevelB},5};
    ParameterPanel unison{host.parameters,"STEREO UNISON",{Param::UnisonVoices,Param::UnisonDetune,Param::Spread,Param::Width,Param::PhaseRandom},5};
    ParameterPanel filter{host.parameters,"FILTER",{Param::Cutoff,Param::Resonance,Param::EnvCutoff},3};
    ParameterPanel envelope{host.parameters,"AMPLITUDE",{Param::Attack,Param::Decay,Param::Sustain,Param::Release},4};
    ParameterPanel performance{host.parameters,"PERFORMANCE",{Param::Glide,Param::BendRange,Param::NoiseLevel,Param::Mono,Param::Legato},5};
    ParameterPanel lfoMini1{host.parameters,"LFO 1",{Param::Lfo1Wave,Param::Lfo1Rate,Param::Lfo1Depth},3};
    ParameterPanel lfoMini2{host.parameters,"LFO 2",{Param::Lfo2Wave,Param::Lfo2Rate,Param::Lfo2Depth},3};
    Panel scopePanel{"OUTPUT",utf8("Реальный выход · stereo → mono scope")},macroPanel{"SOUND SHAPING",utf8("Макросы общего звукового движка")};
    Scope scope{host.scope};SignalViews signals{host};
    Knob tone{host.parameters,Param::Cutoff,"Tone"},movement{host.parameters,Param::UnisonDetune,"Movement"},width{host.parameters,Param::Width,"Width"},drive{host.parameters,Param::Drive,"Drive"},space{host.parameters,Param::ReverbMix,"Space"};
    ParameterPanel saturation{host.parameters,"SATURATION",{Param::SaturationEnabled,Param::Drive,Param::Output,Param::SaturationMix},2};
    ParameterPanel chorus{host.parameters,"CHORUS",{Param::ChorusEnabled,Param::ChorusRate,Param::ChorusDepth,Param::ChorusMix},2};
    ParameterPanel delay{host.parameters,"DELAY",{Param::DelayEnabled,Param::DelayTime,Param::DelayFeedback,Param::DelayMix,Param::DelayStereo},3};
    ParameterPanel reverb{host.parameters,"REVERB",{Param::ReverbEnabled,Param::ReverbSize,Param::ReverbDamping,Param::ReverbMix},2};
    ParameterPanel lfo1{host.parameters,"LFO 1",{Param::Lfo1Wave,Param::Lfo1Rate,Param::Lfo1Depth,Param::Lfo1Phase,Param::Lfo1Sync,Param::Lfo1Retrigger},3};
    ParameterPanel lfo2{host.parameters,"LFO 2",{Param::Lfo2Wave,Param::Lfo2Rate,Param::Lfo2Depth,Param::Lfo2Phase,Param::Lfo2Sync,Param::Lfo2Retrigger},3};
    ParameterPanel matrix1{host.parameters,"LFO 1 > DESTINATION",{Param::Lfo1Pitch,Param::Lfo1Cutoff,Param::Lfo1Level,Param::Lfo1Width},4};
    ParameterPanel matrix2{host.parameters,"LFO 2 > DESTINATION",{Param::Lfo2Pitch,Param::Lfo2Cutoff,Param::Lfo2Level,Param::Lfo2Width},4};
    ParameterPanel tempo{host.parameters,"TEMPO / ENVELOPE",{Param::Bpm,Param::EnvCutoff},2};
    RoutingEditor routing{host.parameters};
    ParameterPanel inspector{host.parameters,"FILTER",{Param::Cutoff,Param::Resonance,Param::EnvCutoff},2};
    PresetBrowser browser;
    AudioMeter meterLeft,meterRight;juce::Label status,octave,voiceCount,levelText;
    std::unique_ptr<juce::FileChooser> chooser;juce::Component::SafePointer<juce::DialogWindow> audioDialog;
    KeyboardTracker computerKeyboard;std::array<bool,128> midiVisual{};bool syncingMidi=false;
    bool simple=false;int currentTab=0,currentPreset=0,baseOctave=5,previewNote=-1;juce::uint32 previewUntil=0;
    Preset savedPreset=factoryPresets()[0];
};
}
