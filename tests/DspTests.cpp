#include "synth/Engine.h"
#include "state/Preset.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <thread>

namespace {
int checks=0;
void require(bool condition,const char* message) {++checks;if(!condition){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
void render(noxis::Engine& e,int samples) {for(int i=0;i<samples;++i)require(std::isfinite(e.tick()),"finite output");}
}
int main() {
    using namespace noxis;
    require(std::abs(midiFrequency(69)-440)<1e-9,"MIDI A4");
    for(double rate:{44100.,48000.,96000.}) for(int wave=0;wave<4;++wave) {
        Oscillator osc;float previous=0;int crossings=0;double power=0;
        for(int i=0;i<static_cast<int>(rate);++i) {float s=osc.tick(440,rate,wave);if(previous<=0&&s>0)++crossings;previous=s;power+=s*s;}
        require(std::abs(crossings-440)<=1,"oscillator fundamental frequency");require(power/rate>.05,"oscillator not silent");
    }
    Envelope env;env.on();
    for(int i=0;i<480;++i)env.tick(48000,.01f,.1f,.4f,.1f);
    require(env.level()>.99f,"attack duration");
    for(int i=0;i<4900;++i)env.tick(48000,.01f,.1f,.4f,.1f);
    require(std::abs(env.level()-.4f)<.001f,"decay and sustain");env.off();
    for(int i=0;i<4900;++i)env.tick(48000,.01f,.1f,.4f,.1f);
    require(!env.active(),"release finishes");
    Engine engine;engine.prepare(48000);require(engine.tick()==0,"initial silence");
    for(int n=60;n<68;++n)engine.noteOn(n,.8f);
    require(engine.activeVoices()==8,"eight independent voices");engine.noteOn(80,1);require(engine.activeVoices()==8,"voice stealing bounded");
    double energy=0;for(int i=0;i<4800;++i){float s=engine.tick();energy+=s*s;require(std::isfinite(s)&&std::abs(s)<=.96f,"bounded chord");}
    require(energy>1,"chord not silent");engine.allNotesOff();render(engine,1000);require(engine.activeVoices()==0,"panic releases all voices");
    engine.noteOn(60,1);render(engine,100);engine.sustain(true);engine.noteOff(60);render(engine,24000);
    require(engine.activeVoices()==1,"sustain holds voice");engine.sustain(false);render(engine,48000);require(engine.activeVoices()==0,"note off after pedal release");
    engine.noteOn(60,1,2);engine.noteOff(60,1);render(engine,2000);require(engine.activeVoices()==1,"MIDI channels isolated");engine.noteOff(60,2);render(engine,48000);require(engine.activeVoices()==0,"channel note off");
    const auto path=std::filesystem::temp_directory_path()/"noxis-preset-test.noxis";
    std::string error;
    for(const auto& original:factoryPresets()) {
        require(savePreset(path,original,error),"save preset");Preset restored;require(loadPreset(path,restored,error),"load preset");
        require(restored.name==original.name&&restored.values==original.values,"exact preset round trip");
        Engine a,b;a.prepare(48000);b.prepare(48000);a.setParameters(original.values,true);b.setParameters(restored.values,true);a.noteOn(48,1);b.noteOn(48,1);
        for(int i=0;i<5000;++i)require(a.tick()==b.tick(),"reproducible sound");
    }
    {std::ofstream out(path);out<<"NOXIS_PRESET 1\nBad\ncutoff nan\n";}
    Preset intact=factoryPresets()[1];require(!loadPreset(path,intact,error),"reject malformed preset");require(intact.name=="Midnight Reese","failed load is transactional");
    std::filesystem::remove(path);
    ParameterStore store;std::atomic<bool> done{false};
    auto low=defaults(),high=defaults();low[index(Param::DetuneA)]=-20;low[index(Param::DetuneB)]=-20;high[index(Param::DetuneA)]=20;high[index(Param::DetuneB)]=20;
    std::thread writer([&]{for(int i=0;i<10000;++i)store.replace(i%2?low:high);done.store(true);});
    auto snapshot=defaults();while(!done.load())if(store.read(snapshot))require(snapshot[index(Param::DetuneA)]==snapshot[index(Param::DetuneB)],"coherent preset snapshot");writer.join();
    std::mt19937 random(42);
    for(double rate:{22050.,44100.,48000.,96000.}) {
        engine.prepare(rate);
        for(int block=0;block<150;++block) {
            auto values=defaults();for(std::size_t p=0;p<values.size();++p){const auto& d=parameterInfo[p];values[p]=d.min+(d.max-d.min)*static_cast<float>(random()%1001)/1000;}
            engine.setParameters(values);engine.noteOn(static_cast<int>(random()%128),.9f);if(block%9==0)engine.allNotesOff();
            for(int s=0;s<256;++s){float v=engine.tick();require(std::isfinite(v)&&std::abs(v)<=.96f,"stress parameter extremes");}
        }
    }
    std::cout<<"PASS: oscillator, ADSR, polyphony, MIDI, presets, snapshots, stress ("<<checks<<" checks)\n";
}
