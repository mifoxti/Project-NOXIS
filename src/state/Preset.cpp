#include "state/Preset.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_set>

namespace noxis {
namespace {
bool validText(const std::string& value,std::size_t limit,bool required=false) {
    return (!required||!value.empty())&&value.size()<=limit&&value.find_first_of("\r\n\0",0,3)==std::string::npos;
}
}
bool savePreset(const std::filesystem::path& path,const Preset& p,std::string& error) {
    CompiledRouting plan;
    if(!validText(p.name,120,true)||!validText(p.category,32,true)||!validText(p.description,2048)||!validText(p.tags,512)||!validText(p.character,512)||p.lowNote<0||p.highNote>127||p.lowNote>p.highNote) {error="Некорректные метаданные пресета";return false;}
    if(!compileRouting(p.routing,plan,error))return false;
    std::ofstream out(path,std::ios::binary|std::ios::trunc);
    if(!out){error="Не удалось открыть файл для записи";return false;}
    out<<"NOXIS_PRESET 2\n"<<p.name<<'\n'<<std::setprecision(9);
    out<<"@category "<<std::quoted(p.category)<<"\n@description "<<std::quoted(p.description)<<"\n@tags "<<std::quoted(p.tags)<<"\n@character "<<std::quoted(p.character)<<"\n@range "<<p.lowNote<<' '<<p.highNote<<'\n';
    for(std::size_t i=0;i<p.values.size();++i)out<<parameterInfo[i].id<<' '<<sanitise(static_cast<Param>(i),p.values[i])<<'\n';
    for(int n=0;n<nodeCount;++n)out<<"@node "<<n<<' '<<(p.routing.present[n]?1:0)<<' '<<p.routing.x[n]<<' '<<p.routing.y[n]<<'\n';
    for(int i=0;i<p.routing.count;++i){auto e=p.routing.edges[i];out<<"@edge "<<e.from<<' '<<e.to<<' '<<e.port<<'\n';}
    out.flush();if(!out){error="Ошибка записи пресета";return false;}error.clear();return true;
}
bool loadPreset(const std::filesystem::path& path,Preset& p,std::string& error) {
    std::error_code ec;const auto size=std::filesystem::file_size(path,ec);
    if(ec||size>131072){error="Файл недоступен или слишком велик";return false;}
    std::ifstream in(path,std::ios::binary);std::string line;
    auto trim=[](std::string& s){if(!s.empty()&&s.back()=='\r')s.pop_back();};
    std::getline(in,line);trim(line);const bool legacy=line=="NOXIS_PRESET 1";
    if(!legacy&&line!="NOXIS_PRESET 2"){error="Неподдерживаемый формат пресета";return false;}
    Preset candidate;std::getline(in,candidate.name);trim(candidate.name);
    if(!validText(candidate.name,120,true)){error="Некорректное имя пресета";return false;}
    if(!legacy){candidate.routing={};}
    std::unordered_set<std::string> seen,metadata;std::array<bool,nodeCount> nodes{};
    while(std::getline(in,line)) {
        trim(line);if(line.empty())continue;
        std::istringstream row(line);std::string key,extra;row>>key;
        auto malformed=[&]{error="Повреждённые параметры пресета: "+key;return false;};
        if(!legacy&&key.starts_with('@')) {
            if(key=="@node") {
                int n,present;float x,y;if(!(row>>n>>present>>x>>y)||(row>>extra)||n<0||n>=nodeCount||nodes[n]||(present!=0&&present!=1))return malformed();
                nodes[n]=true;candidate.routing.present[n]=present!=0;candidate.routing.x[n]=x;candidate.routing.y[n]=y;
            }else if(key=="@edge") {
                Connection edge;if(!(row>>edge.from>>edge.to>>edge.port)||(row>>extra)||candidate.routing.count>=maxEdges)return malformed();candidate.routing.edges[candidate.routing.count++]=edge;
            }else {
                if(!metadata.insert(key).second)return malformed();
                if(key=="@range"){if(!(row>>candidate.lowNote>>candidate.highNote)||(row>>extra)||candidate.lowNote<0||candidate.highNote>127||candidate.lowNote>candidate.highNote)return malformed();}
                else {std::string value;if(!(row>>std::quoted(value))||(row>>extra)||!validText(value,2048))return malformed();
                    if(key=="@category"){if(!validText(value,32,true))return malformed();candidate.category=value;}
                    else if(key=="@description")candidate.description=value;
                    else if(key=="@tags"){if(value.size()>512)return malformed();candidate.tags=value;}
                    else if(key=="@character"){if(value.size()>512)return malformed();candidate.character=value;}
                    else return malformed();
                }
            }
            continue;
        }
        float value;if(!(row>>value)||(row>>extra)||!std::isfinite(value)||!seen.insert(key).second)return malformed();
        bool found=false;const std::size_t limit=legacy?17:parameterInfo.size();
        for(std::size_t i=0;i<limit;++i)if(key==parameterInfo[i].id){const auto& d=parameterInfo[i];if(value<d.min||value>d.max||(d.integral&&value!=std::round(value)))return malformed();candidate.values[i]=value;found=true;break;}
        if(!found)return malformed();
    }
    if(seen.size()!=(legacy?17:parameterInfo.size())){error="Неполный пресет";return false;}
    if(!legacy&&(metadata.size()!=5||std::any_of(nodes.begin(),nodes.end(),[](bool value){return !value;}))){error="Неполные метаданные или схема Routing";return false;}
    CompiledRouting plan;if(!compileRouting(candidate.routing,plan,error))return false;
    p=std::move(candidate);error.clear();return true;
}
}
