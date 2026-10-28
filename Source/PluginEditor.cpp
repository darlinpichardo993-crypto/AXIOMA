#include "PluginEditor.h"

using namespace axioma;

static void spaced (juce::Graphics& g, const juce::String& t, float x, float y, float size, bool bold,
                    float spacing, float alpha, bool centre = false)
{
    juce::Font f (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    g.setFont (f);
    g.setColour (juce::Colours::white.withAlpha (alpha));
    float total = 0.f;
    for (auto c : t) total += f.getStringWidthFloat (juce::String::charToString (c)) + spacing;
    float cx = centre ? x - (total - spacing) * 0.5f : x;
    for (auto c : t)
    {
        auto s = juce::String::charToString (c);
        g.drawSingleLineText (s, juce::roundToInt (cx), juce::roundToInt (y));
        cx += f.getStringWidthFloat (s) + spacing;
    }
}

static void drawPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    auto f = r.toFloat();
    g.setColour (juce::Colour (0xff080808)); g.fillRect (f);
    g.setColour (juce::Colours::white.withAlpha (0.14f)); g.drawRect (f, 1.f);
    g.setColour (juce::Colours::white.withAlpha (0.95f));
    const float L = 9.f;
    g.drawLine (f.getX(), f.getY(), f.getX() + L, f.getY(), 1.5f);          g.drawLine (f.getX(), f.getY(), f.getX(), f.getY() + L, 1.5f);
    g.drawLine (f.getRight(), f.getY(), f.getRight() - L, f.getY(), 1.5f);  g.drawLine (f.getRight(), f.getY(), f.getRight(), f.getY() + L, 1.5f);
    g.drawLine (f.getX(), f.getBottom(), f.getX() + L, f.getBottom(), 1.5f); g.drawLine (f.getX(), f.getBottom(), f.getX(), f.getBottom() - L, 1.5f);
    g.drawLine (f.getRight(), f.getBottom(), f.getRight() - L, f.getBottom(), 1.5f); g.drawLine (f.getRight(), f.getBottom(), f.getRight(), f.getBottom() - L, 1.5f);
    g.fillRect (f.getX() + 10.f, f.getY() + 10.f, 4.f, 4.f);
    spaced (g, title, f.getX() + 20.f, f.getY() + 17.f, 10.f, true, 3.f, 0.75f);
}

AxiomaEditor::AxiomaEditor (AxiomaProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&look);
    auto& ap = proc.apvts;

    for (int i = 0; i < NumParams; ++i)
    {
        const auto& d = defs()[i];
        if (d.kind <= 1)
        {
            knobs[(size_t) i] = std::make_unique<Knob> (ap, d.id, d.name);
            addAndMakeVisible (*knobs[(size_t) i]);
        }
        else if (d.kind == 2)
        {
            combos[(size_t) i] = std::make_unique<Combo> (ap, d.id, juce::StringArray::fromTokens (d.choices, "|", ""));
            addAndMakeVisible (*combos[(size_t) i]);
        }
    }
    addAndMakeVisible (arpBtn);
    arpAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (ap, defs()[ArpOn].id, arpBtn);

    addAndMakeVisible (prevBtn); addAndMakeVisible (nextBtn);
    prevBtn.onClick = [this] { step (-1); };
    nextBtn.onClick = [this] { step (+1); };

    catBox.addItem ("ALL CATEGORIES", 1);
    for (int c = 0; c < presets::numCats; ++c) catBox.addItem (juce::String (presets::catName (c)).toUpperCase(), c + 2);
    catBox.setSelectedId (1, juce::dontSendNotification);
    catBox.onChange = [this] { rebuild(); };
    addAndMakeVisible (catBox);

    search.setTextToShowWhenEmpty ("search...", juce::Colours::white.withAlpha (0.4f));
    search.onTextChange = [this] { rebuild(); };
    addAndMakeVisible (search);

    list.setModel (this);
    list.setRowHeight (22);
    addAndMakeVisible (list);

    setSize (1180, 700);
    rebuild();
    startTimerHz (30);
}

AxiomaEditor::~AxiomaEditor()
{
    stopTimer();
    list.setModel (nullptr);
    setLookAndFeel (nullptr);
}

void AxiomaEditor::rebuild()
{
    filtered.clear();
    const int cat = catBox.getSelectedId() - 2;
    const auto q = search.getText().trim().toLowerCase();
    const int from = cat >= 0 ? cat * presets::perCat : 0;
    const int to   = cat >= 0 ? from + presets::perCat : presets::total;
    for (int i = from; i < to; ++i)
    {
        if (q.isNotEmpty() && ! (juce::String (presets::catName (i / presets::perCat)) + " " + presets::name (i)).toLowerCase().contains (q))
            continue;
        filtered.push_back (i);
    }
    list.updateContent();
    syncList();
}

void AxiomaEditor::syncList()
{
    ignoreSel = true;
    auto it = std::find (filtered.begin(), filtered.end(), proc.currentPreset.load());
    if (it != filtered.end()) list.selectRow ((int) (it - filtered.begin())); else list.deselectAllRows();
    ignoreSel = false;
}

void AxiomaEditor::step (int d)
{
    if (filtered.empty()) return;
    auto it = std::find (filtered.begin(), filtered.end(), proc.currentPreset.load());
    const int sz = (int) filtered.size();
    int pos = it == filtered.end() ? 0 : (int) (it - filtered.begin()) + d;
    pos = ((pos % sz) + sz) % sz;
    proc.loadPreset (filtered[(size_t) pos]);
    syncList();
}

void AxiomaEditor::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= (int) filtered.size()) return;
    const int idx = filtered[(size_t) row];
    if (selected) g.fillAll (juce::Colours::white);
    g.setColour (selected ? juce::Colours::black : juce::Colours::white.withAlpha (0.85f));
    g.setFont (juce::Font (juce::FontOptions (12.f)));
    g.drawText (presets::name (idx), 8, 0, w - 50, h, juce::Justification::centredLeft);
    g.setColour (selected ? juce::Colours::black.withAlpha (0.6f) : juce::Colours::white.withAlpha (0.4f));
    g.setFont (juce::Font (juce::FontOptions (9.f)));
    g.drawText (juce::String (presets::catName (idx / presets::perCat)).toUpperCase(), w - 52, 0, 46, h, juce::Justification::centredRight);
}

void AxiomaEditor::selectedRowsChanged (int row)
{
    if (! ignoreSel && row >= 0 && row < (int) filtered.size())
        proc.loadPreset (filtered[(size_t) row]);
}

void AxiomaEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::white.withAlpha (0.025f));
    for (int x = 0; x < getWidth(); x += 40)  g.drawVerticalLine (x, 0.f, (float) getHeight());
    for (int y = 0; y < getHeight(); y += 40) g.drawHorizontalLine (y, 0.f, (float) getWidth());

    // cabecera
    spaced (g, "AXIOMA", 24.f, 52.f, 38.f, true, 10.f, 1.f);
    spaced (g, "MADE BY ORYMY", 28.f, 70.f, 10.f, false, 4.f, 0.6f);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawHorizontalLine (78, 16.f, (float) getWidth() - 16.f);

    const int cur = proc.currentPreset.load();
    juce::Rectangle<float> pr (330.f, 20.f, 480.f, 48.f);
    g.setColour (juce::Colours::white.withAlpha (0.5f)); g.drawRect (pr, 1.f);
    spaced (g, presets::name (cur).toUpperCase(), pr.getCentreX(), 43.f, 16.f, true, 2.f, 1.f, true);
    spaced (g, juce::String (presets::catName (cur / presets::perCat)).toUpperCase() + "   /   " + juce::String (cur + 1).paddedLeft ('0', 5) + " OF 20000",
            pr.getCentreX(), 59.f, 9.f, false, 3.f, 0.5f, true);
    spaced (g, "20 000 SOUNDS", 1060.f, 46.f, 9.f, false, 3.f, 0.45f, true);

    for (auto& p : panels) drawPanel (g, p.first, p.second);

    // formas de onda de los osciladores
    for (int o = 0; o < 3; ++o)
    {
        auto r = oscWave[o].toFloat();
        g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (r);
        g.setColour (juce::Colours::white.withAlpha (0.12f)); g.drawHorizontalLine ((int) r.getCentreY(), r.getX(), r.getRight());
        const float m = proc.apvts.getRawParameterValue (defs()[O1Wave + o * 5].id)->load();
        juce::Path path;
        for (int k = 0; k <= 120; ++k)
        {
            const float ph = (float) k / 120.f * 0.999f;
            const float x = r.getX() + r.getWidth() * (float) k / 120.f;
            const float y = r.getCentreY() - morphWave (ph, 0.f, m) * r.getHeight() * 0.4f;
            if (k == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
        }
        g.setColour (juce::Colours::white.withAlpha (0.2f)); g.strokePath (path, juce::PathStrokeType (4.f));
        g.setColour (juce::Colours::white); g.strokePath (path, juce::PathStrokeType (1.4f));
    }

    // osciloscopio de salida
    auto area = scopeRect.reduced (12, 0).withTrimmedTop (26).withTrimmedBottom (8).toFloat();
    g.setColour (juce::Colours::white.withAlpha (0.1f)); g.drawHorizontalLine ((int) area.getCentreY(), area.getX(), area.getRight());
    float tmp[1024];
    const int pos = proc.scopePos.load();
    for (int k = 0; k < 1024; ++k) tmp[k] = proc.scopeBuf[(pos + k) & 1023];
    int start = 0;
    for (int k = 0; k < 400; ++k) if (tmp[k] <= 0.f && tmp[k + 1] > 0.f) { start = k; break; }
    juce::Path sp;
    for (int i = 0; i < 512; ++i)
    {
        const float x = area.getX() + area.getWidth() * (float) i / 511.f;
        const float y = area.getCentreY() - juce::jlimit (-1.f, 1.f, tmp[start + i]) * area.getHeight() * 0.48f;
        if (i == 0) sp.startNewSubPath (x, y); else sp.lineTo (x, y);
    }
    g.setColour (juce::Colours::white.withAlpha (0.18f)); g.strokePath (sp, juce::PathStrokeType (4.f));
    g.setColour (juce::Colours::white.withAlpha (0.95f)); g.strokePath (sp, juce::PathStrokeType (1.3f));
}

void AxiomaEditor::resized()
{
    panels.clear();
    auto place = [&] (juce::Rectangle<int> a, std::initializer_list<int> ids)
    {
        const int cnt = (int) ids.size(); if (cnt == 0) return;
        const int w = a.getWidth() / cnt; int x = a.getX();
        for (int id : ids) { knobs[(size_t) id]->setBounds (x, a.getY(), w, a.getHeight()); x += w; }
    };

    prevBtn.setBounds (286, 24, 36, 40);
    nextBtn.setBounds (818, 24, 36, 40);
    knobs[Master]->setBounds (1090, 6, 74, 68);

    juce::Rectangle<int> br (16, 84, 250, 600);
    panels.push_back ({ br, "PRESET BROWSER" });
    catBox.setBounds (br.getX() + 10, br.getY() + 30, br.getWidth() - 20, 24);
    search.setBounds (br.getX() + 10, br.getY() + 62, br.getWidth() - 20, 24);
    list.setBounds (br.getX() + 10, br.getY() + 94, br.getWidth() - 20, br.getHeight() - 104);

    const int X0 = 282, Y0 = 84;

    for (int o = 0; o < 3; ++o)
    {
        juce::Rectangle<int> r (X0 + o * 298, Y0, 286, 190);
        panels.push_back ({ r, "OSCILLATOR " + juce::String (o + 1) });
        oscWave[o] = { r.getX() + 14, r.getY() + 28, r.getWidth() - 28, 44 };
        const int b = O1Wave + o * 5;
        place ({ r.getX() + 8, r.getY() + 80, r.getWidth() - 16, 102 }, { b, b + 1, b + 2, b + 3, b + 4 });
    }

    const int y2 = Y0 + 202;
    {   juce::Rectangle<int> r (X0, y2, 150, 150);
        panels.push_back ({ r, "UNISON" });
        place ({ r.getX() + 8, r.getY() + 40, r.getWidth() - 16, 100 }, { UniVoices, UniSpread }); }
    {   juce::Rectangle<int> r (X0 + 164, y2, 262, 150);
        panels.push_back ({ r, "FILTER" });
        combos[FType]->setBounds (r.getX() + 12, r.getY() + 28, r.getWidth() - 24, 22);
        place ({ r.getX() + 8, r.getY() + 56, r.getWidth() - 16, 88 }, { FCut, FRes, FEnv }); }
    {   juce::Rectangle<int> r (X0 + 440, y2, 214, 150);
        panels.push_back ({ r, "AMP ENVELOPE" });
        place ({ r.getX() + 6, r.getY() + 44, r.getWidth() - 12, 100 }, { AA, AD, AS, AR }); }
    {   juce::Rectangle<int> r (X0 + 668, y2, 214, 150);
        panels.push_back ({ r, "FILTER ENVELOPE" });
        place ({ r.getX() + 6, r.getY() + 44, r.getWidth() - 12, 100 }, { FA, FD, FS, FR }); }

    const int y3 = y2 + 162;
    {   juce::Rectangle<int> r (X0, y3, 220, 140);
        panels.push_back ({ r, "LFO" });
        combos[LfoShape]->setBounds (r.getX() + 10, r.getY() + 28, 96, 22);
        combos[LfoDest]->setBounds (r.getX() + 112, r.getY() + 28, 98, 22);
        place ({ r.getX() + 8, r.getY() + 56, r.getWidth() - 16, 78 }, { LfoRate, LfoDepth }); }
    {   juce::Rectangle<int> r (X0 + 232, y3, 290, 140);
        panels.push_back ({ r, "ARPEGGIATOR" });
        arpBtn.setBounds (r.getX() + 10, r.getY() + 28, 56, 22);
        combos[ArpMode]->setBounds (r.getX() + 72, r.getY() + 28, 112, 22);
        combos[ArpRate]->setBounds (r.getX() + 190, r.getY() + 28, 90, 22);
        place ({ r.getX() + 8, r.getY() + 56, 170, 78 }, { ArpOct, ArpGate }); }
    {   juce::Rectangle<int> r (X0 + 534, y3, 348, 140);
        panels.push_back ({ r, "EFFECTS" });
        place ({ r.getX() + 6, r.getY() + 44, r.getWidth() - 12, 90 }, { Chorus, DelayMix, DelayTime, DelayFb, RevMix, RevSize }); }

    scopeRect = { X0, y3 + 152, 882, 84 };
    panels.push_back ({ scopeRect, "OUTPUT" });
}
