#include "ui/RoutingEditor.h"
#include <cmath>

namespace noxis::ui {
RoutingEditor::RoutingEditor(ParameterStore& parameters):store(parameters) {
    setWantsKeyboardFocus(true);
    const std::array<juce::String,8> labels{utf8("Авто"),utf8("Сброс"),utf8("Вписать"),utf8("−"),"+",utf8("Отмена"),utf8("Повтор"),utf8("Удалить связь")};
    const std::array<juce::String,8> tips{
        utf8("Аккуратно расположить ноды; соединения сохраняются"),utf8("Вернуть заводской звуковой тракт"),
        utf8("Показать всю схему"),utf8("Уменьшить масштаб"),utf8("Увеличить масштаб"),
        utf8("Отменить изменение графа: Ctrl+Z"),utf8("Повторить изменение: Ctrl+Y"),
        utf8("Удалить выбранное соединение: Delete")};
    for(std::size_t i=0;i<buttons.size();++i){buttons[i].setButtonText(labels[i]);buttons[i].setTooltip(tips[i]);buttons[i].setWantsKeyboardFocus(false);addAndMakeVisible(buttons[i]);}
    buttons[0].onClick=[this]{autoLayout();};
    buttons[1].onClick=[this]{change(defaultRouting());fitToView();};
    buttons[2].onClick=[this]{fitToView();};
    buttons[3].onClick=[this]{zoomAt(zoom/1.2f,canvas().getCentre());};
    buttons[4].onClick=[this]{zoomAt(zoom*1.2f,canvas().getCentre());};
    buttons[5].onClick=[this]{undo();};buttons[6].onClick=[this]{redo();};
    buttons[7].onClick=[this]{removeSelectedConnection();};
    undoHistory.reserve(64);redoHistory.reserve(64);visible.reserve(maxEdges*2);
    reload();startTimerHz(30);
}
RoutingEditor::~RoutingEditor(){stopTimer();}
void RoutingEditor::reload() {
    graph=store.graph();dragOriginal=graph;undoHistory.clear();redoHistory.clear();visible.clear();
    selectedNode=selectedEdge=dragNode=dragSource=reconnectEdge=-1;connecting=panning=false;hovered={};
    applied=true;status=utf8("Схема активна · аудио и модуляции");fitPending=true;
    syncVisuals();for(auto& line:visible)line.alpha=1;fitToView();updateButtons();repaint();
}
void RoutingEditor::setActivity(float peak){activity=std::max(activity,std::clamp(peak,0.0f,1.0f));}
juce::Rectangle<float> RoutingEditor::canvas() const {return getLocalBounds().toFloat().withTrimmedTop(44).withTrimmedBottom(28);}
juce::Point<float> RoutingEditor::toWorld(juce::Point<float> point) const {return (point-canvas().getPosition()-pan)/zoom;}
juce::AffineTransform RoutingEditor::transform() const {const auto origin=canvas().getPosition()+pan;return {zoom,0,origin.x,0,zoom,origin.y};}
juce::Rectangle<float> RoutingEditor::nodeBounds(int node) const {return {graph.x[static_cast<std::size_t>(node)],graph.y[static_cast<std::size_t>(node)],nodeWidth,nodeHeight};}
std::vector<int> RoutingEditor::inputPorts(int node) const {
    if(node==2)return {0,1,2,4,5};if(node==3)return {0,3};if(node>=4&&node<=8)return {0};return {};
}
juce::Point<float> RoutingEditor::portPoint(int node,int port,bool output) const {
    auto bounds=nodeBounds(node);
    if(output)return {bounds.getRight(),bounds.getY()+43};
    if(port>=2){const float x=node==2?(port==2?28.0f:(port==4?75.0f:122.0f)):75.0f;return {bounds.getX()+x,bounds.getBottom()};}
    return {bounds.getX(),bounds.getY()+(node==2?(port==0?30.0f:57.0f):43.0f)};
}
RoutingEditor::PortHit RoutingEditor::portAt(juce::Point<float> point) const {
    const float distance=9.0f/zoom;
    for(int node=nodeCount-1;node>=0;--node) {
        if(!graph.present[static_cast<std::size_t>(node)])continue;
        if(node!=8&&point.getDistanceFrom(portPoint(node,0,true))<=distance)return {node,0,true};
        for(int port:inputPorts(node))if(point.getDistanceFrom(portPoint(node,port,false))<=distance)return {node,port,false};
    }
    return {};
}
int RoutingEditor::nodeAt(juce::Point<float> point) const {for(int node=nodeCount-1;node>=0;--node)if(graph.present[static_cast<std::size_t>(node)]&&nodeBounds(node).contains(point))return node;return -1;}
juce::Path RoutingEditor::connectionPath(juce::Point<float> start,juce::Point<float> end,bool modulation) const {
    juce::Path path;path.startNewSubPath(start);
    const float bend=std::clamp(std::abs(end.x-start.x)*.45f,45.0f,180.0f);
    if(modulation)path.cubicTo(start+juce::Point<float>(bend,0),end+juce::Point<float>(0,65),end);
    else path.cubicTo(start+juce::Point<float>(bend,0),end-juce::Point<float>(bend,0),end);
    return path;
}
juce::Path RoutingEditor::connectionPath(Connection edge) const {return connectionPath(portPoint(edge.from,0,true),portPoint(edge.to,edge.port,false),edge.from>=audioNodeCount);}
int RoutingEditor::connectionAt(juce::Point<float> point) const {
    float best=7.0f/zoom;int selected=-1;
    for(int i=0;i<graph.count;++i){juce::Point<float> nearest;const auto path=connectionPath(graph.edges[static_cast<std::size_t>(i)]);const float distance=path.getNearestPoint(point,nearest);if(distance<best){best=distance;selected=i;}}
    return selected;
}
bool RoutingEditor::audibleNode(int node) const {
    if(node<0||node>=audioNodeCount)return false;
    std::array<bool,audioNodeCount> audible{};audible[8]=true;
    for(int pass=0;pass<audioNodeCount;++pass)for(int i=0;i<graph.count;++i){const auto edge=graph.edges[static_cast<std::size_t>(i)];if(edge.from<audioNodeCount&&audible[edge.to])audible[edge.from]=true;}
    return audible[static_cast<std::size_t>(node)];
}
float RoutingEditor::connectionActivity(Connection edge) const {
    if(!applied||!audibleNode(edge.to))return 0;
    if(edge.from<audioNodeCount)return activity;
    if(edge.from==11)return activity*std::min(1.0f,std::abs(store.get(Param::EnvCutoff)));
    const bool second=edge.from==10;
    const std::array<Param,4> first{Param::Lfo1Pitch,Param::Lfo1Cutoff,Param::Lfo1Level,Param::Lfo1Width};
    const std::array<Param,4> other{Param::Lfo2Pitch,Param::Lfo2Cutoff,Param::Lfo2Level,Param::Lfo2Width};
    const int target=std::clamp(edge.port-2,0,3);
    return activity*std::min(1.0f,std::abs(store.get((second?other:first)[static_cast<std::size_t>(target)])))
        *store.get(second?Param::Lfo2Depth:Param::Lfo1Depth);
}
void RoutingEditor::drawConnection(juce::Graphics& graphics,const VisibleConnection& line) {
    const auto edge=line.connection;auto path=connectionPath(edge);
    const bool selected=selectedEdge>=0&&selectedEdge<graph.count&&graph.edges[static_cast<std::size_t>(selectedEdge)]==edge;
    const float signal=connectionActivity(edge);
    auto colour=edge.from>=audioNodeCount?pink():purple();
    if(!applied)colour=muted();
    graphics.setColour(colour.withAlpha(line.alpha*(selected?.95f:(line.wanted?.5f:.28f))));
    graphics.strokePath(path,juce::PathStrokeType((selected?2.5f:1.5f)/zoom));
    const float length=path.getLength();
    if(length>20/zoom){
        const auto tip=path.getPointAlongPath(length-9/zoom),back=path.getPointAlongPath(length-17/zoom);
        auto direction=tip-back;const float norm=std::max(.001f,direction.getDistanceFromOrigin());direction/=norm;
        const juce::Point<float> side{-direction.y*3.6f/zoom,direction.x*3.6f/zoom};
        juce::Path arrow;arrow.addTriangle(tip,back+side,back-side);graphics.fillPath(arrow);
    }
    if(signal>.00005f&&line.wanted){graphics.setColour(colour.withAlpha(line.alpha*std::clamp(signal*9.0f,.15f,.9f)));
        for(int i=0;i<2;++i){const auto position=path.getPointAlongPath(length*std::fmod(flowPhase+static_cast<float>(i)*.5f,1.0f));graphics.fillEllipse(position.x-2.4f/zoom,position.y-2.4f/zoom,4.8f/zoom,4.8f/zoom);}}
}
juce::String RoutingEditor::nodeDetail(int node) const {
    switch(node) {
        case 0:case 1:{const bool b=node==1;const std::array<juce::String,4> waves{"SINE","SAW","SQUARE","TRIANGLE"};return waves[static_cast<std::size_t>(std::clamp(static_cast<int>(store.get(b?Param::WaveB:Param::WaveA)),0,3))]+"  /  "+juce::String(store.get(b?Param::LevelB:Param::LevelA)*100,0)+"%";}
        case 2:return juce::String(store.get(Param::UnisonVoices),0)+" VOICES  /  "+juce::String(store.get(Param::Width)*100,0)+"% WIDTH";
        case 3:return juce::String(store.get(Param::Cutoff),0)+" Hz  /  LP";
        case 4:return store.get(Param::SaturationEnabled)>.5f?juce::String(store.get(Param::Drive),1)+" dB DRIVE":"BYPASS";
        case 5:return store.get(Param::ChorusEnabled)>.5f?juce::String(store.get(Param::ChorusMix)*100,0)+"% MIX":"BYPASS";
        case 6:return store.get(Param::DelayEnabled)>.5f?juce::String(store.get(Param::DelayTime)*1000,0)+" ms":"BYPASS";
        case 7:return store.get(Param::ReverbEnabled)>.5f?juce::String(store.get(Param::ReverbMix)*100,0)+"% MIX":"BYPASS";
        case 8:return "STEREO  /  "+juce::String(store.get(Param::Master),1)+" dB";
        case 9:case 10:return juce::String(store.get(node==9?Param::Lfo1Rate:Param::Lfo2Rate),2)+" Hz  /  MOD";
        default:return "A "+juce::String(store.get(Param::Attack),2)+"  R "+juce::String(store.get(Param::Release),2);
    }
}
void RoutingEditor::drawNode(juce::Graphics& graphics,int node) {
    const auto bounds=nodeBounds(node);const bool selected=node==selectedNode;
    graphics.setGradientFill(juce::ColourGradient(surface(),bounds.getTopLeft(),panelColour(),bounds.getBottomRight(),false));graphics.fillRoundedRectangle(bounds,8);
    graphics.setColour(selected?purple():juce::Colour(0xff3b384e));graphics.drawRoundedRectangle(bounds,8,(selected?1.6f:1.0f)/zoom);
    auto accent=node>=audioNodeCount?pink():purple();graphics.setColour(accent.withAlpha(.7f));graphics.fillRoundedRectangle(bounds.getX()+13,bounds.getY()+15,3,14,1.5f);
    graphics.setFont(juce::FontOptions(12.5f,juce::Font::bold));graphics.setColour(text());graphics.drawText(juce::String(nodeNames[static_cast<std::size_t>(node)].data()),bounds.toNearestInt().withTrimmedLeft(25).withHeight(42),juce::Justification::centredLeft);
    graphics.setColour(muted());graphics.setFont(juce::FontOptions(9.5f));graphics.drawText(nodeDetail(node),bounds.toNearestInt().reduced(13,0).withTrimmedTop(43).withHeight(18),juce::Justification::centredLeft);
    auto drawPort=[&](int port,bool output){const auto point=portPoint(node,port,output);const bool over=hovered.node==node&&hovered.port==port&&hovered.output==output;
        const bool validTarget=connecting&&!output&&compatible(dragSource,node,port);
        const auto colour=output?(node>=audioNodeCount?pink():purple()):(port>=2?pink():purple());
        graphics.setColour(background());graphics.fillEllipse(point.x-5,point.y-5,10,10);
        graphics.setColour(colour.withAlpha(over||validTarget?1.0f:.65f));graphics.drawEllipse(point.x-5,point.y-5,10,10,(over||validTarget?2.0f:1.3f)/zoom);
        if(output||over)graphics.fillEllipse(point.x-2,point.y-2,4,4);
        if(port>=2&&!output){const std::array<juce::String,4> labels{"PITCH","CUTOFF","LEVEL","WIDTH"};graphics.setFont(juce::FontOptions(8));graphics.drawText(labels[static_cast<std::size_t>(port-2)],juce::Rectangle<float>(point.x-22,point.y-18,44,12),juce::Justification::centred);}
    };
    for(int port:inputPorts(node))drawPort(port,false);if(node!=8)drawPort(0,true);
}
void RoutingEditor::paint(juce::Graphics& graphics) {
    graphics.setColour(background());graphics.fillRoundedRectangle(getLocalBounds().toFloat(),8);
    const auto area=canvas();graphics.setColour(panelColour().withAlpha(.45f));graphics.fillRect(area);
    graphics.saveState();graphics.reduceClipRegion(area.toNearestInt());graphics.addTransform(transform());
    const auto first=toWorld(area.getTopLeft()),last=toWorld(area.getBottomRight());
    graphics.setColour(muted().withAlpha(.12f));
    for(float x=std::floor(first.x/28)*28;x<last.x;x+=28)for(float y=std::floor(first.y/28)*28;y<last.y;y+=28)graphics.fillEllipse(x,y,1.5f/zoom,1.5f/zoom);
    for(const auto& line:visible)drawConnection(graphics,line);
    if(connecting&&dragSource>=0){graphics.setColour((hovered.valid()&&!hovered.output&&compatible(dragSource,hovered.node,hovered.port)?purple():muted()).withAlpha(.8f));graphics.strokePath(connectionPath(portPoint(dragSource,0,true),toWorld(pointer),dragSource>=audioNodeCount),juce::PathStrokeType(2/zoom));}
    for(int node=0;node<nodeCount;++node)if(graph.present[static_cast<std::size_t>(node)])drawNode(graphics,node);
    graphics.restoreState();
    graphics.setColour(applied?muted():pink());graphics.setFont(juce::FontOptions(11));
    graphics.drawText(status,8,getHeight()-25,std::max(0,getWidth()-90),22,juce::Justification::centredLeft);
    graphics.setColour(muted());graphics.drawText(juce::String(zoom*100,0)+"%",getWidth()-72,getHeight()-25,62,22,juce::Justification::centredRight);
}
void RoutingEditor::resized() {
    auto row=getLocalBounds().removeFromTop(40).reduced(6,6);
    constexpr std::array<int,8> widths{66,70,76,30,30,74,74,110};
    for(std::size_t i=0;i<buttons.size();++i)buttons[i].setBounds(row.removeFromLeft(widths[i]).reduced(2,0));
    if(fitPending)fitToView();
}
void RoutingEditor::fitToView() {
    if(canvas().getWidth()<20||canvas().getHeight()<20){fitPending=true;return;}
    juce::Rectangle<float> extent;bool first=true;
    for(int node=0;node<nodeCount;++node)if(graph.present[static_cast<std::size_t>(node)]){extent=first?nodeBounds(node):extent.getUnion(nodeBounds(node));first=false;}
    if(first)return;extent=extent.expanded(32);
    zoom=std::clamp(std::min(canvas().getWidth()/extent.getWidth(),canvas().getHeight()/extent.getHeight()),.25f,2.0f);
    pan=juce::Point<float>((canvas().getWidth()-extent.getWidth()*zoom)*.5f,(canvas().getHeight()-extent.getHeight()*zoom)*.5f)-extent.getPosition()*zoom;
    fitPending=false;updateButtons();repaint();
}
void RoutingEditor::zoomAt(float scale,juce::Point<float> point){const auto before=toWorld(point);zoom=std::clamp(scale,.25f,2.5f);pan=point-canvas().getPosition()-before*zoom;updateButtons();repaint();}
void RoutingEditor::autoLayout() {
    auto layout=graph;
    layout.x={35,35,230,425,620,815,815,620,425,230,525,815};
    layout.y={45,190,115,115,115,115,270,270,270,425,425,425};
    change(layout);fitToView();
}
void RoutingEditor::syncVisuals() {
    for(auto& line:visible)line.wanted=false;
    for(int i=0;i<graph.count;++i){const auto edge=graph.edges[static_cast<std::size_t>(i)];auto found=std::find_if(visible.begin(),visible.end(),[&](const auto& line){return line.connection==edge;});if(found==visible.end())visible.push_back({edge,0,true});else found->wanted=true;}
    if(visible.size()>static_cast<std::size_t>(maxEdges*3))visible.erase(std::remove_if(visible.begin(),visible.end(),[](const auto& line){return !line.wanted;}),visible.end());
}
void RoutingEditor::publish() {
    std::string error;applied=store.setGraph(graph,error);
    status=applied?utf8("Схема активна · нод → инспектор · порт → соединение"):
        utf8("ЧЕРНОВИК: ")+juce::String::fromUTF8(error.c_str())+utf8(". Звучит последняя валидная схема.");
    if(applied&&onChange)onChange();
    syncVisuals();updateButtons();repaint();
}
void RoutingEditor::change(const RoutingGraph& next) {
    if(next==graph)return;
    if(undoHistory.size()==64)undoHistory.erase(undoHistory.begin());undoHistory.push_back(graph);redoHistory.clear();graph=next;
    selectedEdge=-1;publish();
}
bool RoutingEditor::createConnection(Connection edge,int oldEdge) {
    if(!compatible(edge.from,edge.to,edge.port)){status=utf8("Несовместимые порты: аудио и модуляции имеют разные входы");repaint();return false;}
    auto candidate=graph;if(oldEdge>=0)disconnect(candidate,oldEdge);
    for(int i=candidate.count-1;i>=0;--i){const auto existing=candidate.edges[static_cast<std::size_t>(i)];
        if(existing==edge)return false;
        if(edge.from<audioNodeCount&&existing.from<audioNodeCount&&existing.to==edge.to&&existing.port==edge.port)disconnect(candidate,i);
    }
    if(candidate.count>=maxEdges){status=utf8("Достигнут лимит соединений");repaint();return false;}
    candidate.edges[static_cast<std::size_t>(candidate.count++)]=edge;
    std::array<int,audioNodeCount> degrees{},queue{};int write=0,read=0,visited=0;
    for(int i=0;i<candidate.count;++i){const auto e=candidate.edges[static_cast<std::size_t>(i)];if(e.from<audioNodeCount)++degrees[e.to];}
    for(int i=0;i<audioNodeCount;++i)if(degrees[static_cast<std::size_t>(i)]==0)queue[static_cast<std::size_t>(write++)]=i;
    while(read<write){const int node=queue[static_cast<std::size_t>(read++)];++visited;for(int i=0;i<candidate.count;++i){const auto e=candidate.edges[static_cast<std::size_t>(i)];if(e.from==node&&--degrees[e.to]==0)queue[static_cast<std::size_t>(write++)]=e.to;}}
    if(visited!=audioNodeCount){status=utf8("Аудиоцикл запрещён; соединение не добавлено");repaint();return false;}
    change(candidate);return true;
}
void RoutingEditor::removeSelectedConnection(){if(selectedEdge<0||selectedEdge>=graph.count)return;auto candidate=graph;disconnect(candidate,selectedEdge);change(candidate);}
void RoutingEditor::undo(){if(undoHistory.empty())return;if(redoHistory.size()==64)redoHistory.erase(redoHistory.begin());redoHistory.push_back(graph);graph=undoHistory.back();undoHistory.pop_back();selectedEdge=-1;publish();}
void RoutingEditor::redo(){if(redoHistory.empty())return;if(undoHistory.size()==64)undoHistory.erase(undoHistory.begin());undoHistory.push_back(graph);graph=redoHistory.back();redoHistory.pop_back();selectedEdge=-1;publish();}
void RoutingEditor::updateButtons(){buttons[3].setEnabled(zoom>.251f);buttons[4].setEnabled(zoom<2.499f);buttons[5].setEnabled(!undoHistory.empty());buttons[6].setEnabled(!redoHistory.empty());buttons[7].setEnabled(selectedEdge>=0&&selectedEdge<graph.count);}
void RoutingEditor::mouseDown(const juce::MouseEvent& event) {
    if(!canvas().contains(event.position))return;grabKeyboardFocus();pointer=event.position;mouseStart=event.position;
    const auto world=toWorld(event.position);hovered=portAt(world);const int previousSelection=selectedEdge;selectedEdge=-1;
    if(event.mods.isMiddleButtonDown()){panning=true;panStart=pan;return;}
    if(hovered.valid()&&!event.mods.isRightButtonDown()){
        if(hovered.output){connecting=true;dragSource=hovered.node;reconnectEdge=-1;}
        else {
            if(previousSelection>=0&&previousSelection<graph.count){const auto edge=graph.edges[static_cast<std::size_t>(previousSelection)];if(edge.to==hovered.node&&edge.port==hovered.port){connecting=true;dragSource=edge.from;reconnectEdge=previousSelection;}}
            if(!connecting)for(int i=0;i<graph.count;++i){const auto edge=graph.edges[static_cast<std::size_t>(i)];if(edge.to==hovered.node&&edge.port==hovered.port){connecting=true;dragSource=edge.from;reconnectEdge=i;break;}}
        }
        repaint();return;
    }
    const int node=nodeAt(world);
    if(node>=0&&!event.mods.isRightButtonDown()){selectedNode=dragNode=node;dragOriginal=graph;nodeStart={graph.x[static_cast<std::size_t>(node)],graph.y[static_cast<std::size_t>(node)]};if(onSelect)onSelect(node);updateButtons();repaint();return;}
    selectedEdge=connectionAt(world);
    if(selectedEdge>=0){selectedNode=-1;if(event.mods.isRightButtonDown())removeSelectedConnection();updateButtons();repaint();return;}
    selectedNode=-1;if(event.mods.isRightButtonDown()){panning=true;panStart=pan;}updateButtons();repaint();
}
void RoutingEditor::mouseDrag(const juce::MouseEvent& event) {
    pointer=event.position;
    if(panning){pan=panStart+event.position-mouseStart;repaint();return;}
    if(connecting){hovered=portAt(toWorld(event.position));repaint();return;}
    if(dragNode>=0){const auto position=nodeStart+(event.position-mouseStart)/zoom;graph.x[static_cast<std::size_t>(dragNode)]=std::clamp(position.x,-9000.0f,9000.0f);graph.y[static_cast<std::size_t>(dragNode)]=std::clamp(position.y,-9000.0f,9000.0f);repaint();}
}
void RoutingEditor::mouseUp(const juce::MouseEvent& event) {
    if(connecting){const auto target=portAt(toWorld(event.position));if(target.valid()&&!target.output)createConnection({dragSource,target.node,target.port},reconnectEdge);
        else if(reconnectEdge>=0){selectedEdge=reconnectEdge;removeSelectedConnection();}
        connecting=false;dragSource=reconnectEdge=-1;
    }
    if(dragNode>=0){const auto moved=graph;graph=dragOriginal;dragNode=-1;change(moved);}
    panning=false;hovered=portAt(toWorld(event.position));updateButtons();repaint();
}
void RoutingEditor::mouseMove(const juce::MouseEvent& event){pointer=event.position;hovered=canvas().contains(event.position)?portAt(toWorld(event.position)):PortHit{};repaint();}
void RoutingEditor::mouseExit(const juce::MouseEvent&){if(!connecting){hovered={};repaint();}}
void RoutingEditor::mouseWheelMove(const juce::MouseEvent& event,const juce::MouseWheelDetails& wheel){if(canvas().contains(event.position))zoomAt(zoom*std::exp(wheel.deltaY*1.4f),event.position);}
void RoutingEditor::mouseDoubleClick(const juce::MouseEvent& event){if(canvas().contains(event.position)&&nodeAt(toWorld(event.position))<0)fitToView();}
bool RoutingEditor::keyPressed(const juce::KeyPress& key){
    if(key.getKeyCode()==juce::KeyPress::deleteKey||key.getKeyCode()==juce::KeyPress::backspaceKey){removeSelectedConnection();return true;}
    if(key.getModifiers().isCommandDown()&&(key.getKeyCode()=='Z'||key.getKeyCode()=='z')){if(key.getModifiers().isShiftDown())redo();else undo();return true;}
    if(key.getModifiers().isCommandDown()&&(key.getKeyCode()=='Y'||key.getKeyCode()=='y')){redo();return true;}return false;
}
void RoutingEditor::focusLost(FocusChangeType){if(dragNode>=0)graph=dragOriginal;dragNode=dragSource=reconnectEdge=-1;connecting=panning=false;hovered={};repaint();}
juce::String RoutingEditor::getTooltip(){
    if(hovered.valid()){if(hovered.output)return hovered.node>=audioNodeCount?utf8("Выход модуляции: перетащите к розовому входу"):utf8("Аудиовыход: перетащите к фиолетовому входу");
        const std::array<juce::String,6> names{utf8("Аудиовход"),utf8("Второй аудиовход"),utf8("Модуляция высоты"),utf8("Модуляция cutoff"),utf8("Модуляция уровня"),utf8("Модуляция стереоширины")};return names[static_cast<std::size_t>(hovered.port)]+utf8(". Подключённый вход можно перетащить для перенаправления.");}
    return utf8("Колесо — масштаб; средняя/правая кнопка на фоне — панорама; Delete — удалить связь. Анимация показывает активность общего выхода.");
}
void RoutingEditor::timerCallback(){
    activity*=.86f;flowPhase=std::fmod(flowPhase+.018f,1.0f);
    for(auto& line:visible)line.alpha+=std::clamp((line.wanted?1.0f:0.0f)-line.alpha,-.13f,.13f);
    visible.erase(std::remove_if(visible.begin(),visible.end(),[](const auto& line){return !line.wanted&&line.alpha<.001f;}),visible.end());
    if(isShowing())repaint(canvas().toNearestInt());
}
}
