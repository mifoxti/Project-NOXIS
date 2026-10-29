#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

namespace noxis {
enum class Node : int { OscA,OscB,Mixer,Filter,Saturation,Chorus,Delay,Reverb,Output,Lfo1,Lfo2,Envelope,Count };
inline constexpr int nodeCount=static_cast<int>(Node::Count),audioNodeCount=9,maxEdges=32;
inline constexpr std::array<std::string_view,nodeCount> nodeNames{"OSC A","OSC B","MIXER","FILTER","SATURATION","CHORUS","DELAY","REVERB","OUTPUT","LFO 1","LFO 2","ENVELOPE"};
enum class Port : int { Audio=0,AudioB=1,Pitch=2,Cutoff=3,Level=4,Width=5 };
struct Connection {int from=0,to=0,port=0;bool operator==(const Connection&) const=default;};
struct RoutingGraph {
    std::array<Connection,maxEdges> edges{};int count=0;
    std::array<float,nodeCount> x{},y{};
    std::array<bool,nodeCount> present{};
    bool operator==(const RoutingGraph&) const=default;
};
struct CompiledRouting {
    std::array<int,audioNodeCount> order{};
    std::array<std::array<int,2>,audioNodeCount> inputs{};
    std::array<int,3> modulationMask{}; // Pitch=1, Cutoff=2, Level=4, Width=8.
    bool operator==(const CompiledRouting&) const=default;
};
inline bool compatible(int from,int to,int port) noexcept {
    if(from<0||from>=nodeCount||to<0||to>=nodeCount||from==to)return false;
    if(from>=audioNodeCount) {
        if(from==static_cast<int>(Node::Envelope))return to==3&&port==3;
        return (to==2&&(port==2||port==4||port==5))||(to==3&&port==3);
    }
    if(from==8||to<2||to>=audioNodeCount)return false;
    return port==0||(to==2&&port==1);
}
inline bool compileRouting(const RoutingGraph& graph,CompiledRouting& result,std::string& error) {
    if(graph.count<0||graph.count>maxEdges||!graph.present[8]){error="В схеме отсутствует Output или превышен лимит связей";return false;}
    CompiledRouting plan;for(auto& i:plan.inputs)i.fill(-1);
    std::array<int,audioNodeCount> degrees{};
    for(int i=0;i<nodeCount;++i)if(!std::isfinite(graph.x[i])||!std::isfinite(graph.y[i])||std::abs(graph.x[i])>10000||std::abs(graph.y[i])>10000){error="Некорректное положение нода";return false;}
    for(int i=0;i<graph.count;++i) {
        const auto e=graph.edges[i];if(!compatible(e.from,e.to,e.port)||!graph.present[e.from]||!graph.present[e.to]){error="Несовместимые порты или отсутствующий нод";return false;}
        for(int j=0;j<i;++j)if(graph.edges[j]==e){error="Повторяющееся соединение";return false;}
        if(e.from>=audioNodeCount)continue;
        if(plan.inputs[e.to][e.port]!=-1){error="Вход уже занят";return false;}
        plan.inputs[e.to][e.port]=e.from;++degrees[e.to];
    }
    std::array<int,audioNodeCount> queue{};int read=0,write=0,done=0;
    for(int i=0;i<audioNodeCount;++i)if(degrees[i]==0)queue[write++]=i;
    while(read<write){const int node=queue[read++];plan.order[done++]=node;for(int i=0;i<graph.count;++i){auto e=graph.edges[i];if(e.from==node&&e.from<audioNodeCount&&--degrees[e.to]==0)queue[write++]=e.to;}}
    if(done!=audioNodeCount){error="Аудиоциклы запрещены";return false;}
    std::array<bool,audioNodeCount> signal{};signal[0]=graph.present[0];signal[1]=graph.present[1];
    for(int node:plan.order)for(int input:plan.inputs[node])if(input>=0)signal[node]=signal[node]||signal[input];
    if(!signal[8]){error="Нет пути от осциллятора к Output";return false;}
    std::array<bool,audioNodeCount> audible{};audible[8]=true;
    for(int i=audioNodeCount-1;i>=0;--i){const int node=plan.order[i];if(audible[node])for(int input:plan.inputs[node])if(input>=0)audible[input]=true;}
    for(int i=0;i<graph.count;++i){const auto e=graph.edges[i];if(e.from>=audioNodeCount&&audible[e.to])plan.modulationMask[e.from-audioNodeCount]|=1<<(e.port-2);}
    result=plan;error.clear();return true;
}
inline RoutingGraph defaultRouting() {
    RoutingGraph g;g.present.fill(true);
    const std::array<Connection,17> edges{{{0,2,0},{1,2,1},{2,3,0},{3,4,0},{4,5,0},{5,6,0},{6,7,0},{7,8,0},
        {9,2,2},{9,3,3},{9,2,4},{9,2,5},{10,2,2},{10,3,3},{10,2,4},{10,2,5},{11,3,3}}};
    for(auto e:edges)g.edges[g.count++]=e;
    g.x={20,20,190,360,530,700,870,1040,1210,190,530,870};
    g.y={50,200,120,120,120,120,120,120,120,330,330,330};return g;
}
inline void disconnect(RoutingGraph& g,int edge) {if(edge<0||edge>=g.count)return;for(int i=edge;i<g.count-1;++i)g.edges[i]=g.edges[i+1];g.edges[--g.count]={};}
inline bool connect(RoutingGraph& graph,Connection e,std::string& error) {
    if(!compatible(e.from,e.to,e.port)){error="Несовместимые порты";return false;}
    auto candidate=graph;
    for(int i=candidate.count-1;i>=0;--i)if(candidate.edges[i].to==e.to&&candidate.edges[i].port==e.port&&candidate.edges[i].from<audioNodeCount)disconnect(candidate,i);
    if(candidate.count>=maxEdges){error="Достигнут лимит соединений";return false;}
    candidate.edges[candidate.count++]=e;CompiledRouting plan;if(!compileRouting(candidate,plan,error))return false;graph=candidate;return true;
}
}
