#include "synth/Engine.h"
#include "state/Preset.h"
#include "audio/WavWriter.h"
#include <iostream>
#include <vector>

int main(int argc,char** argv) {
    constexpr int rate=48000;
    const auto& library=noxis::factoryPresets();
    const std::array<std::size_t,8> selection{1,4,12,18,23,27,32,35};
    std::vector<float> audio;audio.reserve(rate*3*2*selection.size());double square=0;float peak=0;
    for(auto item:selection) {
        const auto& preset=library.at(item);noxis::Engine engine;engine.prepare(rate);engine.setParameters(preset.values,true);
        noxis::CompiledRouting plan;std::string error;if(!noxis::compileRouting(preset.routing,plan,error)){std::cerr<<error<<'\n';return 1;}engine.setRouting(plan,true);
        const int note=std::clamp(preset.category=="BASS"?45:60,preset.lowNote,preset.highNote);
        for(int i=0;i<rate*3;++i){
            if(i==0)engine.noteOn(note,.8f);if(i==rate/2)engine.noteOn(std::min(127,note+7),.6f);
            if(i==rate*3/2){engine.noteOff(note);engine.noteOff(std::min(127,note+7));}
            if(i==rate*11/4)engine.allNotesOff();
            const auto frame=engine.tickStereo();for(float sample:{frame.left,frame.right}){if(!std::isfinite(sample)||std::abs(sample)>.96001f){std::cerr<<"Некорректный аудиосигнал\n";return 1;}peak=std::max(peak,std::abs(sample));square+=sample*sample;audio.push_back(sample);}
        }
    }
    const double rms=std::sqrt(square/static_cast<double>(audio.size()));if(peak<.005f||rms<.0001){std::cerr<<"Сигнал не прошёл проверку уровня\n";return 1;}
    const std::filesystem::path path=argc>1?argv[1]:"noxis-demo.wav";
    if(!noxis::writeWav(path,audio,rate,2)){std::cerr<<"Не удалось записать WAV\n";return 1;}
    if(argc>2){std::filesystem::create_directories(argv[2]);std::string error;for(const auto& preset:library)if(!noxis::savePreset(std::filesystem::path(argv[2])/(preset.name+".noxis"),preset,error)){std::cerr<<error<<'\n';return 1;}}
    std::cout<<"WAV: "<<path.string()<<" | 24 s, 48000 Hz, PCM16 stereo | peak="<<peak<<" RMS="<<rms<<" | library="<<library.size()<<'\n';
}
