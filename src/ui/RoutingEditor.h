#pragma once
#include "ui/Controls.h"
#include <functional>
#include <vector>

namespace noxis::ui {
class RoutingEditor final : public juce::Component,public juce::TooltipClient,private juce::Timer {
public:
    explicit RoutingEditor(ParameterStore&);
    ~RoutingEditor() override;
    void reload();
    void setActivity(float peak);
    bool hasUnappliedChanges() const noexcept {return graph!=store.graph();}
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    bool keyPressed(const juce::KeyPress&) override;
    void focusLost(FocusChangeType) override;
    juce::String getTooltip() override;
    std::function<void(int)> onSelect;
    std::function<void()> onChange;
private:
    struct PortHit {int node=-1,port=0;bool output=false;bool valid() const noexcept{return node>=0;}};
    struct VisibleConnection {Connection connection;float alpha=0;bool wanted=true;};
    static constexpr float nodeWidth=150,nodeHeight=90;
    juce::Rectangle<float> canvas() const;
    juce::Point<float> toWorld(juce::Point<float>) const;
    juce::AffineTransform transform() const;
    juce::Rectangle<float> nodeBounds(int) const;
    juce::Point<float> portPoint(int,int,bool) const;
    std::vector<int> inputPorts(int) const;
    PortHit portAt(juce::Point<float>) const;
    int nodeAt(juce::Point<float>) const;
    int connectionAt(juce::Point<float>) const;
    juce::Path connectionPath(Connection) const;
    juce::Path connectionPath(juce::Point<float>,juce::Point<float>,bool modulation) const;
    void drawConnection(juce::Graphics&,const VisibleConnection&);
    void drawNode(juce::Graphics&,int);
    float connectionActivity(Connection) const;
    bool audibleNode(int) const;
    juce::String nodeDetail(int) const;
    void syncVisuals();
    void publish();
    void change(const RoutingGraph&);
    bool createConnection(Connection,int oldEdge=-1);
    void removeSelectedConnection();
    void undo();
    void redo();
    void autoLayout();
    void fitToView();
    void zoomAt(float,juce::Point<float>);
    void updateButtons();
    void timerCallback() override;

    ParameterStore& store;
    RoutingGraph graph{},dragOriginal{};
    std::vector<RoutingGraph> undoHistory,redoHistory;
    std::vector<VisibleConnection> visible;
    std::array<juce::TextButton,8> buttons;
    juce::String status;
    bool applied=true,fitPending=true,panning=false,connecting=false;
    int selectedNode=-1,selectedEdge=-1,dragNode=-1,dragSource=-1,reconnectEdge=-1;
    float zoom=.75f,activity=0,flowPhase=0;
    juce::Point<float> pan{20,20},panStart{},mouseStart{},nodeStart{},pointer{};
    PortHit hovered;
};
}
