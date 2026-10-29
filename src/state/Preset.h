#pragma once
#include "state/Parameters.h"
#include <filesystem>
#include <string>
#include <vector>

namespace noxis {
struct Preset {
    std::string name;Values values=defaults();
    std::string category="INIT",description,tags,character;
    int lowNote=36,highNote=84;RoutingGraph routing=defaultRouting();
};
const std::vector<Preset>& factoryPresets();
bool savePreset(const std::filesystem::path& path,const Preset& preset,std::string& error);
bool loadPreset(const std::filesystem::path& path,Preset& preset,std::string& error);
}
