#include "state/Preset.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <thread>

void require(bool value,const char* message){if(!value){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
int main(){using namespace noxis;std::string error;auto graph=defaultRouting();CompiledRouting plan;
    require(compileRouting(graph,plan,error),"default graph");require(plan.inputs[3][0]==2&&plan.inputs[4][0]==3,"compiled order");
    auto altered=graph;require(connect(altered,{2,4,0},error),"reconnect mixer to saturation");
    for(int i=altered.count-1;i>=0;--i)if(altered.edges[i].from<audioNodeCount&&(altered.edges[i].to==3||altered.edges[i].to==5))disconnect(altered,i);
    altered.edges[altered.count++]={4,3,0};require(connect(altered,{3,5,0},error),"filter to chorus");require(compileRouting(altered,plan,error),"reordered graph valid");require(plan.inputs[3][0]==4,"saturation before filter");
    auto cyclic=graph;cyclic.edges[cyclic.count++]={7,2,1};require(!compileRouting(cyclic,plan,error),"reject duplicate occupied input");
    cyclic=graph;disconnect(cyclic,1);cyclic.edges[cyclic.count++]={7,2,1};require(!compileRouting(cyclic,plan,error),"reject cycle");
    auto bad=graph;bad.present[8]=false;require(!compileRouting(bad,plan,error),"mandatory output");bad=graph;bad.edges[0]={9,3,0};require(!compileRouting(bad,plan,error),"port types");
    bad=graph;disconnect(bad,7);require(!compileRouting(bad,plan,error),"output disconnected");
    ParameterStore store;const auto before=store.snapshot();require(!store.replacePatch(defaults(),bad,error),"atomic invalid patch");require(store.snapshot()==before&&store.graph()==graph,"invalid patch unchanged");
    std::atomic<bool> done{false};std::thread writer([&]{for(int i=0;i<5000;++i){auto v=defaults();v[index(Param::DetuneA)]=i%2?10:-10;store.replacePatch(v,i%2?graph:altered,error);}done.store(true);});
    RealtimeState state;while(!done.load()){if(store.read(state)){const float v=state.values[index(Param::DetuneA)];if(v==10)require(state.routing.inputs[3][0]==2,"coherent graph A");if(v==-10)require(state.routing.inputs[3][0]==4,"coherent graph B");}}writer.join();
    Preset legacy;require(loadPreset("PRESET_PATH",legacy,error)==false,"missing path handled");
    const auto path=std::filesystem::temp_directory_path()/"noxis-v1-migration.noxis";
    {std::ofstream out(path);out<<"NOXIS_PRESET 1\nLegacy\n";auto d=defaults();for(std::size_t i=0;i<17;++i)out<<parameterInfo[i].id<<' '<<d[i]<<'\n';}
    require(loadPreset(path,legacy,error),"version 1 migration");require(legacy.values[index(Param::UnisonVoices)]==1&&legacy.routing==graph,"legacy new defaults");
    require(savePreset(path,legacy,error),"version 2 save");Preset roundtrip;require(loadPreset(path,roundtrip,error)&&roundtrip.values==legacy.values&&roundtrip.routing==graph,"version 2 round trip");std::filesystem::remove(path);
    std::cout<<"PASS: graph validation, reconnect, cycles, atomic snapshots, preset v1/v2\n";
}
