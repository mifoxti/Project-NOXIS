#include "audio/AudioHost.h"

namespace noxis {
AudioHost::AudioHost() {
    initialError=devices.initialise(0,2,nullptr,true);
    devices.addAudioCallback(this);
    devices.addMidiInputDeviceCallback({},this);
}
AudioHost::~AudioHost(){devices.removeMidiInputDeviceCallback({},this);devices.removeAudioCallback(this);devices.closeAudioDevice();}
void AudioHost::audioDeviceAboutToStart(juce::AudioIODevice* device) {
    rate.store(device->getCurrentSampleRate());engine.prepare(rate.load());parameters.read(snapshot);engine.setParameters(snapshot,true);
}
void AudioHost::audioDeviceStopped(){voices.store(0);peak.store(0);rms.store(0);}
void AudioHost::handleIncomingMidiMessage(juce::MidiInput*,const juce::MidiMessage& m) {
    if(m.isNoteOn())send({NoteEvent::On,m.getNoteNumber(),m.getFloatVelocity(),m.getChannel()});
    else if(m.isNoteOff())send({NoteEvent::Off,m.getNoteNumber(),0,m.getChannel()});
    else if(m.isController()&&m.getControllerNumber()==64)send({NoteEvent::Pedal,0,m.getControllerValue()>=64?1.0f:0.0f,m.getChannel()});
    else if(m.isAllNotesOff()||m.isAllSoundOff())panic();
}
void AudioHost::audioDeviceIOCallbackWithContext(const float* const*,int,float* const* output,int channels,int samples,const juce::AudioIODeviceCallbackContext&) {
    juce::ScopedNoDenormals noDenormals;
    if(parameters.read(snapshot))engine.setParameters(snapshot);
    NoteEvent event;int count=0;
    const bool shouldPanic=panicRequested.exchange(false);
    while(count++<512 && events.pop(event)) {
        if(shouldPanic)continue;
        switch(event.kind) {
            case NoteEvent::On:engine.noteOn(event.note,event.value,event.channel);break;
            case NoteEvent::Off:engine.noteOff(event.note,event.channel);break;
            case NoteEvent::Pedal:engine.sustain(event.value>0,event.channel);break;
            case NoteEvent::Panic:engine.allNotesOff();break;
        }
    }
    if(shouldPanic)engine.allNotesOff();
    float maximum=0;double square=0;
    const bool capturing=captureEnabled.load(std::memory_order_acquire);
    auto captured=captureSize.load(std::memory_order_relaxed);
    for(int s=0;s<samples;++s) {
        const float value=engine.tick();maximum=std::max(maximum,std::abs(value));square+=value*value;
        for(int c=0;c<channels;++c)if(output[c])output[c][s]=value;
        if(++scopeDivider==4){scope.push(value);scopeDivider=0;}
        if(capturing&&captured<capture.size())capture[captured++]=value;
    }
    captureSize.store(captured,std::memory_order_release);
    peak.store(maximum,std::memory_order_relaxed);rms.store(static_cast<float>(std::sqrt(square/std::max(1,samples))),std::memory_order_relaxed);
    voices.store(engine.activeVoices(),std::memory_order_relaxed);callbacks.fetch_add(1,std::memory_order_relaxed);
}
std::vector<float> AudioHost::capturedAudio() const {const auto size=captureSize.load(std::memory_order_acquire);return {capture.begin(),capture.begin()+static_cast<std::ptrdiff_t>(size)};}
}
