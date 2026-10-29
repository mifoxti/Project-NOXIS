#include "state/Preset.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_set>

namespace noxis {
const std::array<Preset,4>& factoryPresets() {
    static const auto presets=[] {
        std::array<Preset,4> result{{{"Init",defaults()},{"Midnight Reese",defaults()},{"Afterglow Lead",defaults()},{"Nightdrive Bass",defaults()}}};
        auto set=[&](int n,Param p,float v){result[static_cast<std::size_t>(n)].values[index(p)]=v;};
        set(1,Param::WaveA,1);set(1,Param::WaveB,1);set(1,Param::OctaveA,-1);set(1,Param::OctaveB,-1);
        set(1,Param::DetuneA,-13);set(1,Param::DetuneB,13);set(1,Param::LevelB,.7f);set(1,Param::Cutoff,850);
        set(1,Param::Resonance,.3f);set(1,Param::Attack,.035f);set(1,Param::Release,.45f);set(1,Param::Drive,8);
        set(2,Param::WaveA,3);set(2,Param::WaveB,1);set(2,Param::DetuneB,7);set(2,Param::LevelB,.23f);
        set(2,Param::Attack,.16f);set(2,Param::Decay,.6f);set(2,Param::Sustain,.65f);set(2,Param::Release,1.4f);
        set(2,Param::Cutoff,3400);set(2,Param::Resonance,.2f);set(2,Param::Drive,3);
        set(3,Param::WaveA,2);set(3,Param::WaveB,0);set(3,Param::OctaveA,-1);set(3,Param::OctaveB,-2);
        set(3,Param::LevelB,.8f);set(3,Param::Cutoff,500);set(3,Param::Resonance,.18f);set(3,Param::Drive,12);
        set(3,Param::Attack,.006f);set(3,Param::Decay,.18f);set(3,Param::Sustain,.6f);set(3,Param::Release,.12f);
        return result;
    }(); return presets;
}
bool savePreset(const std::filesystem::path& path,const Preset& p,std::string& error) {
    if(p.name.empty() || p.name.size()>120 || p.name.find_first_of("\r\n")!=std::string::npos) { error="Некорректное имя пресета";return false; }
    std::ofstream out(path,std::ios::binary|std::ios::trunc);
    if(!out) { error="Не удалось открыть файл для записи";return false; }
    out<<"NOXIS_PRESET 1\n"<<p.name<<'\n'<<std::setprecision(9);
    for(std::size_t i=0;i<p.values.size();++i) out<<parameterInfo[i].id<<' '<<sanitise(static_cast<Param>(i),p.values[i])<<'\n';
    out.flush();if(!out) { error="Ошибка записи пресета";return false; } error.clear();return true;
}
bool loadPreset(const std::filesystem::path& path,Preset& p,std::string& error) {
    std::error_code ec;const auto size=std::filesystem::file_size(path,ec);
    if(ec || size>16384) { error="Файл недоступен или слишком велик";return false; }
    std::ifstream in(path,std::ios::binary);std::string line;
    auto trim=[](std::string& s){if(!s.empty()&&s.back()=='\r')s.pop_back();};
    std::getline(in,line);trim(line);
    if(line!="NOXIS_PRESET 1") { error="Неподдерживаемый формат пресета";return false; }
    Preset candidate;std::getline(in,candidate.name);trim(candidate.name);
    if(candidate.name.empty() || candidate.name.size()>120) { error="Некорректное имя пресета";return false; }
    std::unordered_set<std::string> seen;
    while(std::getline(in,line)) {
        trim(line); if(line.empty()) continue;
        std::istringstream row(line);std::string key,extra;float value;
        if(!(row>>key>>value) || (row>>extra) || !std::isfinite(value) || !seen.insert(key).second) {error="Повреждённые параметры пресета";return false;}
        bool found=false;
        for(std::size_t i=0;i<parameterInfo.size();++i) if(key==parameterInfo[i].id) {
            const auto& d=parameterInfo[i];if(value<d.min || value>d.max || (d.integral&&value!=std::round(value))) {error="Параметр вне допустимого диапазона";return false;}
            candidate.values[i]=value;found=true;break;
        }
        if(!found) {error="Неизвестный параметр пресета";return false;}
    }
    if(seen.size()!=parameterInfo.size()) {error="Неполный пресет";return false;}
    p=std::move(candidate);error.clear();return true;
}
}
