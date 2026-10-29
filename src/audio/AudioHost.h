#pragma once
#include "synth/Engine.h"
#include "audio/RealtimeQueues.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <array>

namespace noxis {
struct NoteEvent { enum Kind { On, Off, Pedal, Panic } kind=Panic; int note=0; float value=0; int channel=1; };
class AudioHost final : private juce::AudioIODeviceCallback,private juce::MidiInputCallback {
public:
    AudioHost();~AudioHost() override;
    void send(NoteEvent event) noexcept {if(!events.push(event))panicRequested.store(true);}
    void panic() noexcept {panicRequested.store(true);}
    ParameterStore parameters;
    juce::AudioDeviceManager devices;
    ScopeBuffer scope;
    std::atomic<float> peak{0},rms{0};std::atomic<int> voices{0};
    std::atomic<std::uint64_t> callbacks{0};
    juce::String initialError;
    void beginCapture() noexcept {captureEnabled.store(true,std::memory_order_release);}
    void endCapture() noexcept {captureEnabled.store(false,std::memory_order_release);}
    std::vector<float> capturedAudio() const;
    double currentRate() const noexcept {return rate.load();}
private:
    void audioDeviceIOCallbackWithContext(const float* const*,int,float* const*,int,int,const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart(juce::AudioIODevice*) override;
    void audioDeviceStopped() override;
    void handleIncomingMidiMessage(juce::MidiInput*,const juce::MidiMessage&) override;
    Engine engine;
    Values snapshot=defaults();
    EventQueue<NoteEvent,512> events;
    std::atomic<bool> panicRequested{false},captureEnabled{false};
    std::array<float,48000*8> capture{};
    std::atomic<std::size_t> captureSize{0};
    std::atomic<double> rate{48000};
    int scopeDivider=0;
};
}
