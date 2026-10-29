#include "synth/Engine.h"
#include "state/Preset.h"
#include "audio/WavWriter.h"
#include <iostream>
#include <vector>

int main(int argc,char** argv) {
    constexpr int rate=48000;
    noxis::Engine engine;engine.prepare(rate);
    std::vector<float> audio;audio.reserve(rate*12);
    double square=0;float peak=0;
    for(const auto& preset:noxis::factoryPresets()) {
        engine.setParameters(preset.values);
        for(int i=0;i<rate*3;++i) {
            if(i==0)engine.noteOn(45,.8f);
            if(i==rate/2)engine.noteOn(52,.65f);
            if(i==rate)engine.noteOn(57,.6f);
            if(i==rate*3/2) {engine.noteOff(45);engine.noteOff(52);engine.noteOff(57);}
            if(i==rate*5/2)engine.allNotesOff();
            const float sample=engine.tick();
            if(!std::isfinite(sample)) {std::cerr<<"Некорректный аудиосигнал\n";return 1;}
            peak=std::max(peak,std::abs(sample));square+=sample*sample;audio.push_back(sample);
        }
    }
    const double rms=std::sqrt(square/static_cast<double>(audio.size()));
    if(peak<.01f || peak>.97f || rms<.001) {std::cerr<<"Сигнал не прошёл проверку уровня\n";return 1;}
    const std::filesystem::path path=argc>1?argv[1]:"noxis-demo.wav";
    if(!noxis::writeWav(path,audio,rate)) {std::cerr<<"Не удалось записать WAV\n";return 1;}
    if(argc>2) {
        std::filesystem::create_directories(argv[2]);std::string error;
        for(const auto& preset:noxis::factoryPresets())if(!noxis::savePreset(std::filesystem::path(argv[2])/(preset.name+".noxis"),preset,error)){std::cerr<<error<<'\n';return 1;}
    }
    std::cout<<"WAV: "<<path.string()<<" | 12 s, 48000 Hz, PCM16 mono | peak="<<peak<<" RMS="<<rms<<'\n';
}
