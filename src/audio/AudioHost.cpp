#include "audio/AudioHost.h"

namespace noxis {
AudioHost::AudioHost() {
    initialError=devices.initialise(0,2,nullptr,true);
    devices.addAudioCallback(this);
    devices.addMidiInputDeviceCallback({},this);
}
AudioHost::~AudioHost(){devices.removeMidiInputDeviceCallback({},this);devices.removeAudioCallback(this);devices.closeAudioDevice();}
void AudioHost::audioDeviceAboutToStart(juce::AudioIODevice* device) {
    rate.store(device->getCurrentSampleRate());engine.prepare(rate.load());parameters.read(snapshot);engine.setParameters(snapshot.values,true);engine.setRouting(snapshot.routing,true);
}
void AudioHost::audioDeviceStopped(){voices.store(0);peak.store(0);rms.store(0);for(auto& p:stereoPeak)p.store(0);for(auto& r:stereoRms)r.store(0);}
void AudioHost::handleIncomingMidiMessage(juce::MidiInput*,const juce::MidiMessage& m) {
    if(m.isNoteOn()){midiDown[m.getNoteNumber()].store(true);send({NoteEvent::On,m.getNoteNumber(),m.getFloatVelocity(),m.getChannel()});}
    else if(m.isNoteOff()){midiDown[m.getNoteNumber()].store(false);send({NoteEvent::Off,m.getNoteNumber(),0,m.getChannel()});}
    else if(m.isPitchWheel())send({NoteEvent::Bend,0,static_cast<float>(m.getPitchWheelValue()-8192)/(m.getPitchWheelValue()<8192?8192.0f:8191.0f),m.getChannel()});
    else if(m.isController()&&m.getControllerNumber()==64)send({NoteEvent::Pedal,0,m.getControllerValue()>=64?1.0f:0.0f,m.getChannel()});
    else if(m.isAllNotesOff()||m.isAllSoundOff())panic();
}
void AudioHost::audioDeviceIOCallbackWithContext(const float* const*,int,float* const* output,int channels,int samples,const juce::AudioIODeviceCallbackContext&) {
    juce::ScopedNoDenormals noDenormals;
    if(parameters.read(snapshot)){engine.setParameters(snapshot.values);engine.setRouting(snapshot.routing);}
    NoteEvent event;int count=0;
    const bool shouldPanic=panicRequested.exchange(false);
    if(shouldPanic){engine.allNotesOff();for(auto& key:midiDown)key.store(false);}
    while(count++<512 && events.pop(event)) {
        if(event.epoch!=eventEpoch.load())continue;
        switch(event.kind) {
            case NoteEvent::On:engine.noteOn(event.note,event.value,event.channel);break;
            case NoteEvent::Off:engine.noteOff(event.note,event.channel);break;
            case NoteEvent::Pedal:engine.sustain(event.value>0,event.channel);break;
            case NoteEvent::Panic:engine.allNotesOff();break;
            case NoteEvent::Bend:engine.pitchBend(event.value,event.channel);break;
        }
    }
    std::array<float,2> maximum{};std::array<double,2> square{};
    const bool capturing=captureEnabled.load(std::memory_order_acquire);
    auto captured=captureSize.load(std::memory_order_relaxed);
    for(int s=0;s<samples;++s) {
        const auto value=engine.tickStereo();maximum[0]=std::max(maximum[0],std::abs(value.left));maximum[1]=std::max(maximum[1],std::abs(value.right));square[0]+=value.left*value.left;square[1]+=value.right*value.right;
        for(int c=0;c<channels;++c)if(output[c])output[c][s]=channels==1?(value.left+value.right)*.5f:(c%2==0?value.left:value.right);
        if(++scopeDivider==4){scope.push((value.left+value.right)*.5f);scopeDivider=0;}
        if(capturing&&captured+1<capture.size()){capture[captured++]=value.left;capture[captured++]=value.right;}
    }
    captureSize.store(captured,std::memory_order_release);
    for(int c=0;c<2;++c){stereoPeak[c].store(maximum[c]);stereoRms[c].store(static_cast<float>(std::sqrt(square[c]/std::max(1,samples))));}
    peak.store(std::max(maximum[0],maximum[1]),std::memory_order_relaxed);rms.store((stereoRms[0].load()+stereoRms[1].load())*.5f,std::memory_order_relaxed);
    const auto modulation=engine.modulationValues();for(int c=0;c<2;++c)lfo[c].store(modulation[c]);envelopeLevel.store(engine.envelopeLevel());
    voices.store(engine.activeVoices(),std::memory_order_relaxed);callbacks.fetch_add(1,std::memory_order_relaxed);
}
std::vector<float> AudioHost::capturedAudio() const {const auto size=captureSize.load(std::memory_order_acquire);return {capture.begin(),capture.begin()+static_cast<std::ptrdiff_t>(size)};}
}
