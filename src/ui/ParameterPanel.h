#pragma once
#include "ui/Controls.h"
#include <vector>

namespace noxis::ui {
class ParameterPanel final : public juce::Component {
public:
    ParameterPanel(ParameterStore& s,juce::String name,std::initializer_list<Param> ids,int columnCount=4):store(s){configure(std::move(name),std::vector<Param>(ids),columnCount);}
    void configure(juce::String name,std::vector<Param> ids,int columnCount=3) {
        title=std::move(name);columns=columnCount;controls.clear();
        for(Param p:ids){Entry entry;entry.id=p;const auto& info=parameterInfo[index(p)];
            if(info.integral&&info.min==0&&info.max==1){entry.button=std::make_unique<juce::TextButton>(juce::String(info.label.data()));entry.button->setClickingTogglesState(true);auto* button=entry.button.get();button->onClick=[this,p,button]{store.set(p,button->getToggleState()?1.0f:0.0f);};addAndMakeVisible(*button);}
            else if(p==Param::Lfo1Wave||p==Param::Lfo2Wave||p==Param::WaveA||p==Param::WaveB){entry.choice=std::make_unique<juce::ComboBox>();auto* choice=entry.choice.get();if(p==Param::WaveA||p==Param::WaveB)choice->addItemList({"Sine","Saw","Square","Triangle"},1);else choice->addItemList({"Sine","Triangle","Saw","Square","S & H"},1);choice->onChange=[this,p,choice]{store.set(p,static_cast<float>(choice->getSelectedId()-1));};addAndMakeVisible(*choice);}
            else {entry.knob=std::make_unique<Knob>(store,p);addAndMakeVisible(*entry.knob);}
            controls.push_back(std::move(entry));
        }sync();resized();repaint();
    }
    void sync(){for(auto& e:controls){if(e.knob)e.knob->sync();if(e.button)e.button->setToggleState(store.get(e.id)>.5f,juce::dontSendNotification);if(e.choice)e.choice->setSelectedId(static_cast<int>(store.get(e.id))+1,juce::dontSendNotification);}}
    void paint(juce::Graphics& g) override {
        auto r=getLocalBounds().toFloat().reduced(.5f);g.setColour(panelColour());g.fillRoundedRectangle(r,9);g.setColour(juce::Colour(0xff303041));g.drawRoundedRectangle(r,9,1);
        g.setColour(purple());g.fillRoundedRectangle(12,13,3,13,1);g.setColour(text());g.setFont(juce::FontOptions(12,juce::Font::bold));g.drawText(title,23,8,getWidth()-35,24,juce::Justification::left);
    }
    void resized() override {
        if(controls.empty())return;const int cols=std::max(1,std::min(columns,static_cast<int>(controls.size()))),rows=(static_cast<int>(controls.size())+cols-1)/cols;
        auto area=getLocalBounds().reduced(8).withTrimmedTop(30);const int cw=area.getWidth()/cols,ch=area.getHeight()/rows;
        for(std::size_t i=0;i<controls.size();++i){auto r=juce::Rectangle<int>(area.getX()+static_cast<int>(i)%cols*cw,area.getY()+static_cast<int>(i)/cols*ch,cw,ch).reduced(3,1);auto& e=controls[i];if(e.knob)e.knob->setBounds(r);else if(e.button)e.button->setBounds(r.reduced(2).withSizeKeepingCentre(std::max(30,r.getWidth()-4),std::min(30,r.getHeight())));else if(e.choice)e.choice->setBounds(r.withSizeKeepingCentre(std::max(30,r.getWidth()-4),28));}
    }
private:
    struct Entry {Param id{};std::unique_ptr<Knob> knob;std::unique_ptr<juce::TextButton> button;std::unique_ptr<juce::ComboBox> choice;};
    ParameterStore& store;juce::String title;int columns=4;std::vector<Entry> controls;
};
class SignalViews final : public juce::Component {
public:
    explicit SignalViews(AudioHost& h):host(h){}
    void update(){for(int c=0;c<2;++c){trace[c][cursor]=host.lfo[c].load();}cursor=(cursor+1)%trace[0].size();repaint();}
    void paint(juce::Graphics& g) override;
private:AudioHost& host;std::array<std::array<float,120>,2> trace{};std::size_t cursor=0;
};
}
