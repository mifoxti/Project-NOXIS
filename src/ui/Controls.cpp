#include "ui/Controls.h"

namespace noxis::ui {
NoxisLookAndFeel::NoxisLookAndFeel() {
    setColour(juce::ResizableWindow::backgroundColourId,background());
    setColour(juce::Slider::textBoxTextColourId,text());setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);setColour(juce::Slider::thumbColourId,purple());
    setColour(juce::ComboBox::backgroundColourId,surface());setColour(juce::ComboBox::textColourId,text());setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff39364c));
    setColour(juce::PopupMenu::backgroundColourId,panelColour());setColour(juce::PopupMenu::textColourId,text());setColour(juce::PopupMenu::highlightedBackgroundColourId,surface());
    setColour(juce::TextButton::textColourOffId,muted());setColour(juce::TextButton::textColourOnId,purple());setColour(juce::Label::textColourId,text());
    setColour(juce::TextEditor::backgroundColourId,surface());setColour(juce::TextEditor::textColourId,text());setColour(juce::TextEditor::highlightColourId,purple().withAlpha(.3f));
}
void NoxisLookAndFeel::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float position,float start,float end,juce::Slider&) {
    auto area=juce::Rectangle<float>(static_cast<float>(x),static_cast<float>(y),static_cast<float>(w),static_cast<float>(h)).reduced(8);
    const float radius=std::min(area.getWidth(),area.getHeight())*.5f,cx=area.getCentreX(),cy=area.getCentreY();
    juce::Path track;track.addCentredArc(cx,cy,radius,radius,0,start,end,true);g.setColour(surface().brighter(.12f));g.strokePath(track,juce::PathStrokeType(3));
    juce::Path active;active.addCentredArc(cx,cy,radius,radius,0,start,start+position*(end-start),true);g.setColour(purple());g.strokePath(active,juce::PathStrokeType(3,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    g.setColour(juce::Colour(0xff292939));g.fillEllipse(cx-radius+6,cy-radius+6,(radius-6)*2,(radius-6)*2);
    const float angle=start+position*(end-start);g.setColour(text());g.drawLine(cx+std::sin(angle)*(radius*.42f),cy-std::cos(angle)*(radius*.42f),cx+std::sin(angle)*(radius*.72f),cy-std::cos(angle)*(radius*.72f),2);
}
void NoxisLookAndFeel::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float,float,const juce::Slider::SliderStyle style,juce::Slider&) {
    const bool vertical=style==juce::Slider::LinearVertical;
    g.setColour(surface());if(vertical)g.fillRoundedRectangle(static_cast<float>(x+w/2-3),static_cast<float>(y),6,static_cast<float>(h),3);else g.fillRoundedRectangle(static_cast<float>(x),static_cast<float>(y+h/2-3),static_cast<float>(w),6,3);
    g.setColour(purple());if(vertical){g.fillRoundedRectangle(static_cast<float>(x+w/2-3),pos,6,static_cast<float>(y+h)-pos,3);g.fillRoundedRectangle(static_cast<float>(x+w/2-10),pos-4,20,8,3);}else{g.fillRoundedRectangle(static_cast<float>(x),static_cast<float>(y+h/2-3),pos-static_cast<float>(x),6,3);g.fillEllipse(pos-5,static_cast<float>(y+h/2-5),10,10);}
}
void NoxisLookAndFeel::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool down) {
    auto r=b.getLocalBounds().toFloat().reduced(.5f);g.setColour(b.getToggleState()?purple().withAlpha(.13f):(over||down?surface().brighter(.08f):surface()));g.fillRoundedRectangle(r,6);
    g.setColour(b.getToggleState()?purple().withAlpha(.6f):juce::Colour(0xff363448));g.drawRoundedRectangle(r,6,1);
}
Knob::Knob(ParameterStore& s,Param p,const juce::String& title):store(s),parameter(p),label(title.isEmpty()?juce::String(parameterInfo[index(p)].label.data()):title) {
    const auto& d=parameterInfo[index(p)];addAndMakeVisible(slider);addAndMakeVisible(label);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,90,21);
    slider.setRange(d.min,d.max,d.integral?1:0);slider.setDoubleClickReturnValue(true,d.initial);
    if(p==Param::Cutoff)slider.setSkewFactorFromMidPoint(1800);else if(p==Param::Attack||p==Param::Decay||p==Param::Release)slider.setSkewFactorFromMidPoint(.4);
    slider.textFromValueFunction=[p](double value){const auto& info=parameterInfo[index(p)];if(info.unit=="%")return juce::String(value*100,0)+" %";return juce::String(value,info.integral||info.unit=="Hz"?0:(info.unit=="s"?3:1))+" "+juce::String(info.unit.data());};
    slider.valueFromTextFunction=[p](const juce::String& value){const double number=value.getDoubleValue();return parameterInfo[index(p)].unit=="%"?number/100:number;};
    slider.onValueChange=[this]{store.set(parameter,static_cast<float>(slider.getValue()));};
    slider.setColour(juce::Slider::textBoxTextColourId,text());slider.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);slider.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);slider.updateText();
    slider.setTooltip(utf8("Перетаскивайте; Shift — точно; двойной щелчок — сброс. Значение можно ввести с клавиатуры."));sync();
}
void Knob::resized(){auto r=getLocalBounds();label.setBounds(r.removeFromTop(19));slider.setBounds(r);}
void Knob::sync(){if(!slider.isMouseButtonDown())slider.setValue(store.get(parameter),juce::dontSendNotification);}
WaveformSelector::WaveformSelector(ParameterStore& s,Param p):store(s),parameter(p) {
    const std::array<juce::String,4> names{"SINE","SAW","SQR","TRI"};
    for(std::size_t i=0;i<buttons.size();++i){auto& b=buttons[i];b.setButtonText(names[i]);addAndMakeVisible(b);b.onClick=[this,i]{store.set(parameter,static_cast<float>(i));sync();};}sync();
}
void WaveformSelector::resized(){auto r=getLocalBounds();const int w=r.getWidth()/4;for(auto& b:buttons)b.setBounds(r.removeFromLeft(w).reduced(2,0));}
void WaveformSelector::sync(){for(std::size_t i=0;i<buttons.size();++i)buttons[i].setToggleState(static_cast<int>(store.get(parameter))==static_cast<int>(i),juce::dontSendNotification);}
void Panel::paint(juce::Graphics& g){auto r=getLocalBounds().toFloat().reduced(.5f);g.setColour(panelColour());g.fillRoundedRectangle(r,10);g.setColour(juce::Colour(0xff303041));g.drawRoundedRectangle(r,10,1);g.setColour(text());g.setFont(juce::FontOptions(14,juce::Font::bold));g.drawText(name,18,12,getWidth()-36,22,juce::Justification::left);g.setColour(muted());g.setFont(juce::FontOptions(11));g.drawText(detail,18,35,getWidth()-36,16,juce::Justification::left);}
TabBar::TabBar(){const std::array<juce::String,4> names{"SYNTH","ROUTING","FX","PRESETS"};for(std::size_t i=0;i<buttons.size();++i){auto& b=buttons[i];addAndMakeVisible(b);b.setButtonText(names[i]);b.onClick=[this,i]{select(static_cast<int>(i));if(onSelect)onSelect(static_cast<int>(i));};}select(0);}
void TabBar::resized(){auto r=getLocalBounds();for(auto& b:buttons)b.setBounds(r.removeFromLeft(112).reduced(0,3).withTrimmedRight(8));}
void TabBar::select(int tab){for(std::size_t i=0;i<buttons.size();++i)buttons[i].setToggleState(static_cast<int>(i)==tab,juce::dontSendNotification);}
PresetSelector::PresetSelector(){int id=1;for(const auto& p:factoryPresets())addItem(p.name,id++);setSelectedId(1,juce::dontSendNotification);}
void AudioMeter::setLevel(float p,float r){maximum=std::max(p,maximum*.88f);average=r;repaint();}
void AudioMeter::paint(juce::Graphics& g){auto r=getLocalBounds().toFloat();g.setColour(surface());g.fillRoundedRectangle(r,3);auto normal=[](float v){return juce::jlimit(0.0f,1.0f,(20*std::log10(std::max(v,.00001f))+60)/60);};g.setColour(purple().withAlpha(.6f));g.fillRoundedRectangle(r.withWidth(r.getWidth()*normal(average)),3);g.setColour(maximum>.85f?pink():purple());float x=r.getWidth()*normal(maximum);g.fillRect(std::max(0.0f,x-2),0.0f,2.0f,r.getHeight());}
void Scope::update(){float sample=0;int count=0;while(count++<8192&&buffer.pop(sample)){history[cursor]=sample;cursor=(cursor+1)%history.size();}amplitude=0;for(float s:history)amplitude=std::max(amplitude,std::abs(s));repaint();}
void Scope::paint(juce::Graphics& g){auto r=getLocalBounds().toFloat().reduced(12);g.setColour(surface().withAlpha(.45f));for(int i=1;i<4;++i)g.drawHorizontalLine(static_cast<int>(r.getY()+r.getHeight()*i/4),r.getX(),r.getRight());g.setColour(muted().withAlpha(.25f));g.drawHorizontalLine(static_cast<int>(r.getCentreY()),r.getX(),r.getRight());
    // Trigger on a real rising zero crossing; a quiet input remains a flat line.
    std::size_t start=cursor;for(std::size_t i=0;i<512;++i){auto a=(cursor+i)%history.size(),b=(a+1)%history.size();if(history[a]<=0&&history[b]>0){start=b;break;}}
    const float gain=amplitude>.0001f?std::min(8.0f,.85f/amplitude):1;
    juce::Path path;for(std::size_t i=0;i<512;++i){float x=r.getX()+r.getWidth()*static_cast<float>(i)/511;float y=r.getCentreY()-history[(start+i)%history.size()]*gain*r.getHeight()*.45f;if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}g.setColour(purple());g.strokePath(path,juce::PathStrokeType(1.6f));
}
}
