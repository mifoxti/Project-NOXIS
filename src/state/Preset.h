#pragma once
#include "state/Parameters.h"
#include <filesystem>
#include <string>

namespace noxis {
struct Preset { std::string name; Values values=defaults(); };
const std::array<Preset,4>& factoryPresets();
bool savePreset(const std::filesystem::path& path,const Preset& preset,std::string& error);
bool loadPreset(const std::filesystem::path& path,Preset& preset,std::string& error);
}
