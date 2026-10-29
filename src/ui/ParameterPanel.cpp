#include "ui/ParameterPanel.h"
#include <complex>

namespace noxis::ui {
void SignalViews::paint(juce::Graphics& g){
    const auto v=host.parameters.snapshot();auto get=[&](Param p){return v[index(p)];};auto all=getLocalBounds().toFloat();
    auto frame=[&](juce::Rectangle<float> r,const juce::String& title){g.setColour(panelColour());g.fillRoundedRectangle(r,9);g.setColour(muted());g.setFont(juce::FontOptions(10,juce::Font::bold));g.drawText(title,r.reduced(12,8).removeFromTop(16),juce::Justification::left);return r.reduced(14,12).withTrimmedTop(22);};
    const float h=(all.getHeight()-16)/3;
    auto env=frame(all.removeFromTop(h),utf8("ADSR · амплитуда"));all.removeFromTop(8);
    const float a=get(Param::Attack),d=get(Param::Decay),s=get(Param::Sustain),r=get(Param::Release),sum=a+d+r+.4f;
    juce::Path envelope;envelope.startNewSubPath(env.getX(),env.getBottom());envelope.lineTo(env.getX()+env.getWidth()*a/sum,env.getY());envelope.lineTo(env.getX()+env.getWidth()*(a+d)/sum,env.getBottom()-s*env.getHeight());envelope.lineTo(env.getRight()-env.getWidth()*r/sum,env.getBottom()-s*env.getHeight());envelope.lineTo(env.getRight(),env.getBottom());
    g.setColour(pink());g.strokePath(envelope,juce::PathStrokeType(1.5f));g.setColour(pink().withAlpha(.3f));const float activity=host.envelopeLevel.load();g.drawHorizontalLine(static_cast<int>(env.getBottom()-activity*env.getHeight()),env.getX(),env.getRight());
    auto filter=frame(all.removeFromTop(h),utf8("FILTER · базовая характеристика"));all.removeFromTop(8);juce::Path response;
    const double sr=host.currentRate(),cut=std::min(static_cast<double>(get(Param::Cutoff)),sr*.45),k=2-1.85*get(Param::Resonance);
    for(int i=0;i<160;++i){const double hz=25*std::pow(std::min(20000.,sr*.45)/25,i/159.0),w=std::tan(pi*hz/sr)/std::tan(pi*cut/sr);const std::complex<double> denominator(1-w*w,k*w);const double db=std::clamp(-20*std::log10(std::max(1.e-9,std::abs(denominator))),-48.0,12.0);const float x=filter.getX()+filter.getWidth()*i/159.0f,y=filter.getY()+filter.getHeight()*static_cast<float>((12-db)/60);if(i==0)response.startNewSubPath(x,y);else response.lineTo(x,y);}
    g.setColour(purple());g.strokePath(response,juce::PathStrokeType(1.5f));
    auto lfo=frame(all,utf8("LFO 1 / 2 · фактическая активность"));g.setColour(surface());g.drawHorizontalLine(static_cast<int>(lfo.getCentreY()),lfo.getX(),lfo.getRight());
    for(int c=0;c<2;++c){juce::Path path;for(std::size_t i=0;i<trace[c].size();++i){float x=lfo.getX()+lfo.getWidth()*static_cast<float>(i)/119,y=lfo.getCentreY()-trace[c][(cursor+i)%trace[c].size()]*lfo.getHeight()*.43f;if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}g.setColour(c==0?purple():pink().withAlpha(.75f));g.strokePath(path,juce::PathStrokeType(1.25f));}
}
}
