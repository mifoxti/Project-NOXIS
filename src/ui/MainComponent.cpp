#include "ui/MainComponent.h"
#if JUCE_WINDOWS
#include <windows.h>
#undef min
#undef max
#endif

namespace noxis::ui {
MainComponent::MainComponent() {
    setLookAndFeel(&look);setWantsKeyboardFocus(true);master.slider.setSliderStyle(juce::Slider::LinearHorizontal);
    for(auto* c:std::initializer_list<juce::Component*>{&presets,&previous,&next,&save,&load,&noob,&pro,&settings,&master,&tabs,&keyboard,&panic,&octDown,&octUp,&keyboardEnabled,&rootNote,&meterLeft,&meterRight,&status,&octave,&voiceCount,&levelText,&synthPage,&routingPage,&fxPage,&browser,&modPage})addAndMakeVisible(c);
    for(auto* c:std::initializer_list<juce::Component*>{&oscA,&oscB,&unison,&filter,&envelope,&performance,&lfoMini1,&lfoMini2,&scopePanel,&macroPanel,&signals})synthPage.addAndMakeVisible(c);
    scopePanel.addAndMakeVisible(scope);for(auto* k:{&tone,&movement,&width,&drive,&space})macroPanel.addAndMakeVisible(k);
    for(auto* p:{&saturation,&chorus,&delay,&reverb})fxPage.addAndMakeVisible(p);
    for(auto* p:{&lfo1,&lfo2,&matrix1,&matrix2,&tempo})modPage.addAndMakeVisible(p);
    routingPage.addAndMakeVisible(routing);routingPage.addAndMakeVisible(inspector);
    routing.onSelect=[this](int node){selectNode(node);};routing.onChange=[this]{sync();};
    rebuildPresetSelector();presets.onChange=[this]{if(presets.getSelectedId()>0)applyPreset(presets.getSelectedId()-1);};
    previous.onClick=[this]{applyPreset((currentPreset+static_cast<int>(browser.library().size())-1)%static_cast<int>(browser.library().size()));};
    next.onClick=[this]{applyPreset((currentPreset+1)%static_cast<int>(browser.library().size()));};
    browser.onLoad=[this](int n){applyPreset(n);};browser.onPreview=[this](int n){previewPreset(n);};
    save.onClick=[this]{choosePresetFile(true);};load.onClick=[this]{choosePresetFile(false);};
    noob.onClick=[this]{setMode(true);};pro.onClick=[this]{setMode(false);};
    tabs.onSelect=[this](int tab){currentTab=tab;layoutPages();repaint();};
    settings.onClick=[this]{
        if(audioDialog){audioDialog->toFront(true);return;}
        auto selector=std::make_unique<juce::AudioDeviceSelectorComponent>(host.devices,0,0,0,2,true,false,true,false);selector->setSize(610,470);
        juce::DialogWindow::LaunchOptions options;options.content.setOwned(selector.release());options.dialogTitle=utf8("NOXIS — аудио и MIDI");options.dialogBackgroundColour=background();options.escapeKeyTriggersCloseButton=true;options.useNativeTitleBar=false;options.resizable=false;
        audioDialog=options.launchAsync();audioDialog->setLookAndFeel(&look);
    };
    panic.onClick=[this]{stopNotes();};octDown.onClick=[this]{changeOctave(-1);};octUp.onClick=[this]{changeOctave(1);};
    rootNote.addItemList({"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"},1);rootNote.setSelectedId(1,juce::dontSendNotification);rootNote.onChange=[this]{changeOctave(0);};
    keyboardEnabled.setClickingTogglesState(true);keyboardEnabled.setToggleState(true,juce::dontSendNotification);keyboardEnabled.onClick=[this]{stopNotes();};
    keyboardEnabled.setTooltip(utf8("Музыкальный ввод с физических клавиш. Q = C5, Z = C4; работает и в русской раскладке."));
    keyboard.setAvailableRange(0,127);keyboard.setOctaveForMiddleC(5);keyboard.setKeyWidth(28);keyboard.setVelocity(.8f,true);keyboard.setWantsKeyboardFocus(false);keyboard.clearKeyMappings();
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,juce::Colour(0xffc8c4d8));keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId,juce::Colour(0xff14141f));keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,purple().withAlpha(.8f));keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,purple().withAlpha(.25f));keyboardState.addListener(this);
    movement.slider.onValueChange=[this]{auto v=host.parameters.snapshot();const float amount=static_cast<float>(movement.slider.getValue());v[index(Param::UnisonDetune)]=amount;v[index(Param::UnisonVoices)]=std::max(2.0f,v[index(Param::UnisonVoices)]);v[index(Param::Lfo1Cutoff)]=amount/25;v[index(Param::Lfo1Rate)]=.15f+amount/25;host.parameters.replace(v);};
    width.slider.onValueChange=[this]{auto v=host.parameters.snapshot();v[index(Param::Width)]=static_cast<float>(width.slider.getValue());v[index(Param::UnisonVoices)]=std::max(2.0f,v[index(Param::UnisonVoices)]);v[index(Param::Spread)]=std::max(.5f,v[index(Param::Spread)]);host.parameters.replace(v);};
    drive.slider.onValueChange=[this]{auto v=host.parameters.snapshot();v[index(Param::Drive)]=static_cast<float>(drive.slider.getValue());v[index(Param::SaturationEnabled)]=1;host.parameters.replace(v);};
    space.slider.onValueChange=[this]{auto v=host.parameters.snapshot();const float mix=static_cast<float>(space.slider.getValue());v[index(Param::ReverbMix)]=mix;v[index(Param::DelayMix)]=mix*.4f;v[index(Param::ReverbEnabled)]=mix>0?1.0f:0.0f;v[index(Param::DelayEnabled)]=mix>0?1.0f:0.0f;host.parameters.replace(v);};
    for(auto* label:{&status,&octave,&voiceCount,&levelText}){label->setFont(juce::FontOptions(11));label->setColour(juce::Label::textColourId,muted());}
    save.setTooltip(utf8("Сохранить параметры, эффекты, метаданные и схему Routing в .noxis v2"));load.setTooltip(utf8("Открыть .noxis версии 1 или 2"));panic.setTooltip(utf8("Отпустить все голоса и очистить музыкальные клавиши; Escape"));
    setMode(false);changeOctave(0);applyPreset(0);setSize(1280,790);startTimerHz(30);
}
MainComponent::~MainComponent(){stopTimer();if(auto* top=getTopLevelComponent())top->removeKeyListener(this);if(audioDialog)audioDialog.deleteAndZero();keyboardState.removeListener(this);setLookAndFeel(nullptr);}
void MainComponent::parentHierarchyChanged(){if(auto* top=getTopLevelComponent())top->addKeyListener(this);}
void MainComponent::handleNoteOn(juce::MidiKeyboardState*,int channel,int note,float velocity){if(!syncingMidi)host.send({NoteEvent::On,note,velocity,channel});}
void MainComponent::handleNoteOff(juce::MidiKeyboardState*,int channel,int note,float){if(!syncingMidi)host.send({NoteEvent::Off,note,0,channel});}
void MainComponent::stopNotes(){host.panic();keyboardState.allNotesOff(0);computerKeyboard.reset();previewNote=-1;for(auto& value:midiVisual)value=false;}
void MainComponent::rebuildPresetSelector(){presets.clear(juce::dontSendNotification);int id=1;for(const auto& p:browser.library())presets.addItem(juce::String::fromUTF8(p.name.c_str()),id++);}
void MainComponent::applyPreset(int n){
    currentPreset=juce::jlimit(0,static_cast<int>(browser.library().size())-1,n);const auto p=browser.library()[static_cast<std::size_t>(currentPreset)];std::string error;
    if(!host.parameters.replacePatch(p.values,p.routing,error)){juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Routing",utf8(error.c_str()));return;}
    savedPreset=p;presets.setSelectedId(currentPreset+1,juce::dontSendNotification);browser.select(currentPreset);routing.reload();sync();repaint();
}
void MainComponent::previewPreset(int n){stopNotes();applyPreset(n);const auto& p=browser.library()[static_cast<std::size_t>(currentPreset)];previewNote=juce::jlimit(p.lowNote,p.highNote,p.category=="BASS"?48:60);host.send({NoteEvent::On,previewNote,.8f,15});previewUntil=juce::Time::getMillisecondCounter()+1200;}
void MainComponent::setMode(bool mode){simple=mode;noob.setToggleState(simple,juce::dontSendNotification);pro.setToggleState(!simple,juce::dontSendNotification);layoutPages();}
void MainComponent::changeOctave(int delta){stopNotes();baseOctave=juce::jlimit(1,8,baseOctave+delta);const int root=baseOctave*12+std::max(0,rootNote.getSelectedId()-1);keyboard.setLowestVisibleKey(std::max(0,root-12));octave.setText("Q = "+rootNote.getText()+juce::String(baseOctave),juce::dontSendNotification);}
void MainComponent::choosePresetFile(bool writing){
    if(chooser)return;if(writing&&routing.hasUnappliedChanges()){juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Routing",utf8("Сначала завершите или отмените черновик Routing: сейчас звучит последняя валидная схема."));return;}
    const auto folder=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("NOXIS Presets");if(writing)folder.createDirectory();
    chooser=std::make_unique<juce::FileChooser>(writing?utf8("Сохранить пресет NOXIS"):utf8("Открыть пресет NOXIS"),folder.getChildFile(writing?"My Sound.noxis":""),"*.noxis",false);
    const int fileFlags=writing?(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting):(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles);
    chooser->launchAsync(fileFlags,[safe=juce::Component::SafePointer<MainComponent>(this),writing](const juce::FileChooser& dialog){if(!safe)return;auto file=dialog.getResult();if(file!=juce::File{}){
        if(writing)file=file.withFileExtension("noxis");Preset p=safe->savedPreset;std::string error;const std::filesystem::path path(file.getFullPathName().toWideCharPointer());
        if(writing){p.name=file.getFileNameWithoutExtension().toStdString();p.values=safe->host.parameters.snapshot();p.routing=safe->host.parameters.graph();p.category="USER";}
        const bool ok=writing?savePreset(path,p,error):loadPreset(path,p,error);
        if(ok){const int index=safe->browser.addUserPreset(p);safe->rebuildPresetSelector();safe->applyPreset(index);}
        else juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,utf8("Пресет"),utf8(error.c_str()));
    }juce::MessageManager::callAsync([safe]{if(safe)safe->chooser.reset();});});
}
bool MainComponent::keyPressed(const juce::KeyPress& key){if(dynamic_cast<juce::TextEditor*>(juce::Component::getCurrentlyFocusedComponent()))return false;if(key==juce::KeyPress::escapeKey){panic.triggerClick();return true;}if(key.getTextCharacter()=='['){changeOctave(-1);return true;}if(key.getTextCharacter()==']'){changeOctave(1);return true;}return keyStateChanged(true);}
bool MainComponent::keyStateChanged(bool){
    const bool editing=dynamic_cast<juce::TextEditor*>(juce::Component::getCurrentlyFocusedComponent())!=nullptr;std::array<bool,musicalKeys.size()> keys{};
    for(std::size_t i=0;i<keys.size();++i){
#if JUCE_WINDOWS
        const auto vk=MapVirtualKeyW(static_cast<UINT>(musicalKeys[i].scan),MAPVK_VSC_TO_VK_EX);keys[i]=(GetAsyncKeyState(static_cast<int>(vk))&0x8000)!=0;
#else
        keys[i]=juce::KeyPress::isKeyCurrentlyDown(musicalKeys[i].key);
#endif
    }
    const bool focused=juce::Process::isForegroundProcess()&&getTopLevelComponent()->hasKeyboardFocus(true);const auto mods=juce::ModifierKeys::getCurrentModifiersRealtime();
    const int root=std::min(127,baseOctave*12+std::max(0,rootNote.getSelectedId()-1));
    computerKeyboard.update(keys,root,keyboardEnabled.getToggleState(),editing||mods.isCtrlDown()||mods.isAltDown(),focused,[this](int n){keyboardState.noteOn(1,n,.8f);},[this](int n){keyboardState.noteOff(1,n,0);});
    return !editing&&!mods.isCtrlDown()&&!mods.isAltDown()&&keyboardEnabled.getToggleState()&&std::any_of(keys.begin(),keys.end(),[](bool down){return down;});
}
void MainComponent::sync(){
    master.sync();for(auto* p:{&oscA,&oscB,&unison,&filter,&envelope,&performance,&lfoMini1,&lfoMini2,&saturation,&chorus,&delay,&reverb,&lfo1,&lfo2,&matrix1,&matrix2,&tempo,&inspector})p->sync();
    for(auto* p:{&tone,&movement,&width,&drive,&space})p->sync();
}
void MainComponent::timerCallback(){
    keyStateChanged(false);if(previewNote>=0&&static_cast<juce::int32>(juce::Time::getMillisecondCounter()-previewUntil)>=0){host.send({NoteEvent::Off,previewNote,0,15});previewNote=-1;}
    syncingMidi=true;for(int n=0;n<128;++n){const bool down=host.midiDown[n].load();if(down!=midiVisual[n]){midiVisual[n]=down;if(down)keyboardState.noteOn(16,n,.8f);else keyboardState.noteOff(16,n,0);}}syncingMidi=false;
    sync();scope.update();signals.update();routing.setActivity(host.peak.load());meterLeft.setLevel(host.stereoPeak[0].load(),host.stereoRms[0].load());meterRight.setLevel(host.stereoPeak[1].load(),host.stereoRms[1].load());
    voiceCount.setText(juce::String(host.voices.load())+" / 8 voices",juce::dontSendNotification);const float peak=host.peak.load();levelText.setText(peak>.00001f?juce::String(20*std::log10(peak),1)+" dBFS":utf8("−∞ dBFS"),juce::dontSendNotification);
    if(auto* device=host.devices.getCurrentAudioDevice())status.setText(device->getName()+utf8(" · ")+juce::String(device->getCurrentSampleRate()/1000,1)+" kHz / "+juce::String(device->getCurrentBufferSizeSamples())+utf8(" samples · QWERTY + ЙЦУКЕН"),juce::dontSendNotification);
    else status.setText(utf8("Нет аудиоустройства — откройте «Аудио». ")+host.initialError,juce::dontSendNotification);
    const bool changed=host.parameters.snapshot()!=savedPreset.values||host.parameters.graph()!=savedPreset.routing;const auto title=utf8(savedPreset.name.c_str())+(changed?" *":"");if(presets.getText()!=title)presets.setText(title,juce::dontSendNotification);
}
void MainComponent::selectNode(int node){
    using P=Param;std::vector<P> ids;
    switch(static_cast<Node>(node)){
        case Node::OscA:ids={P::WaveA,P::OctaveA,P::DetuneA,P::LevelA,P::SubLevel};break;
        case Node::OscB:ids={P::WaveB,P::OctaveB,P::SemitoneB,P::DetuneB,P::LevelB};break;
        case Node::Mixer:ids={P::UnisonVoices,P::UnisonDetune,P::Spread,P::Width,P::PhaseRandom,P::NoiseLevel};break;
        case Node::Filter:ids={P::Cutoff,P::Resonance,P::EnvCutoff};break;
        case Node::Saturation:ids={P::SaturationEnabled,P::Drive,P::Output,P::SaturationMix};break;
        case Node::Chorus:ids={P::ChorusEnabled,P::ChorusRate,P::ChorusDepth,P::ChorusMix};break;
        case Node::Delay:ids={P::DelayEnabled,P::DelayTime,P::DelayFeedback,P::DelayMix,P::DelayStereo};break;
        case Node::Reverb:ids={P::ReverbEnabled,P::ReverbSize,P::ReverbDamping,P::ReverbMix};break;
        case Node::Output:ids={P::Master,P::Width};break;
        case Node::Lfo1:ids={P::Lfo1Wave,P::Lfo1Rate,P::Lfo1Depth,P::Lfo1Phase,P::Lfo1Sync,P::Lfo1Retrigger,P::Lfo1Pitch,P::Lfo1Cutoff,P::Lfo1Level,P::Lfo1Width};break;
        case Node::Lfo2:ids={P::Lfo2Wave,P::Lfo2Rate,P::Lfo2Depth,P::Lfo2Phase,P::Lfo2Sync,P::Lfo2Retrigger,P::Lfo2Pitch,P::Lfo2Cutoff,P::Lfo2Level,P::Lfo2Width};break;
        default:ids={P::Attack,P::Decay,P::Sustain,P::Release};break;
    }
    inspector.configure(juce::String(nodeNames[static_cast<std::size_t>(juce::jlimit(0,nodeCount-1,node))].data()),std::move(ids),2);
}
void MainComponent::resized(){
    const int w=getWidth(),h=getHeight();const int presetWidth=juce::jlimit(160,240,w-1000),end=174+presetWidth;
    presets.setBounds(174,13,presetWidth,32);previous.setBounds(end+5,13,28,32);next.setBounds(end+38,13,28,32);save.setBounds(end+76,13,88,32);load.setBounds(end+170,13,76,32);
    noob.setBounds(w-388,13,58,32);pro.setBounds(w-325,13,52,32);master.setBounds(w-266,0,148,65);settings.setBounds(w-106,13,90,32);tabs.setBounds(16,65,570,36);
    auto body=juce::Rectangle<int>(16,110,w-32,h-270);for(auto* p:{&synthPage,&routingPage,&fxPage,&modPage})p->setBounds(body);browser.setBounds(body);
    const int footer=h-148;octDown.setBounds(16,footer,26,26);octave.setBounds(46,footer,85,26);octUp.setBounds(132,footer,26,26);rootNote.setBounds(170,footer,62,26);keyboardEnabled.setBounds(242,footer,58,26);voiceCount.setBounds(311,footer,100,26);panic.setBounds(w-96,footer,80,26);
    keyboard.setBounds(16,h-114,w-32,80);status.setBounds(16,h-29,w-410,22);meterLeft.setBounds(w-302,h-24,190,6);meterRight.setBounds(w-302,h-13,190,6);levelText.setBounds(w-104,h-29,88,22);layoutPages();
}
void MainComponent::layoutPages(){
    synthPage.setVisible(currentTab==0);routingPage.setVisible(currentTab==1);fxPage.setVisible(currentTab==2);browser.setVisible(currentTab==3);modPage.setVisible(currentTab==4);
    const int w=synthPage.getWidth(),h=synthPage.getHeight();if(w<=0||h<=0)return;const int side=std::min(280,w/4),left=w-side-10,gap=8;
    for(auto* p:{&oscA,&oscB,&unison,&filter,&envelope,&performance,&lfoMini1,&lfoMini2})p->setVisible(!simple);macroPanel.setVisible(simple);
    const int top=static_cast<int>(h*.33f),middle=static_cast<int>(h*.35f),bottom=h-top-middle-2*gap;
    scopePanel.setBounds(left+10,0,side,top);signals.setBounds(left+10,top+gap,side,h-top-gap);
    if(simple){scopePanel.setBounds(0,0,left,static_cast<int>(h*.55f));macroPanel.setBounds(0,static_cast<int>(h*.55f)+gap,left,h-static_cast<int>(h*.55f)-gap);signals.setBounds(left+10,0,side,h);const int cell=(left-24)/5;int x=12;for(auto* k:{&tone,&movement,&width,&drive,&space}){k->setBounds(x,58,cell,macroPanel.getHeight()-66);x+=cell;}}
    else {const int half=(left-gap)/2;oscA.setBounds(0,0,half,top);oscB.setBounds(half+gap,0,left-half-gap,top);int x=0;const int uw=left/3,fw=left/4;unison.setBounds(x,top+gap,uw,middle);x+=uw+gap;filter.setBounds(x,top+gap,fw,middle);x+=fw+gap;envelope.setBounds(x,top+gap,left-x,middle);const int y=top+middle+2*gap,pw=left*44/100,lw=(left-pw-2*gap)/2;performance.setBounds(0,y,pw,bottom);lfoMini1.setBounds(pw+gap,y,lw,bottom);lfoMini2.setBounds(pw+lw+2*gap,y,left-pw-lw-2*gap,bottom);}
    scope.setBounds(5,49,scopePanel.getWidth()-10,scopePanel.getHeight()-56);
    const int half=(w-gap)/2,fh=(h-gap)/2;saturation.setBounds(0,0,half,fh);chorus.setBounds(half+gap,0,w-half-gap,fh);delay.setBounds(0,fh+gap,half,h-fh-gap);reverb.setBounds(half+gap,fh+gap,w-half-gap,h-fh-gap);
    const int inspectorWidth=244;routing.setBounds(0,0,w-inspectorWidth-gap,h);inspector.setBounds(w-inspectorWidth,0,inspectorWidth,h);
    const int modTop=static_cast<int>(h*.50f),tempoWidth=220,modHalf=(w-tempoWidth-2*gap)/2;lfo1.setBounds(0,0,modHalf,modTop);lfo2.setBounds(modHalf+gap,0,modHalf,modTop);tempo.setBounds(w-tempoWidth,0,tempoWidth,h);matrix1.setBounds(0,modTop+gap,modHalf,h-modTop-gap);matrix2.setBounds(modHalf+gap,modTop+gap,modHalf,h-modTop-gap);repaint();
}
void MainComponent::paint(juce::Graphics& g){g.fillAll(background());g.setColour(purple());g.setFont(juce::FontOptions(29,juce::Font::bold));g.drawText("NOXIS",16,8,145,36,juce::Justification::left);g.setColour(muted());g.setFont(juce::FontOptions(9));g.drawText("SOUND IDENTITY / 02",18,44,145,14,juce::Justification::left);g.setColour(surface());g.drawHorizontalLine(61,16,static_cast<float>(getWidth()-16));g.drawHorizontalLine(getHeight()-158,16,static_cast<float>(getWidth()-16));g.setColour(muted());g.setFont(juce::FontOptions(10));g.drawText(utf8("Q W E R T Y U I / Z X C V B N M · [ ] октава · ESC panic"),430,getHeight()-148,getWidth()-540,26,juce::Justification::centredLeft);}
}
