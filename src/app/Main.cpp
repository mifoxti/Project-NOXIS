#include "ui/MainComponent.h"
#include "audio/WavWriter.h"
#include <fstream>

class NoxisApplication final : public juce::JUCEApplication,private juce::Timer {
public:
    const juce::String getApplicationName() override{return "NOXIS";}
    const juce::String getApplicationVersion() override{return "0.2.0";}
    bool moreThanOneInstanceAllowed() override{return false;}
    void initialise(const juce::String& commandLine) override {
        window=std::make_unique<Window>();
        if(commandLine.contains("--smoke-test")||commandLine.contains("--stress-test")){
            smoke=true;stress=commandLine.contains("--stress-test");window->main->applyPreset(4);auto& host=window->main->host;auto p=host.parameters.snapshot();
            for(auto id:{noxis::Param::ChorusEnabled,noxis::Param::DelayEnabled,noxis::Param::ReverbEnabled})p[noxis::index(id)]=1;
            for(auto id:{noxis::Param::ChorusMix,noxis::Param::DelayMix,noxis::Param::ReverbMix})p[noxis::index(id)]=.2f;
            if(stress){p[noxis::index(noxis::Param::UnisonVoices)]=8;p[noxis::index(noxis::Param::WaveA)]=3;p[noxis::index(noxis::Param::WaveB)]=3;}
            host.parameters.replace(p);host.beginCapture();startTimer(500);
        }
    }
    void shutdown() override{stopTimer();window.reset();}
    void systemRequestedQuit() override{quit();}
    void anotherInstanceStarted(const juce::String&) override{if(window)window->toFront(true);}
private:
    class Window final : public juce::DocumentWindow {
    public:
        Window():DocumentWindow("NOXIS",noxis::ui::background(),allButtons){setLookAndFeel(&look);setUsingNativeTitleBar(false);setTitleBarHeight(28);setColour(juce::DocumentWindow::textColourId,noxis::ui::muted());main=new noxis::ui::MainComponent;setContentOwned(main,true);setResizable(true,false);setResizeLimits(1100,720,2000,1400);centreWithSize(1280,820);setVisible(true);}
        ~Window() override {clearContentComponent();setLookAndFeel(nullptr);}
        void closeButtonPressed() override{juce::JUCEApplication::getInstance()->systemRequestedQuit();}
        noxis::ui::MainComponent* main=nullptr;
        noxis::ui::NoxisLookAndFeel look;
    };
    void timerCallback() override {
        if(!smoke||!window)return;auto& host=window->main->host;
        peakCpu=std::max(peakCpu,host.devices.getCpuUsage());
        if(step==0){for(int i=0;i<(stress?8:2);++i)host.send({noxis::NoteEvent::On,45+i*3,.65f,1});}
        if(step==3){for(int i=0;i<(stress?8:2);++i)host.send({noxis::NoteEvent::Off,45+i*3,0,1});}
        if(++step<7)return;
        stopTimer();host.endCapture();
        const auto audio=host.capturedAudio();double energy=0,stereoDifference=0;float peak=0;for(float s:audio){energy+=s*s;peak=std::max(peak,std::abs(s));}for(std::size_t i=0;i+1<audio.size();i+=2){const double difference=audio[i]-audio[i+1];stereoDifference+=difference*difference;}
        const bool passed=!audio.empty()&&energy>.01&&stereoDifference>1.e-6&&peak>.001f&&peak<.97f&&host.callbacks.load()>0&&host.voices.load()==0;
        const auto folder=juce::File::getCurrentWorkingDirectory();
        noxis::writeWav(std::filesystem::path(folder.getChildFile("standalone-smoke.wav").getFullPathName().toWideCharPointer()),audio,static_cast<int>(host.currentRate()),2);
        std::ofstream report(std::filesystem::path(folder.getChildFile("standalone-smoke.txt").getFullPathName().toWideCharPointer()));
        report<<"result="<<(passed?"PASS":"FAIL")<<"\ncallbacks="<<host.callbacks.load()<<"\nsamples="<<audio.size()<<"\npeak="<<peak<<"\nvoices_after_release="<<host.voices.load()<<"\nrate="<<host.currentRate()<<'\n';
        report<<"stress="<<(stress?1:0)<<"\nchannels=2\nstereo_difference_energy="<<stereoDifference<<"\npeak_cpu_fraction="<<peakCpu<<"\nxruns="<<host.devices.getXRunCount()<<'\n';
        if(auto* device=host.devices.getCurrentAudioDevice())report<<"device="<<device->getName().toStdString()<<"\nbuffer="<<device->getCurrentBufferSizeSamples()<<'\n';
        report.close();setApplicationReturnValue(passed?0:1);quit();
    }
    std::unique_ptr<Window> window;bool smoke=false,stress=false;int step=0;double peakCpu=0;
};
START_JUCE_APPLICATION(NoxisApplication)
