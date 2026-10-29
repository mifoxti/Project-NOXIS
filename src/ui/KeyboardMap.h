#pragma once
#include <array>
#include <algorithm>

namespace noxis {
struct MusicalKey { int key, scan, offset; };
// FL Studio labels MIDI 60 as C5. Upper Q row: C5; lower Z row: C4.
inline constexpr std::array<MusicalKey,25> musicalKeys{{
    {'Q',0x10,0},{'2',0x03,1},{'W',0x11,2},{'3',0x04,3},{'E',0x12,4},{'R',0x13,5},
    {'5',0x06,6},{'T',0x14,7},{'6',0x07,8},{'Y',0x15,9},{'7',0x08,10},{'U',0x16,11},{'I',0x17,12},
    {'Z',0x2c,-12},{'S',0x1f,-11},{'X',0x2d,-10},{'D',0x20,-9},{'C',0x2e,-8},{'V',0x2f,-7},
    {'G',0x22,-6},{'B',0x30,-5},{'H',0x23,-4},{'N',0x31,-3},{'J',0x24,-2},{'M',0x32,-1}
}};
class KeyboardTracker {
public:
    template<typename On,typename Off> void update(const std::array<bool,musicalKeys.size()>& keys,int root,bool enabled,bool editing,bool focused,On on,Off off) {
        for(std::size_t i=0;i<keys.size();++i) {
            const bool down=keys[i]&&enabled&&focused&&!editing;
            if(down==held[i])continue;
            held[i]=down;
            if(down){notes[i]=std::clamp(root+musicalKeys[i].offset,0,127);on(notes[i]);}else off(notes[i]);
        }
    }
    void reset() noexcept {held.fill(false);}
private:std::array<bool,musicalKeys.size()> held{};std::array<int,musicalKeys.size()> notes{};
};
}
