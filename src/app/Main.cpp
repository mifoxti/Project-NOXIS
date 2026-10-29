#include "ui/MainComponent.h"
#include "audio/WavWriter.h"
#include <fstream>

class NoxisApplication final : public juce::JUCEApplication,private juce::Timer {
public:
    const juce::String getApplicationName() override{return "NOXIS";}
    const juce::String getApplicationVersion() override{return "0.1.0";}
    bool moreThanOneInstanceAllowed() override{return false;}
    void initialise(const juce::String& commandLine) override {
        window=std::make_unique<Window>();
        if(commandLine.contains("--smoke-test")){smoke=true;window->main->applyPreset(1);window->main->host.beginCapture();startTimer(500);}
    }
    void shutdown() override{stopTimer();window.reset();}
    void systemRequestedQuit() override{quit();}
    void anotherInstanceStarted(const juce::String&) override{if(window)window->toFront(true);}
private:
    class Window final : public juce::DocumentWindow {
    public:
        Window():DocumentWindow("NOXIS",noxis::ui::background(),allButtons){setUsingNativeTitleBar(true);main=new noxis::ui::MainComponent;setContentOwned(main,true);setResizable(true,false);setResizeLimits(1100,720,1800,1100);centreWithSize(1200,760);setVisible(true);}
        void closeButtonPressed() override{juce::JUCEApplication::getInstance()->systemRequestedQuit();}
        noxis::ui::MainComponent* main=nullptr;
    };
    void timerCallback() override {
        if(!smoke||!window)return;auto& host=window->main->host;
        if(step==0){host.send({noxis::NoteEvent::On,45,.8f,1});host.send({noxis::NoteEvent::On,52,.7f,1});}
        if(step==3){host.send({noxis::NoteEvent::Off,45,0,1});host.send({noxis::NoteEvent::Off,52,0,1});}
        if(++step<7)return;
        stopTimer();host.endCapture();
        const auto audio=host.capturedAudio();double energy=0;float peak=0;for(float s:audio){energy+=s*s;peak=std::max(peak,std::abs(s));}
        const bool passed=!audio.empty()&&energy>.01&&peak>.001f&&peak<.97f&&host.callbacks.load()>0&&host.voices.load()==0;
        const auto folder=juce::File::getCurrentWorkingDirectory();
        noxis::writeWav(std::filesystem::path(folder.getChildFile("standalone-smoke.wav").getFullPathName().toWideCharPointer()),audio,static_cast<int>(host.currentRate()));
        std::ofstream report(std::filesystem::path(folder.getChildFile("standalone-smoke.txt").getFullPathName().toWideCharPointer()));
        report<<"result="<<(passed?"PASS":"FAIL")<<"\ncallbacks="<<host.callbacks.load()<<"\nsamples="<<audio.size()<<"\npeak="<<peak<<"\nvoices_after_release="<<host.voices.load()<<"\nrate="<<host.currentRate()<<'\n';
        if(auto* device=host.devices.getCurrentAudioDevice())report<<"device="<<device->getName().toStdString()<<"\nbuffer="<<device->getCurrentBufferSizeSamples()<<'\n';
        report.close();setApplicationReturnValue(passed?0:1);quit();
    }
    std::unique_ptr<Window> window;bool smoke=false;int step=0;
};
START_JUCE_APPLICATION(NoxisApplication)
