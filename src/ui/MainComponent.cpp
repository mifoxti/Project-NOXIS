#include "ui/MainComponent.h"

namespace noxis::ui {
Knob& MainComponent::addKnob(juce::Component& parent,Param p,const juce::String& title){auto k=std::make_unique<Knob>(host.parameters,p,title);parent.addAndMakeVisible(*k);knobs.push_back(std::move(k));return *knobs.back();}
MainComponent::MainComponent() {
    setLookAndFeel(&look);setWantsKeyboardFocus(true);master.slider.setSliderStyle(juce::Slider::LinearHorizontal);
    for(juce::Component* c:std::initializer_list<juce::Component*>{&presets,&previous,&next,&save,&load,&noob,&pro,&settings,&master,&tabs,&keyboard,&panic,&octDown,&octUp,&meter,&status,&octave,&voiceCount,&levelText,&synthView,&routeView,&fxView,&presetView})addAndMakeVisible(c);
    for(auto* c:{&oscA,&oscB,&filter,&envelope,&saturation,&scopePanel,&macros})synthView.addAndMakeVisible(c);
    oscA.addAndMakeVisible(waveA);oscB.addAndMakeVisible(waveB);scopePanel.addAndMakeVisible(scope);
    const std::array<Param,3> a{Param::OctaveA,Param::DetuneA,Param::LevelA},b{Param::OctaveB,Param::DetuneB,Param::LevelB};
    for(std::size_t i=0;i<3;++i){oscAKnobs[i]=&addKnob(oscA,a[i]);oscBKnobs[i]=&addKnob(oscB,b[i]);}
    filterKnobs={&addKnob(filter,Param::Cutoff),&addKnob(filter,Param::Resonance)};
    envKnobs={&addKnob(envelope,Param::Attack),&addKnob(envelope,Param::Decay),&addKnob(envelope,Param::Sustain),&addKnob(envelope,Param::Release)};
    satKnobs={&addKnob(saturation,Param::Drive),&addKnob(saturation,Param::Output)};
    fxKnobs={&addKnob(fxView,Param::Drive),&addKnob(fxView,Param::Output)};
    tone=&addKnob(macros,Param::Cutoff,"Tone");drive=&addKnob(macros,Param::Drive,"Drive");
    macros.addAndMakeVisible(movement);macros.addAndMakeVisible(movementLabel);
    movement.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);movement.setRange(0,50,.1);movement.setTextBoxStyle(juce::Slider::TextBoxBelow,false,90,21);movement.setTextValueSuffix(" ct");movement.setDoubleClickReturnValue(true,0);
    movement.onValueChange=[this]{auto p=host.parameters.snapshot();p[index(Param::DetuneA)]=-static_cast<float>(movement.getValue());p[index(Param::DetuneB)]=static_cast<float>(movement.getValue());host.parameters.replace(p);};
    movement.setTooltip(utf8("Противоположная расстройка двух осцилляторов создаёт биения. Скорость зависит от высоты ноты."));
    for(std::size_t i=0;i<presetCards.size();++i){auto& card=presetCards[i];card.setButtonText(factoryPresets()[i].name);presetView.addAndMakeVisible(card);card.onClick=[this,i]{applyPreset(static_cast<int>(i));};}
    presets.onChange=[this]{const int id=presets.getSelectedId();if(id>=1&&id<=4)applyPreset(id-1);};
    previous.onClick=[this]{applyPreset((currentPreset+3)%4);};next.onClick=[this]{applyPreset((currentPreset+1)%4);};
    save.onClick=[this]{choosePresetFile(true);};load.onClick=[this]{choosePresetFile(false);};
    noob.onClick=[this]{setMode(true);};pro.onClick=[this]{setMode(false);};
    tabs.onSelect=[this](int tab){currentTab=tab;layoutControls();repaint();};
    settings.onClick=[this]{
        if(audioDialog){audioDialog->toFront(true);return;}
        auto selector=std::make_unique<juce::AudioDeviceSelectorComponent>(host.devices,0,0,0,2,true,false,true,false);selector->setSize(600,460);
        juce::DialogWindow::LaunchOptions options;options.content.setOwned(selector.release());options.dialogTitle=utf8("NOXIS — аудио и MIDI");options.dialogBackgroundColour=background();options.escapeKeyTriggersCloseButton=true;options.useNativeTitleBar=true;options.resizable=false;
        audioDialog=options.launchAsync();audioDialog->setLookAndFeel(&look);
    };
    panic.onClick=[this]{host.panic();keyboardState.allNotesOff(0);computerHeld.fill(false);};
    octDown.onClick=[this]{changeOctave(-1);};octUp.onClick=[this]{changeOctave(1);};
    keyboard.setAvailableRange(0,127);keyboard.setOctaveForMiddleC(4);keyboard.setLowestVisibleKey(48);keyboard.setKeyWidth(30);keyboard.setVelocity(.8f,true);keyboard.setWantsKeyboardFocus(false);keyboard.clearKeyMappings();
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,juce::Colour(0xffd8d4e5));keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId,juce::Colour(0xff15151f));keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,purple().withAlpha(.8f));keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,purple().withAlpha(.3f));
    keyboardState.addListener(this);
    status.setColour(juce::Label::textColourId,muted());status.setFont(juce::FontOptions(11));octave.setFont(juce::FontOptions(12));voiceCount.setFont(juce::FontOptions(12));levelText.setFont(juce::FontOptions(11));
    save.setTooltip(utf8("Сохранить все параметры в .noxis"));load.setTooltip(utf8("Загрузить пользовательский пресет"));panic.setTooltip(utf8("Плавно заглушить все ноты; Escape"));
    setMode(false);changeOctave(0);setSize(1200,760);startTimerHz(30);
}
MainComponent::~MainComponent(){stopTimer();if(auto* top=getTopLevelComponent())top->removeKeyListener(this);if(audioDialog)audioDialog.deleteAndZero();keyboardState.removeListener(this);setLookAndFeel(nullptr);}
void MainComponent::parentHierarchyChanged(){if(auto* top=getTopLevelComponent()){top->addKeyListener(this);}}
void MainComponent::handleNoteOn(juce::MidiKeyboardState*,int channel,int note,float velocity){host.send({NoteEvent::On,note,velocity,channel});}
void MainComponent::handleNoteOff(juce::MidiKeyboardState*,int channel,int note,float){host.send({NoteEvent::Off,note,0,channel});}
void MainComponent::applyPreset(int number){currentPreset=juce::jlimit(0,3,number);const auto& p=factoryPresets()[static_cast<std::size_t>(currentPreset)];host.parameters.replace(p.values);presetName=p.name;savedValues=p.values;presets.setSelectedId(currentPreset+1,juce::dontSendNotification);sync();repaint();}
void MainComponent::setMode(bool value){simple=value;noob.setToggleState(simple,juce::dontSendNotification);pro.setToggleState(!simple,juce::dontSendNotification);layoutControls();}
void MainComponent::changeOctave(int delta){host.panic();keyboardState.allNotesOff(0);computerHeld.fill(false);baseOctave=juce::jlimit(0,6,baseOctave+delta);keyboard.setLowestVisibleKey((baseOctave+1)*12);octave.setText("C"+juce::String(baseOctave)+utf8(" – E")+juce::String(baseOctave+1),juce::dontSendNotification);}
void MainComponent::choosePresetFile(bool writing){
    if(chooser)return;
    chooser=std::make_unique<juce::FileChooser>(writing?utf8("Сохранить пресет NOXIS"):utf8("Загрузить пресет NOXIS"),juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("My Sound.noxis"),"*.noxis");
    auto fileFlags=writing?(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting):(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles);
    chooser->launchAsync(fileFlags,[safe=juce::Component::SafePointer<MainComponent>(this),writing](const juce::FileChooser& dialog){if(!safe)return;const auto file=dialog.getResult();if(file!=juce::File{}){
        std::string error;Preset p{file.getFileNameWithoutExtension().toStdString(),safe->host.parameters.snapshot()};const std::filesystem::path path(file.getFullPathName().toWideCharPointer());
        bool ok=writing?savePreset(path,p,error):loadPreset(path,p,error);
        if(ok){if(!writing)safe->host.parameters.replace(p.values);safe->presetName=juce::String::fromUTF8(p.name.c_str());safe->savedValues=p.values;safe->presets.setSelectedId(0,juce::dontSendNotification);safe->presets.setText(safe->presetName,juce::dontSendNotification);safe->sync();}
        else juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,utf8("Пресет"),juce::String::fromUTF8(error.c_str()));
    }juce::MessageManager::callAsync([safe]{if(safe)safe->chooser.reset();});});
}
bool MainComponent::keyPressed(const juce::KeyPress& key){if(dynamic_cast<juce::TextEditor*>(juce::Component::getCurrentlyFocusedComponent()))return false;if(key==juce::KeyPress::escapeKey){panic.triggerClick();return true;}if(key.getTextCharacter()=='z'){changeOctave(-1);return true;}if(key.getTextCharacter()=='x'){changeOctave(1);return true;}return keyStateChanged(true);}
bool MainComponent::keyStateChanged(bool){
    const bool editing=dynamic_cast<juce::TextEditor*>(juce::Component::getCurrentlyFocusedComponent())!=nullptr;
    constexpr std::array<int,17> keys{'a','w','s','e','d','f','t','g','y','h','u','j','k','o','l','p',';'};bool handled=false;
    for(std::size_t i=0;i<keys.size();++i){const bool down=juce::KeyPress::isKeyCurrentlyDown(keys[i])||juce::KeyPress::isKeyCurrentlyDown(juce::CharacterFunctions::toUpperCase(static_cast<juce::juce_wchar>(keys[i])));if(editing&&down&&!computerHeld[i])continue;if(down!=computerHeld[i]){computerHeld[i]=down;handled=true;if(down){computerNotes[i]=(baseOctave+1)*12+static_cast<int>(i);keyboardState.noteOn(1,computerNotes[i],.8f);}else keyboardState.noteOff(1,computerNotes[i],0);}}
    return handled;
}
void MainComponent::sync(){master.sync();for(auto& k:knobs)k->sync();waveA.sync();waveB.sync();if(!movement.isMouseButtonDown())movement.setValue((std::abs(host.parameters.get(Param::DetuneA))+std::abs(host.parameters.get(Param::DetuneB)))*.5f,juce::dontSendNotification);}
void MainComponent::timerCallback(){
    if(!juce::Process::isForegroundProcess()&&std::any_of(computerHeld.begin(),computerHeld.end(),[](bool h){return h;})){host.panic();keyboardState.allNotesOff(0);computerHeld.fill(false);}
    sync();scope.update();meter.setLevel(host.peak.load(),host.rms.load());voiceCount.setText(juce::String(host.voices.load())+" / 8 voices",juce::dontSendNotification);
    const float peak=host.peak.load();levelText.setText(peak>.00001f?juce::String(20*std::log10(peak),1)+" dBFS":utf8("−∞ dBFS"),juce::dontSendNotification);
    if(auto* device=host.devices.getCurrentAudioDevice())status.setText(device->getName()+utf8(" · ")+juce::String(device->getCurrentSampleRate()/1000,1)+utf8(" kHz · ")+juce::String(device->getCurrentBufferSizeSamples())+" samples",juce::dontSendNotification);
    else status.setText(utf8("Нет аудиоустройства — откройте «Аудио». ")+host.initialError,juce::dontSendNotification);
    const bool modified=host.parameters.snapshot()!=savedValues;presets.setText(presetName+(modified?" *":""),juce::dontSendNotification);
}
void MainComponent::resized(){
    const int w=getWidth(),h=getHeight(),presetWidth=juce::jlimit(170,236,w-964);presets.setBounds(190,23,presetWidth,36);const int end=190+presetWidth;previous.setBounds(end+6,23,32,36);next.setBounds(end+44,23,32,36);save.setBounds(end+88,23,100,36);load.setBounds(end+194,23,100,36);
    noob.setBounds(w-414,23,65,36);pro.setBounds(w-343,23,58,36);master.setBounds(w-278,4,148,75);settings.setBounds(w-116,23,92,36);tabs.setBounds(24,88,470,42);
    const auto body=juce::Rectangle<int>(24,144,w-48,h-344);for(auto* view:{&synthView,&routeView,&fxView,&presetView})view->setBounds(body);
    const int footer=h-178;octDown.setBounds(24,footer,30,28);octave.setBounds(60,footer,100,28);octUp.setBounds(160,footer,30,28);voiceCount.setBounds(208,footer,100,28);panic.setBounds(w-104,footer,80,28);
    keyboard.setBounds(24,h-140,w-48,94);status.setBounds(24,h-36,w-475,24);meter.setBounds(w-326,h-27,205,8);levelText.setBounds(w-110,h-36,92,24);
    layoutControls();
}
static void row(std::initializer_list<Knob*> knobs,juce::Rectangle<int> r){const int width=r.getWidth()/static_cast<int>(knobs.size());for(auto* k:knobs)k->setBounds(r.removeFromLeft(width).reduced(4,0));}
void MainComponent::layoutControls(){
    synthView.setVisible(currentTab==0);routeView.setVisible(currentTab==1);fxView.setVisible(currentTab==2);presetView.setVisible(currentTab==3);
    const int w=synthView.getWidth(),h=synthView.getHeight();if(w<=0||h<=0)return;
    for(auto* p:{&oscA,&oscB,&filter,&envelope,&saturation})p->setVisible(!simple);macros.setVisible(simple);
    if(simple){scopePanel.setBounds(0,0,w,h/2-6);macros.setBounds(0,h/2+6,w,h/2-6);int kw=std::min(180,(w-60)/3);int start=(w-kw*3)/2;const int kh=macros.getHeight()-64;tone->setBounds(start,58,kw,kh);movementLabel.setBounds(start+kw,58,kw,19);movement.setBounds(start+kw,77,kw,kh-19);drive->setBounds(start+kw*2,58,kw,kh);}
    else {
        const int gap=12,top=(h-12)/2,col=(w-24)/3;oscA.setBounds(0,0,col,top);oscB.setBounds(col+gap,0,col,top);scopePanel.setBounds((col+gap)*2,0,w-(col+gap)*2,top);
        const int filterW=(w-24)/4,envW=(w-24)/2;filter.setBounds(0,top+gap,filterW,h-top-gap);envelope.setBounds(filterW+gap,top+gap,envW,h-top-gap);saturation.setBounds(filterW+envW+24,top+gap,w-filterW-envW-24,h-top-gap);
        for(auto* p:{&oscA,&oscB}){auto& wave=p==&oscA?waveA:waveB;wave.setBounds(14,57,p->getWidth()-28,28);}
        row({oscAKnobs[0],oscAKnobs[1],oscAKnobs[2]},{10,91,oscA.getWidth()-20,top-98});row({oscBKnobs[0],oscBKnobs[1],oscBKnobs[2]},{10,91,oscB.getWidth()-20,top-98});
        row({filterKnobs[0],filterKnobs[1]},{8,58,filter.getWidth()-16,filter.getHeight()-66});row({envKnobs[0],envKnobs[1],envKnobs[2],envKnobs[3]},{8,58,envelope.getWidth()-16,envelope.getHeight()-66});row({satKnobs[0],satKnobs[1]},{8,58,saturation.getWidth()-16,saturation.getHeight()-66});
    }
    scope.setBounds(8,58,scopePanel.getWidth()-16,scopePanel.getHeight()-68);
    row({fxKnobs[0],fxKnobs[1]},{(w-400)/2,100,400,std::min(220,h-130)});
    for(std::size_t i=0;i<presetCards.size();++i)presetCards[i].setBounds(24,60+static_cast<int>(i)*65,w-48,52);
    repaint();
}
void MainComponent::paint(juce::Graphics& g){
    g.fillAll(background());g.setColour(purple());g.setFont(juce::FontOptions(32,juce::Font::bold));g.drawText("NOXIS",24,19,160,38,juce::Justification::left);g.setColour(muted());g.setFont(juce::FontOptions(10));g.drawText("DARK MATTER / PHASE 01",26,58,160,14,juce::Justification::left);
    g.setColour(juce::Colour(0xff292736));g.drawHorizontalLine(80,24,static_cast<float>(getWidth()-24));g.drawHorizontalLine(getHeight()-190,24,static_cast<float>(getWidth()-24));
    g.setColour(muted());g.setFont(juce::FontOptions(11));g.drawText(utf8("A W S E D F T G Y H U J K  ·  Z / X — октава  ·  ESC — panic"),340,getHeight()-178,getWidth()-470,28,juce::Justification::centredLeft);
    auto body=routeView.getBounds();if(currentTab==1){g.setColour(text());g.setFont(juce::FontOptions(22,juce::Font::bold));g.drawText(utf8("Звуковой тракт"),body.removeFromTop(45),juce::Justification::left);g.setFont(juce::FontOptions(14));g.setColour(muted());g.drawText(utf8("Фиксированный порядок обработки в Phase 1"),body.removeFromTop(32),juce::Justification::left);
        const std::array<juce::String,4> labels{"OSC A + OSC B","LOW-PASS + ADSR","SATURATION","MASTER / OUTPUT"};int box=(body.getWidth()-90)/4;for(int i=0;i<4;++i){auto r=juce::Rectangle<int>(body.getX()+i*(box+30),body.getY()+65,box,100);g.setColour(panelColour());g.fillRoundedRectangle(r.toFloat(),10);g.setColour(purple());g.drawText(labels[static_cast<std::size_t>(i)],r,juce::Justification::centred);if(i<3)g.drawText(utf8("→"),r.getRight(),r.getY(),30,100,juce::Justification::centred);}
        g.setColour(muted());g.drawText(utf8("Каждый голос: два осциллятора → фильтр → амплитудная огибающая. Затем голоса суммируются."),body.getX(),body.getY()+190,body.getWidth(),35,juce::Justification::centred);
    }else if(currentTab==2){g.setColour(text());g.setFont(juce::FontOptions(20,juce::Font::bold));g.drawText("SATURATION",body.removeFromTop(40),juce::Justification::centred);g.setColour(muted());g.setFont(juce::FontOptions(13));g.drawText(utf8("Drive и Output управляют общей сатурацией движка"),body.removeFromTop(30),juce::Justification::centred);
    }else if(currentTab==3){g.setColour(text());g.setFont(juce::FontOptions(20,juce::Font::bold));g.drawText(utf8("Библиотека звуков"),body.removeFromTop(40),juce::Justification::left);}
}
}
