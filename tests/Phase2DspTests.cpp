#include "synth/Engine.h"
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <new>

namespace {
std::atomic<bool> allocationCheck{false};std::atomic<int> allocationCount{0};
int checks=0;
void require(bool condition,const char* message){++checks;if(!condition){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
using namespace noxis;
Values neutral(){auto p=defaults();p[index(Param::WaveA)]=0;p[index(Param::LevelA)]=.7f;p[index(Param::LevelB)]=0;p[index(Param::Cutoff)]=12000;p[index(Param::Attack)]=.002f;p[index(Param::Decay)]=.01f;p[index(Param::Sustain)]=1;p[index(Param::Release)]=.02f;return p;}
void advance(Engine& e,int samples){for(int i=0;i<samples;++i){const auto s=e.tickStereo();require(std::isfinite(s.left)&&std::isfinite(s.right)&&std::abs(s.left)<=.960001f&&std::abs(s.right)<=.960001f,"Конечный ограниченный стереосигнал");}}
double power(Engine& e,int samples,double* stereo=nullptr){double energy=0,difference=0;for(int i=0;i<samples;++i){auto s=e.tickStereo();energy+=(s.left*s.left+s.right*s.right)*.5;difference+=(s.left-s.right)*(s.left-s.right);}if(stereo)*stereo=difference/samples;return energy/samples;}
CompiledRouting planFor(const RoutingGraph& graph){CompiledRouting plan;std::string error;require(compileRouting(graph,plan,error),"Валидный граф для DSP");return plan;}
}
void* operator new(std::size_t size){if(allocationCheck.load())allocationCount.fetch_add(1);if(void* memory=std::malloc(size?size:1))return memory;throw std::bad_alloc();}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* memory) noexcept {std::free(memory);}
void operator delete[](void* memory) noexcept {std::free(memory);}
void operator delete(void* memory,std::size_t) noexcept {std::free(memory);}
void operator delete[](void* memory,std::size_t) noexcept {std::free(memory);}

int main(){
    using namespace noxis;
    auto engine=std::make_unique<Engine>();engine->prepare(48000);auto p=neutral();engine->setParameters(p,true);engine->noteOn(69,.8f);
    double difference=0;const double monoPower=power(*engine,12000,&difference);require(monoPower>1e-5&&difference<1e-12,"Один unison-голос остаётся моно");
    engine->prepare(48000);p[index(Param::UnisonVoices)]=8;p[index(Param::PhaseRandom)]=.8f;p[index(Param::UnisonDetune)]=18;engine->setParameters(p,true);engine->noteOn(69,.8f);
    const double widePower=power(*engine,16000,&difference);require(difference>1e-6,"Unison создаёт действительное стерео");require(widePower<monoPower*2.5,"Нормализация unison ограничивает уровень");
    p[index(Param::Width)]=0;engine->prepare(48000);engine->setParameters(p,true);engine->noteOn(69,.8f);power(*engine,12000,&difference);require(difference<1e-12,"Width=0 сводит unison в моно");
    p[index(Param::Width)]=1;p[index(Param::UnisonVoices)]=1;
    for(int shape=0;shape<5;++shape){Lfo lfo;lfo.reset();float previous=lfo.tick(2,48000,shape);int crossings=0;float peak=0;for(int i=0;i<48000;++i){float value=lfo.tick(2,48000,shape);require(std::isfinite(value)&&std::abs(value)<=1.00001f,"Пределы форм LFO");if(previous<=0&&value>0)++crossings;previous=value;peak=std::max(peak,std::abs(value));}require(peak>.1f,"LFO не является тишиной");if(shape<4)require(std::abs(crossings-2)<=1,"Частота LFO");}
    require(std::abs(Lfo::synchronisedRate(1,120)-2)<1e-9,"Синхронизация LFO с BPM");
    auto compare=std::make_unique<Engine>();compare->prepare(48000);engine->prepare(48000);auto mod=p;mod[index(Param::Lfo1Pitch)]=3;mod[index(Param::Lfo1Rate)]=4;engine->setParameters(mod,true);compare->setParameters(mod,true);
    auto graph=defaultRouting();for(int i=graph.count-1;i>=0;--i)if(graph.edges[i].from==9&&graph.edges[i].port==2)disconnect(graph,i);
    compare->setRouting(planFor(graph),true);engine->noteOn(69,1);compare->noteOn(69,1);double delta=0;for(int i=0;i<16000;++i){const float a=engine->tick(),b=compare->tick();delta+=(a-b)*(a-b);}require(delta>.01,"Граф действительно включает и отключает модуляцию pitch");
    engine->prepare(48000);p=neutral();p[index(Param::Mono)]=1;p[index(Param::Legato)]=1;p[index(Param::Glide)]=.1f;engine->setParameters(p,true);engine->noteOn(60,1);advance(*engine,4800);engine->noteOn(72,1);
    require(engine->activeVoices()==1,"Mono ограничивает полифонию");require(std::abs(engine->currentFrequency()-midiFrequency(60))<1,"Начало glide сохраняет прежнюю частоту");advance(*engine,2400);require(engine->currentFrequency()>midiFrequency(60)&&engine->currentFrequency()<midiFrequency(72),"Промежуточная частота glide");advance(*engine,2500);require(std::abs(engine->currentFrequency()-midiFrequency(72))<.1,"Glide достигает ноты");
    engine->noteOff(72);advance(*engine,4900);require(std::abs(engine->currentFrequency()-midiFrequency(60))<.1&&engine->activeVoices()==1,"Mono возвращает удерживаемую ноту");
    engine->sustain(true);engine->noteOff(60);advance(*engine,2400);require(engine->activeVoices()==1,"Mono sustain удерживает ноту");engine->sustain(false);advance(*engine,2400);require(engine->activeVoices()==0,"Mono sustain отпускает последнюю ноту");
    engine->prepare(48000);p=neutral();p[index(Param::BendRange)]=12;engine->setParameters(p,true);engine->noteOn(69,1,2);engine->pitchBend(1,1);advance(*engine,200);require(std::abs(engine->currentFrequency()-440)<.1,"Pitch bend изолирован по MIDI-каналу");engine->pitchBend(1,2);advance(*engine,200);require(std::abs(engine->currentFrequency()-880)<.1,"Pitch bend на октаву");engine->pitchBend(-1,2);advance(*engine,200);require(std::abs(engine->currentFrequency()-220)<.1,"Отрицательный pitch bend");
    StereoDelay delay;delay.prepare(48000);float firstEcho=0,secondRight=0;for(int i=0;i<4000;++i){const auto s=delay.process(i==0?StereoFrame{1,0}:StereoFrame{},.02f,.5f,1,true);if(i==960)firstEcho=s.left;if(i==1920)secondRight=s.right;}require(firstEcho>.99f&&secondRight>.49f,"Delay time и ping-pong feedback");
    delay.reset();const auto bypassDelay=delay.process({.3f,-.2f},.1f,.8f,0,true);require(bypassDelay.left==.3f&&bypassDelay.right==-.2f,"Delay mix=0 пропускает сухой сигнал");
    StereoChorus chorus;chorus.prepare(48000);double chorusEnergy=0,chorusStereo=0;for(int i=0;i<6000;++i){const auto s=chorus.process(i==0?StereoFrame{1,1}:StereoFrame{},.7f,.8f,1);chorusEnergy+=s.left*s.left+s.right*s.right;chorusStereo+=(s.left-s.right)*(s.left-s.right);}require(chorusEnergy>.01&&chorusStereo>.01,"Chorus создаёт задержанный стереосигнал");
    StereoReverb reverb;reverb.prepare(48000);double early=0,late=0;for(int i=0;i<48000;++i){const auto s=reverb.process(i==0?StereoFrame{1,0}:StereoFrame{},.85f,.3f,1);require(std::isfinite(s.left)&&std::isfinite(s.right),"Reverb finite");if(i<12000)early+=s.left*s.left+s.right*s.right;else late+=s.left*s.left+s.right*s.right;}require(early>.001&&late>.000001,"Reverb создаёт настоящий хвост");
    auto before=defaultRouting(),after=defaultRouting();for(int i=after.count-1;i>=0;--i)if(after.edges[i].from==2||after.edges[i].from==3||after.edges[i].from==4)disconnect(after,i);
    after.edges[after.count++]={2,4,0};after.edges[after.count++]={4,3,0};after.edges[after.count++]={3,5,0};
    engine->prepare(48000);compare->prepare(48000);p=neutral();p[index(Param::WaveA)]=1;p[index(Param::Cutoff)]=700;p[index(Param::Drive)]=20;p[index(Param::LevelA)]=1;
    engine->setParameters(p,true);compare->setParameters(p,true);engine->setRouting(planFor(before),true);compare->setRouting(planFor(after),true);engine->noteOn(48,1);compare->noteOn(48,1);delta=0;for(int i=0;i<12000;++i){float a=engine->tick(),b=compare->tick();delta+=(a-b)*(a-b);}require(delta>.001,"Порядок Filter/Saturation меняет звучание");
    p[index(Param::UnisonVoices)]=8;p[index(Param::ReverbEnabled)]=1;p[index(Param::DelayEnabled)]=1;p[index(Param::ChorusEnabled)]=1;engine->setParameters(p);
    const auto beforePlan=planFor(before),afterPlan=planFor(after);allocationCount.store(0);allocationCheck.store(true);
    for(int block=0;block<40;++block){if(block%5==0){engine->noteOn(36+block,.9f,block%16+1);engine->pitchBend(.5f,block%16+1);engine->setRouting(block%10?beforePlan:afterPlan);}if(block==20)engine->allNotesOff();engine->setParameters(p);for(int i=0;i<128;++i)engine->tickStereo();}
    allocationCheck.store(false);require(allocationCount.load()==0,"Штатная обработка DSP не выделяет память");
    engine->allNotesOff();advance(*engine,2000);require(power(*engine,4000)<1e-15,"Panic глушит хвосты эффектов");engine->noteOn(60,1);require(power(*engine,6000)>1e-8,"После Panic новые ноты снова звучат");
    // Контролируемые крайние значения не должны разрушать состояния фильтров и обратной связи.
    for(double rate:{22050.,48000.,96000.}){engine->prepare(rate);p=defaults();for(std::size_t i=0;i<p.size();++i)p[i]=parameterInfo[i].max;p[index(Param::UnisonVoices)]=8;p[index(Param::Mono)]=0;engine->setParameters(p,true);for(int note=60;note<68;++note)engine->noteOn(note,1);advance(*engine,6000);engine->allNotesOff();advance(*engine,1000);require(engine->activeVoices()==0,"Panic глушит голоса при предельных параметрах");}
    std::cout<<"PASS: Phase 2 DSP, unison, LFO, mono/glide, bend, effects, routing, allocation-free ("<<checks<<" checks)\n";
}
