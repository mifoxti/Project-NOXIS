#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace noxis {
inline bool writeWav(const std::filesystem::path& path,const std::vector<float>& samples,int rate,int channels=1) {
    if((channels!=1&&channels!=2)||samples.size()%static_cast<std::size_t>(channels)!=0)return false;
    std::ofstream file(path,std::ios::binary);if(!file)return false;
    auto u16=[&](std::uint16_t n){file.put(static_cast<char>(n&255));file.put(static_cast<char>((n>>8)&255));};
    auto u32=[&](std::uint32_t n){u16(static_cast<std::uint16_t>(n&65535));u16(static_cast<std::uint16_t>(n>>16));};
    const auto bytes=static_cast<std::uint32_t>(samples.size()*2);
    file.write("RIFF",4);u32(36+bytes);file.write("WAVEfmt ",8);u32(16);u16(1);u16(static_cast<std::uint16_t>(channels));
    u32(static_cast<std::uint32_t>(rate));u32(static_cast<std::uint32_t>(rate*2*channels));u16(static_cast<std::uint16_t>(2*channels));u16(16);
    file.write("data",4);u32(bytes);
    for(float s:samples) { if(!std::isfinite(s))return false;const auto pcm=static_cast<std::int16_t>(std::round(std::clamp(s,-1.0f,1.0f)*32767));u16(static_cast<std::uint16_t>(pcm)); }
    file.flush();return static_cast<bool>(file);
}
}
