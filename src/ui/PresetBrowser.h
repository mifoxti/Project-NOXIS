#pragma once
#include "ui/Controls.h"
#include <set>

namespace noxis::ui {
class PresetBrowser final : public juce::Component,private juce::ListBoxModel {
public:
    PresetBrowser();
    void resized() override;void paint(juce::Graphics&) override;
    const std::vector<Preset>& library() const noexcept{return entries;}
    int addUserPreset(const Preset&);void select(int);
    std::function<void(int)> onLoad,onPreview;
private:
    int getNumRows() override{return static_cast<int>(visible.size());}
    void paintListBoxItem(int,juce::Graphics&,int,int,bool) override;
    void selectedRowsChanged(int) override;void listBoxItemDoubleClicked(int,const juce::MouseEvent&) override;
    void refresh();void showDescription();void saveFavorites();
    std::vector<Preset> entries;std::vector<int> visible;std::set<std::string> favorites;std::set<int> users;
    juce::ListBox list{"Presets",this};juce::TextEditor search;juce::ComboBox category;
    juce::TextButton favoriteOnly{utf8("Избранное")},favorite{utf8("В избранное")},preview{utf8("Прослушать")},apply{utf8("Открыть звук")};
    juce::Label description;int selected=0;bool selecting=false;
    juce::File favoritesFile;
};
}
