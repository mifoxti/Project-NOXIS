#include "state/Preset.h"
#include <initializer_list>
#include <utility>

namespace noxis {
const std::vector<Preset>& factoryPresets() {
    static const auto library=[] {
        using P=Param;
        std::vector<Preset> result;
        result.reserve(36);
        auto add=[&](const char* name,const char* category,const char* description,const char* tags,
                     const char* character,int low,int high,std::initializer_list<std::pair<P,float>> settings)->Preset& {
            Preset p;
            p.name=name;p.category=category;p.description=description;p.tags=tags;p.character=character;
            p.lowNote=low;p.highNote=high;
            for(auto [parameter,value]:settings)p.values[index(parameter)]=value;
            result.push_back(std::move(p));return result.back();
        };

        // Первые четыре тембра сохраняют исходные значения Phase 1.
        add("Init","INIT","Нейтральная отправная точка для собственного звука.","init,neutral,dry","Чистый, прямой",36,84,{});
        add("Midnight Reese","BASS","Исходный тёмный бас из двух расстроенных пил.","bass,reese,dark,legacy","Пульсирующий, плотный",36,60,
            {{P::WaveA,1},{P::WaveB,1},{P::OctaveA,-1},{P::OctaveB,-1},{P::DetuneA,-13},{P::DetuneB,13},
             {P::LevelB,.7f},{P::Cutoff,850},{P::Resonance,.3f},{P::Attack,.035f},{P::Release,.45f},{P::Drive,8}});
        add("Afterglow Lead","LEAD","Исходный мягкий лид с плавной атакой.","lead,warm,legacy","Мягкий, задумчивый",48,84,
            {{P::WaveA,3},{P::WaveB,1},{P::DetuneB,7},{P::LevelB,.23f},{P::Attack,.16f},{P::Decay,.6f},
             {P::Sustain,.65f},{P::Release,1.4f},{P::Cutoff,3400},{P::Resonance,.2f},{P::Drive,3}});
        add("Nightdrive Bass","BASS","Исходный низкий square/sine-бас с насыщением.","bass,sub,dark,legacy","Низкий, собранный",36,60,
            {{P::WaveA,2},{P::WaveB,0},{P::OctaveA,-1},{P::OctaveB,-2},{P::LevelB,.8f},{P::Cutoff,500},
             {P::Resonance,.18f},{P::Drive,12},{P::Attack,.006f},{P::Decay,.18f},{P::Sustain,.6f},{P::Release,.12f}});

        add("Midnight Reese II","BASS","Широкие пилы, чистый суб и медленное дыхание фильтра.","bass,reese,wide,motion","Тёмный, текучий",36,60,
            {{P::WaveA,1},{P::WaveB,1},{P::OctaveA,-1},{P::OctaveB,-1},{P::DetuneA,-9},{P::DetuneB,9},{P::LevelB,.62f},
             {P::UnisonVoices,4},{P::UnisonDetune,14},{P::Spread,.75f},{P::Width,.62f},{P::PhaseRandom,.45f},{P::SubLevel,.27f},
             {P::Cutoff,1100},{P::Resonance,.24f},{P::Lfo1Rate,.32f},{P::Lfo1Cutoff,.65f},{P::Lfo2Rate,.18f},{P::Lfo2Width,.15f},
             {P::Attack,.035f},{P::Release,.48f},{P::Drive,9},{P::Output,-5}});
        add("Sub Pressure","BASS","Монофонический синусовый фундамент с плавным переходом нот.","bass,sub,mono,glide","Глубокий, устойчивый",36,55,
            {{P::WaveA,0},{P::WaveB,0},{P::OctaveA,-1},{P::LevelA,.86f},{P::LevelB,0},{P::SubLevel,.22f},{P::Cutoff,180},
             {P::Mono,1},{P::Legato,1},{P::Glide,.13f},{P::Attack,.009f},{P::Decay,.4f},{P::Sustain,.82f},{P::Release,.12f},
             {P::Drive,4},{P::Width,0},{P::Output,-2}});
        add("Nightdrive 808","BASS","Затухающий синусовый бас с коротким нисходящим движением высоты.","bass,808,mono,decay","Упругий, ударный",36,60,
            {{P::WaveA,0},{P::OctaveA,-1},{P::WaveB,3},{P::OctaveB,-1},{P::LevelA,.9f},{P::LevelB,.12f},{P::SubLevel,.18f},
             {P::Mono,1},{P::Glide,.07f},{P::Cutoff,1200},{P::Lfo1Wave,2},{P::Lfo1Rate,.7f},{P::Lfo1Pitch,-2.5f},
             {P::Attack,.003f},{P::Decay,.75f},{P::Sustain,0},{P::Release,.14f},{P::Drive,12},{P::Width,0},{P::Output,-5}});
        add("Distorted Reese","BASS","Сжатые расстроенные пилы с резонансным движением и сильным drive.","bass,reese,distorted,aggressive","Шершавый, напористый",36,58,
            {{P::WaveA,1},{P::WaveB,1},{P::OctaveA,-1},{P::OctaveB,-1},{P::DetuneA,-21},{P::DetuneB,21},{P::LevelB,.72f},
             {P::UnisonVoices,3},{P::UnisonDetune,19},{P::Width,.4f},{P::SubLevel,.16f},{P::Cutoff,1750},{P::Resonance,.48f},
             {P::Lfo1Wave,1},{P::Lfo1Rate,1.7f},{P::Lfo1Cutoff,.85f},{P::EnvCutoff,1.3f},{P::Attack,.008f},{P::Decay,.32f},
             {P::Sustain,.68f},{P::Release,.2f},{P::Drive,19},{P::SaturationMix,.86f},{P::Output,-8}});
        add("Wide Dark Bass","BASS","Тёмный стереобас с шестью unison-слоями и мягким chorus.","bass,wide,unison,chorus","Объёмный, мягкий",38,62,
            {{P::WaveA,1},{P::WaveB,3},{P::OctaveA,-1},{P::OctaveB,-1},{P::LevelB,.45f},{P::UnisonVoices,6},{P::UnisonDetune,17},
             {P::Spread,1},{P::Width,.95f},{P::PhaseRandom,.7f},{P::Cutoff,690},{P::Resonance,.2f},{P::Attack,.09f},
             {P::Sustain,.78f},{P::Release,.65f},{P::ChorusEnabled,1},{P::ChorusRate,.24f},{P::ChorusDepth,.28f},{P::ChorusMix,.18f},
             {P::Drive,5},{P::Output,-5}});
        add("Deep Motion","BASS","Субовый пульс с двумя скоростями модуляции фильтра и амплитуды.","bass,sub,pulse,lfo","Гипнотический, ритмичный",36,60,
            {{P::WaveA,3},{P::WaveB,1},{P::OctaveA,-1},{P::OctaveB,-2},{P::LevelB,.35f},{P::SubLevel,.3f},{P::Cutoff,460},
             {P::Resonance,.43f},{P::Lfo1Wave,1},{P::Lfo1Rate,2.2f},{P::Lfo1Cutoff,1.8f},{P::Lfo2Wave,0},{P::Lfo2Rate,.55f},
             {P::Lfo2Level,.32f},{P::Attack,.025f},{P::Release,.28f},{P::Drive,7},{P::Width,.15f}});
        add("Analog Sub","BASS","Треугольный низ с тихим синусовым слоем и округлым перегрузом.","bass,sub,analog,dry","Округлый, тёплый",36,60,
            {{P::WaveA,3},{P::WaveB,0},{P::OctaveA,-1},{P::OctaveB,-2},{P::LevelA,.82f},{P::LevelB,.36f},{P::Cutoff,310},
             {P::Resonance,.05f},{P::Mono,1},{P::Attack,.014f},{P::Decay,.24f},{P::Sustain,.76f},{P::Release,.2f},
             {P::Drive,3},{P::SaturationMix,.6f},{P::Width,0},{P::Output,-1}});
        add("Phonk Growl","BASS","Резонансный square/saw с быстрой модуляцией и зернистым верхом.","bass,growl,phonk,rough","Рычащий, нервный",36,62,
            {{P::WaveA,2},{P::WaveB,1},{P::OctaveA,-1},{P::OctaveB,-1},{P::SemitoneB,7},{P::LevelB,.42f},{P::NoiseLevel,.035f},
             {P::UnisonVoices,2},{P::UnisonDetune,8},{P::Width,.3f},{P::Cutoff,1250},{P::Resonance,.68f},
             {P::Lfo1Wave,2},{P::Lfo1Rate,4.2f},{P::Lfo1Cutoff,1.1f},{P::Lfo2Rate,.37f},{P::Lfo2Pitch,.08f},
             {P::Attack,.006f},{P::Decay,.25f},{P::Sustain,.58f},{P::Release,.16f},{P::Drive,16},{P::Output,-7}});

        add("Afterglow Lead II","LEAD","Певучий стереолид с лёгким вибрато и длинным пространством.","lead,atmospheric,wide,soft","Меланхоличный, воздушный",55,84,
            {{P::WaveA,3},{P::WaveB,1},{P::LevelB,.22f},{P::UnisonVoices,4},{P::UnisonDetune,8},{P::Width,.78f},{P::PhaseRandom,.3f},
             {P::Cutoff,3800},{P::Attack,.13f},{P::Decay,.65f},{P::Sustain,.72f},{P::Release,1.3f},{P::Lfo1Rate,4.7f},{P::Lfo1Pitch,.07f},
             {P::ChorusEnabled,1},{P::ChorusMix,.15f},{P::DelayEnabled,1},{P::DelayTime,.375f},{P::DelayMix,.17f},{P::DelayFeedback,.33f},
             {P::ReverbEnabled,1},{P::ReverbMix,.23f},{P::ReverbSize,.65f},{P::Drive,2},{P::Output,-4}});
        add("Neon Tears","LEAD","Тонкий square-лид, плавные переливы и заметные стереоповторы.","lead,square,delay,glide","Светящийся, печальный",55,88,
            {{P::WaveA,2},{P::WaveB,3},{P::LevelB,.18f},{P::Mono,1},{P::Legato,1},{P::Glide,.11f},{P::UnisonVoices,2},
             {P::UnisonDetune,5},{P::Cutoff,2600},{P::Resonance,.28f},{P::EnvCutoff,.65f},{P::Attack,.026f},{P::Release,.72f},
             {P::Lfo1Rate,5.3f},{P::Lfo1Pitch,.09f},{P::DelayEnabled,1},{P::DelayTime,.28f},{P::DelayFeedback,.46f},{P::DelayMix,.28f},
             {P::ReverbEnabled,1},{P::ReverbSize,.6f},{P::ReverbMix,.16f},{P::Drive,5},{P::Output,-5}});
        add("Cold Horizon","LEAD","Ясный синусовый лид с верхней октавой и холодным chorus.","lead,sine,octave,cold","Прозрачный, отстранённый",55,88,
            {{P::WaveA,0},{P::WaveB,3},{P::OctaveB,1},{P::LevelA,.8f},{P::LevelB,.23f},{P::Cutoff,7200},{P::Attack,.045f},
             {P::Decay,.7f},{P::Sustain,.56f},{P::Release,1},{P::UnisonVoices,3},{P::UnisonDetune,6},{P::Spread,.9f},
             {P::ChorusEnabled,1},{P::ChorusRate,.63f},{P::ChorusDepth,.48f},{P::ChorusMix,.35f},
             {P::ReverbEnabled,1},{P::ReverbSize,.78f},{P::ReverbDamping,.28f},{P::ReverbMix,.25f},{P::Drive,0}});
        add("Night Echo","LEAD","Короткая атака пилы, резонансная огибающая и пунктирное эхо.","lead,saw,echo,resonant","Дальний, отчётливый",48,84,
            {{P::WaveA,1},{P::WaveB,0},{P::SemitoneB,7},{P::LevelB,.16f},{P::Cutoff,1500},{P::Resonance,.48f},{P::EnvCutoff,2.4f},
             {P::Attack,.007f},{P::Decay,.27f},{P::Sustain,.35f},{P::Release,.45f},{P::UnisonVoices,2},{P::UnisonDetune,9},
             {P::DelayEnabled,1},{P::DelayTime,.45f},{P::DelayFeedback,.52f},{P::DelayMix,.31f},{P::DelayStereo,1},
             {P::ReverbEnabled,1},{P::ReverbMix,.12f},{P::Drive,4},{P::Output,-4}});
        add("Melancholic Lead","LEAD","Узкий тёплый лид с legato и медленным движением тембра.","lead,mono,warm,legato","Камерный, задумчивый",48,84,
            {{P::WaveA,3},{P::WaveB,1},{P::OctaveB,-1},{P::LevelB,.12f},{P::Mono,1},{P::Legato,1},{P::Glide,.2f},{P::Width,.18f},
             {P::Cutoff,1850},{P::Resonance,.15f},{P::Lfo1Rate,.21f},{P::Lfo1Cutoff,.32f},{P::Lfo2Rate,4.9f},{P::Lfo2Pitch,.045f},
             {P::Attack,.09f},{P::Decay,.4f},{P::Sustain,.75f},{P::Release,.95f},{P::ReverbEnabled,1},{P::ReverbSize,.48f},
             {P::ReverbDamping,.7f},{P::ReverbMix,.22f},{P::Drive,3}});
        add("Ghost Signal","LEAD","Тихий шумовой ореол, sample-and-hold и размытые повторы.","lead,ghost,random,texture","Призрачный, нестабильный",55,84,
            {{P::WaveA,0},{P::WaveB,2},{P::OctaveB,1},{P::LevelB,.13f},{P::NoiseLevel,.045f},{P::UnisonVoices,3},{P::UnisonDetune,12},
             {P::Cutoff,2300},{P::Resonance,.32f},{P::Lfo1Wave,4},{P::Lfo1Rate,2.1f},{P::Lfo1Cutoff,.55f},{P::Lfo2Rate,.4f},{P::Lfo2Width,.25f},
             {P::Attack,.18f},{P::Release,1.7f},{P::DelayEnabled,1},{P::DelayTime,.61f},{P::DelayMix,.22f},{P::DelayFeedback,.43f},
             {P::ReverbEnabled,1},{P::ReverbSize,.82f},{P::ReverbMix,.32f},{P::Drive,1},{P::Output,-4}});

        add("Empty Streets","PAD","Широкая медленная пила, мягкий chorus и тёплая реверберация.","pad,wave,wide,warm","Ночной, спокойный",43,76,
            {{P::WaveA,1},{P::WaveB,3},{P::OctaveB,-1},{P::LevelB,.34f},{P::UnisonVoices,6},{P::UnisonDetune,13},{P::Spread,1},{P::PhaseRandom,.75f},
             {P::Cutoff,1650},{P::Resonance,.14f},{P::Attack,1.3f},{P::Decay,1.1f},{P::Sustain,.74f},{P::Release,3.2f},{P::Lfo1Rate,.11f},{P::Lfo1Cutoff,.5f},
             {P::ChorusEnabled,1},{P::ChorusRate,.22f},{P::ChorusMix,.26f},{P::ReverbEnabled,1},{P::ReverbSize,.86f},{P::ReverbDamping,.65f},{P::ReverbMix,.36f},{P::Output,-7}});
        add("Distant Memories","PAD","Мягкая треугольная основа с квинтой и плавающей стереошириной.","pad,soft,fifth,nostalgic","Тёплый, ностальгический",43,76,
            {{P::WaveA,3},{P::WaveB,0},{P::SemitoneB,7},{P::LevelB,.3f},{P::UnisonVoices,4},{P::UnisonDetune,7},{P::PhaseRandom,.5f},
             {P::Cutoff,2700},{P::Attack,1.8f},{P::Decay,1.5f},{P::Sustain,.8f},{P::Release,4.2f},{P::Lfo1Rate,.17f},{P::Lfo1Width,.28f},
             {P::Lfo2Rate,.09f},{P::Lfo2Pitch,.055f},{P::DelayEnabled,1},{P::DelayTime,.72f},{P::DelayFeedback,.25f},{P::DelayMix,.12f},
             {P::ReverbEnabled,1},{P::ReverbSize,.9f},{P::ReverbMix,.38f},{P::Drive,1},{P::Output,-5}});
        add("Dark Atmosphere","PAD","Низкий шумовой слой и медленная резонансная модуляция.","pad,dark,noise,cinematic","Густой, тревожный",36,67,
            {{P::WaveA,2},{P::WaveB,1},{P::OctaveA,-1},{P::OctaveB,-1},{P::LevelB,.35f},{P::NoiseLevel,.13f},{P::UnisonVoices,3},
             {P::UnisonDetune,22},{P::Width,.85f},{P::Cutoff,610},{P::Resonance,.49f},{P::Attack,2},{P::Decay,1.3f},{P::Sustain,.72f},
             {P::Release,4.8f},{P::Lfo1Rate,.075f},{P::Lfo1Cutoff,1.15f},{P::Lfo2Rate,.12f},{P::Lfo2Level,.18f},
             {P::ReverbEnabled,1},{P::ReverbSize,.95f},{P::ReverbDamping,.78f},{P::ReverbMix,.42f},{P::Drive,7},{P::Output,-7}});
        add("Frozen Air","PAD","Высокий синусовый слой, воздух и медленный холодный chorus.","pad,air,cold,bright","Ледяной, невесомый",48,84,
            {{P::WaveA,0},{P::WaveB,3},{P::OctaveB,1},{P::LevelA,.68f},{P::LevelB,.25f},{P::NoiseLevel,.09f},{P::UnisonVoices,5},
             {P::UnisonDetune,9},{P::PhaseRandom,.9f},{P::Cutoff,5900},{P::Attack,1.1f},{P::Decay,1},{P::Sustain,.66f},{P::Release,3.7f},
             {P::Lfo1Rate,.23f},{P::Lfo1Cutoff,.35f},{P::ChorusEnabled,1},{P::ChorusRate,.19f},{P::ChorusDepth,.7f},{P::ChorusMix,.42f},
             {P::ReverbEnabled,1},{P::ReverbSize,.83f},{P::ReverbDamping,.22f},{P::ReverbMix,.4f},{P::Drive,0},{P::Output,-6}});
        add("Midnight Fog","PAD","Тёмные пилы, октавный низ и длинный приглушённый шлейф.","pad,fog,low,dark","Обволакивающий, медленный",36,72,
            {{P::WaveA,1},{P::WaveB,1},{P::OctaveB,-1},{P::LevelB,.24f},{P::UnisonVoices,8},{P::UnisonDetune,18},{P::PhaseRandom,1},
             {P::Cutoff,930},{P::Resonance,.25f},{P::Attack,2.3f},{P::Decay,1.8f},{P::Sustain,.84f},{P::Release,5.2f},
             {P::Lfo1Wave,1},{P::Lfo1Rate,.06f},{P::Lfo1Cutoff,.7f},{P::Lfo2Rate,.14f},{P::Lfo2Width,.2f},
             {P::ChorusEnabled,1},{P::ChorusMix,.18f},{P::ReverbEnabled,1},{P::ReverbSize,.96f},{P::ReverbDamping,.86f},{P::ReverbMix,.48f},{P::Output,-8}});

        add("Glass Rain","PLUCK","Короткий синусовый щипок с верхним призвуком и лёгким эхом.","pluck,glass,sine,echo","Хрупкий, чистый",55,88,
            {{P::WaveA,0},{P::WaveB,3},{P::OctaveB,1},{P::LevelA,.86f},{P::LevelB,.3f},{P::Cutoff,6400},{P::Attack,.003f},
             {P::Decay,.23f},{P::Sustain,0},{P::Release,.18f},{P::EnvCutoff,1.4f},{P::DelayEnabled,1},{P::DelayTime,.24f},
             {P::DelayFeedback,.32f},{P::DelayMix,.25f},{P::ReverbEnabled,1},{P::ReverbSize,.62f},{P::ReverbMix,.2f},{P::Drive,0}});
        add("Broken Neon","PLUCK","Грубоватый square-щипок с пятой гармонической опорой.","pluck,square,grit,phonk","Резкий, прерывистый",48,84,
            {{P::WaveA,2},{P::WaveB,1},{P::SemitoneB,7},{P::LevelB,.22f},{P::Cutoff,1900},{P::Resonance,.52f},{P::EnvCutoff,2.7f},
             {P::Attack,.002f},{P::Decay,.14f},{P::Sustain,.035f},{P::Release,.095f},{P::Drive,13},{P::SaturationMix,.75f},
             {P::DelayEnabled,1},{P::DelayTime,.19f},{P::DelayFeedback,.25f},{P::DelayMix,.18f},{P::Output,-5}});
        add("Hollow Keys","PLUCK","Полый треугольный тембр с короткой огибающей и малой комнатой.","pluck,keys,hollow,soft","Сухой, деревянный",48,84,
            {{P::WaveA,3},{P::WaveB,0},{P::OctaveB,-1},{P::LevelB,.28f},{P::Cutoff,2200},{P::Resonance,.37f},{P::EnvCutoff,1.1f},
             {P::Attack,.005f},{P::Decay,.36f},{P::Sustain,.08f},{P::Release,.25f},{P::ReverbEnabled,1},{P::ReverbSize,.28f},
             {P::ReverbDamping,.77f},{P::ReverbMix,.17f},{P::Drive,2},{P::Width,.25f}});
        add("Crystal Motion","PLUCK","Широкий яркий щипок со ступенчатым движением фильтра.","pluck,crystal,random,stereo","Искристый, живой",55,91,
            {{P::WaveA,3},{P::WaveB,1},{P::OctaveB,1},{P::LevelB,.18f},{P::UnisonVoices,3},{P::UnisonDetune,12},{P::Spread,1},
             {P::Cutoff,4300},{P::Resonance,.28f},{P::EnvCutoff,1.7f},{P::Lfo1Wave,4},{P::Lfo1Rate,5.5f},{P::Lfo1Cutoff,.36f},
             {P::Attack,.002f},{P::Decay,.31f},{P::Sustain,0},{P::Release,.23f},{P::DelayEnabled,1},{P::DelayTime,.32f},
             {P::DelayFeedback,.4f},{P::DelayMix,.27f},{P::ReverbEnabled,1},{P::ReverbMix,.24f},{P::Drive,1},{P::Output,-4}});

        add("Dark Cowbell","PHONK","Два square-осциллятора с негармонической настройкой и коротким спадом.","phonk,cowbell,metallic,dry","Металлический, тёмный",60,84,
            {{P::WaveA,2},{P::WaveB,2},{P::OctaveA,1},{P::OctaveB,1},{P::SemitoneB,6.7f},{P::LevelA,.58f},{P::LevelB,.47f},
             {P::Cutoff,5200},{P::Resonance,.13f},{P::Attack,.002f},{P::Decay,.19f},{P::Sustain,0},{P::Release,.08f},
             {P::Drive,8},{P::SaturationMix,.75f},{P::Width,0},{P::Output,-4}});
        add("Drift Bell","PHONK","Более яркий и жёсткий колокольный удар с коротким стереоэхом.","phonk,bell,drift,bright","Жёсткий, звонкий",60,88,
            {{P::WaveA,2},{P::WaveB,2},{P::OctaveA,1},{P::OctaveB,1},{P::SemitoneB,7.25f},{P::LevelA,.66f},{P::LevelB,.5f},
             {P::Cutoff,8900},{P::Resonance,.27f},{P::Attack,.002f},{P::Decay,.27f},{P::Sustain,.015f},{P::Release,.11f},
             {P::Drive,16},{P::DelayEnabled,1},{P::DelayTime,.135f},{P::DelayFeedback,.22f},{P::DelayMix,.14f},
             {P::ReverbEnabled,1},{P::ReverbSize,.32f},{P::ReverbMix,.1f},{P::Output,-7}});
        add("Metallic Phonk","PHONK","Негармоническая sine/square-пара с резонансным фильтром и насыщением.","phonk,metal,resonant,sharp","Колкий, напряжённый",55,86,
            {{P::WaveA,0},{P::WaveB,2},{P::OctaveA,1},{P::OctaveB,1},{P::SemitoneB,10.4f},{P::LevelA,.8f},{P::LevelB,.34f},
             {P::Cutoff,4100},{P::Resonance,.6f},{P::EnvCutoff,1.6f},{P::Attack,.002f},{P::Decay,.34f},{P::Sustain,0},{P::Release,.16f},
             {P::Drive,11},{P::ReverbEnabled,1},{P::ReverbSize,.46f},{P::ReverbDamping,.3f},{P::ReverbMix,.2f},{P::Output,-5}});
        add("Haunted Melody","PHONK","Мягкий мелодический колокольчик с медленным вибрато и дальним пространством.","phonk,bell,atmospheric,haunted","Призрачный, мелодичный",55,88,
            {{P::WaveA,3},{P::WaveB,0},{P::OctaveB,1},{P::SemitoneB,.12f},{P::LevelB,.42f},{P::UnisonVoices,2},{P::UnisonDetune,5},
             {P::Cutoff,3600},{P::Attack,.008f},{P::Decay,.65f},{P::Sustain,.04f},{P::Release,.8f},{P::Lfo1Rate,3.8f},{P::Lfo1Pitch,.08f},
             {P::DelayEnabled,1},{P::DelayTime,.42f},{P::DelayFeedback,.41f},{P::DelayMix,.23f},{P::ReverbEnabled,1},{P::ReverbSize,.82f},
             {P::ReverbMix,.35f},{P::Drive,2},{P::Output,-3}});
        add("Tape Night","PHONK","Приглушённый треугольный тембр с шумом и плаванием высоты.","phonk,tape,wow,noise","Потёртый, тёплый",48,84,
            {{P::WaveA,3},{P::WaveB,0},{P::OctaveB,-1},{P::LevelB,.19f},{P::NoiseLevel,.055f},{P::Cutoff,1850},{P::Attack,.018f},
             {P::Decay,.5f},{P::Sustain,.32f},{P::Release,.6f},{P::Lfo1Rate,.46f},{P::Lfo1Pitch,.13f},{P::Lfo2Rate,6.7f},{P::Lfo2Pitch,.025f},
             {P::ChorusEnabled,1},{P::ChorusRate,.13f},{P::ChorusDepth,.2f},{P::ChorusMix,.17f},{P::ReverbEnabled,1},
             {P::ReverbSize,.52f},{P::ReverbDamping,.78f},{P::ReverbMix,.24f},{P::Drive,7},{P::Output,-4}});

        add("Rising Tension","FX","Длинная атака, восходящая модуляция фильтра и шумовой ореол.","fx,riser,noise,cinematic","Нарастающий, тревожный",36,72,
            {{P::WaveA,1},{P::WaveB,2},{P::SemitoneB,1},{P::LevelB,.2f},{P::NoiseLevel,.24f},{P::UnisonVoices,4},{P::UnisonDetune,28},
             {P::Cutoff,1200},{P::Resonance,.54f},{P::Attack,2.1f},{P::Decay,.5f},{P::Sustain,.78f},{P::Release,1.6f},
             {P::Lfo1Wave,2},{P::Lfo1Rate,.15f},{P::Lfo1Cutoff,2.7f},{P::Lfo2Rate,5},{P::Lfo2Level,.22f},
             {P::ReverbEnabled,1},{P::ReverbSize,.86f},{P::ReverbMix,.34f},{P::Drive,8},{P::Output,-7}});
        add("Digital Ghost","FX","Ступенчатая модуляция высоты и фильтра над тонким синусовым тоном.","fx,digital,random,ghost","Ломаный, электронный",48,84,
            {{P::WaveA,0},{P::WaveB,2},{P::OctaveB,1},{P::LevelB,.1f},{P::NoiseLevel,.035f},{P::Cutoff,2800},{P::Resonance,.46f},
             {P::Lfo1Wave,4},{P::Lfo1Rate,7.3f},{P::Lfo1Pitch,2.4f},{P::Lfo2Wave,4},{P::Lfo2Rate,2.4f},{P::Lfo2Cutoff,1.1f},
             {P::Attack,.12f},{P::Decay,.6f},{P::Sustain,.6f},{P::Release,1.4f},{P::DelayEnabled,1},{P::DelayTime,.22f},
             {P::DelayFeedback,.56f},{P::DelayMix,.31f},{P::ReverbEnabled,1},{P::ReverbSize,.7f},{P::ReverbMix,.23f},{P::Drive,3},{P::Output,-5}});
        add("Noise Sweep","FX","Настоящий шумовой источник, резонансный sweep и длинный шлейф.","fx,noise,sweep,texture","Шумящий, воздушный",36,84,
            {{P::WaveA,0},{P::WaveB,0},{P::LevelA,.025f},{P::LevelB,0},{P::NoiseLevel,.8f},{P::Cutoff,1500},{P::Resonance,.64f},
             {P::Lfo1Wave,1},{P::Lfo1Rate,.24f},{P::Lfo1Cutoff,3.1f},{P::Attack,.65f},{P::Decay,1},{P::Sustain,.7f},{P::Release,2.1f},
             {P::ChorusEnabled,1},{P::ChorusDepth,.6f},{P::ChorusMix,.3f},{P::ReverbEnabled,1},{P::ReverbSize,.9f},
             {P::ReverbDamping,.45f},{P::ReverbMix,.4f},{P::Drive,0},{P::Output,-6}});
        add("Distant Signal","FX","Почти чистый тон с редким тремоло, вибрато и длинным ping-pong.","fx,signal,sine,echo","Дальний, мерцающий",55,91,
            {{P::WaveA,0},{P::WaveB,0},{P::OctaveB,1},{P::LevelA,.76f},{P::LevelB,.08f},{P::Cutoff,6800},{P::Attack,.3f},
             {P::Decay,1.2f},{P::Sustain,.54f},{P::Release,2.2f},{P::Lfo1Wave,0},{P::Lfo1Rate,.75f},{P::Lfo1Level,.65f},
             {P::Lfo2Rate,5.6f},{P::Lfo2Pitch,.1f},{P::DelayEnabled,1},{P::DelayTime,.83f},{P::DelayFeedback,.62f},{P::DelayMix,.36f},
             {P::ReverbEnabled,1},{P::ReverbSize,.88f},{P::ReverbDamping,.35f},{P::ReverbMix,.3f},{P::Drive,0},{P::Output,-4}});
        return result;
    }();
    return library;
}
}
