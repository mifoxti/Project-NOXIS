#include "state/Preset.h"
#include "synth/Engine.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {
void require(bool condition,const std::string& message) {
    if(!condition){std::cerr<<"Ошибка библиотеки: "<<message<<'\n';std::exit(1);}
}
}

int main() {
    using namespace noxis;
    const auto& library=factoryPresets();
    require(library.size()>=34,"недостаточно заводских пресетов");
    std::set<std::string> names;
    std::map<std::string,int> categories;
    std::vector<std::array<double,64>> fingerprints;
    const auto temporary=std::filesystem::temp_directory_path()/"noxis-library-roundtrip.noxis";
    double libraryPeak=0,smallestRms=1;
    for(std::size_t presetIndex=0;presetIndex<library.size();++presetIndex) {
        const auto& p=library[presetIndex];
        require(!p.name.empty()&&names.insert(p.name).second,"пустое или повторяющееся имя");
        require(!p.category.empty()&&!p.description.empty()&&!p.tags.empty()&&!p.character.empty(),p.name+": неполные метаданные");
        require(p.lowNote>=0&&p.highNote<=127&&p.lowNote<=p.highNote,p.name+": диапазон нот");
        ++categories[p.category];
        for(std::size_t i=0;i<p.values.size();++i)
            require(std::isfinite(p.values[i])&&sanitise(static_cast<Param>(i),p.values[i])==p.values[i],p.name+": значение параметра вне диапазона");
        for(std::size_t i=0;i<presetIndex;++i)
            require(p.values!=library[i].values||p.routing!=library[i].routing,p.name+": дублирует состояние "+library[i].name);
        std::string error;CompiledRouting plan;
        require(compileRouting(p.routing,plan,error),p.name+": "+error);
        require(savePreset(temporary,p,error),p.name+": сохранение: "+error);
        Preset restored;
        require(loadPreset(temporary,restored,error),p.name+": загрузка: "+error);
        require(p.name==restored.name&&p.values==restored.values&&p.routing==restored.routing,p.name+": состояние не восстановлено точно");
        require(p.category==restored.category&&p.description==restored.description&&p.tags==restored.tags&&p.character==restored.character
                &&p.lowNote==restored.lowNote&&p.highNote==restored.highNote,p.name+": метаданные не восстановлены точно");

        constexpr int sampleRate=48000;
        Engine engine;engine.prepare(sampleRate);engine.setParameters(p.values,true);engine.setRouting(plan,true);
        const int note=p.lowNote+std::min(12,p.highNote-p.lowNote);
        const double seconds=std::max(2.0,static_cast<double>(p.values[index(Param::Attack)])+.5);
        const int samples=static_cast<int>(seconds*sampleRate);
        engine.noteOn(note,.8f);
        double power=0,sidePower=0,peak=0;int nearCeiling=0;
        std::array<double,64> fingerprint{};
        for(int i=0;i<samples;++i) {
            if(i==samples/3&&p.values[index(Param::Mono)]<.5f&&note+7<=p.highNote)engine.noteOn(note+7,.6f);
            if(i==samples*3/4){engine.noteOff(note);engine.noteOff(note+7);}
            const auto frame=engine.tickStereo();
            require(std::isfinite(frame.left)&&std::isfinite(frame.right),p.name+": NaN или Inf");
            const double maximum=std::max(std::abs(frame.left),std::abs(frame.right));
            require(maximum<=.96001,p.name+": превышен безопасный пик");
            peak=std::max(peak,maximum);if(maximum>.93)++nearCeiling;
            const double energy=(static_cast<double>(frame.left)*frame.left+static_cast<double>(frame.right)*frame.right)*.5;
            power+=energy;sidePower+=static_cast<double>(frame.left-frame.right)*(frame.left-frame.right);
            const auto bin=static_cast<std::size_t>(static_cast<long long>(i)*64/samples);fingerprint[bin]+=energy;
        }
        const double rms=std::sqrt(power/samples);
        require(peak>.00005&&rms>.000003,p.name+": тишина или практически неслышимый сигнал");
        require(nearCeiling<samples/100,p.name+": слишком длительная работа около ограничителя");
        if(p.values[index(Param::UnisonVoices)]>1&&p.values[index(Param::Width)]>.2f&&p.values[index(Param::Spread)]>.2f)
            require(sidePower>1.e-10,p.name+": stereo-unison не создаёт различий каналов");
        for(const auto& other:fingerprints) {
            double distance=0,scale=0;
            for(std::size_t i=0;i<fingerprint.size();++i){distance+=std::abs(fingerprint[i]-other[i]);scale+=fingerprint[i]+other[i];}
            require(distance/std::max(scale,1.e-12)>1.e-6,p.name+": аудиодинамика совпадает с другим пресетом");
        }
        fingerprints.push_back(fingerprint);libraryPeak=std::max(libraryPeak,peak);smallestRms=std::min(smallestRms,rms);
        engine.allNotesOff();for(int i=0;i<sampleRate/20;++i)engine.tickStereo();
        require(engine.activeVoices()==0,p.name+": Panic оставил активный голос");
    }
    std::error_code error;std::filesystem::remove(temporary,error);
    require(categories["BASS"]>=8&&categories["LEAD"]>=6&&categories["PAD"]>=5&&categories["PLUCK"]>=4&&categories["PHONK"]>=5&&categories["FX"]>=4,
            "неполный набор категорий");
    std::cout<<"Библиотека: "<<library.size()<<" пресетов; метаданные, round-trip, routing, конечность, уровень и stereo проверены. Peak="
             <<libraryPeak<<", минимальный RMS="<<smallestRms<<'\n';
}
