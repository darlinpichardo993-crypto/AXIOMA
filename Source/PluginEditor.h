#pragma once
#include "PluginProcessor.h"

// ───────────── Look & Feel negro / blanco futurista ─────────────
struct AxLook : public juce::LookAndFeel_V4
{
    AxLook()
    {
        using namespace juce;
        const Colour w = Colours::white, k = Colours::black;
        setColour (ResizableWindow::backgroundColourId, k);
        setColour (ComboBox::backgroundColourId, k);
        setColour (ComboBox::textColourId, w);
        setColour (ComboBox::outlineColourId, w.withAlpha (0.35f));
        setColour (ComboBox::arrowColourId, w);
        setColour (PopupMenu::backgroundColourId, Colour (0xff0a0a0a));
        setColour (PopupMenu::textColourId, w);
        setColour (PopupMenu::highlightedBackgroundColourId, w);
        setColour (PopupMenu::highlightedTextColourId, k);
        setColour (ListBox::backgroundColourId, Colour (0xff050505));
        setColour (ListBox::outlineColourId, w.withAlpha (0.12f));
        setColour (TextEditor::backgroundColourId, k);
        setColour (TextEditor::textColourId, w);
        setColour (TextEditor::outlineColourId, w.withAlpha (0.3f));
        setColour (TextEditor::focusedOutlineColourId, w);
        setColour (TextEditor::highlightColourId, w.withAlpha (0.3f));
        setColour (CaretComponent::caretColourId, w);
        setColour (TextButton::buttonColourId, k);
        setColour (TextButton::textColourOffId, w);
        setColour (TextButton::textColourOnId, w);
        setColour (ScrollBar::thumbColourId, w.withAlpha (0.35f));
        setColour (BubbleComponent::backgroundColourId, k);
        setColour (BubbleComponent::outlineColourId, w);
        setColour (TooltipWindow::textColourId, w);
        setColour (Label::textColourId, w);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override
    {
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.f);
        const float rad = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const float ang = a0 + pos * (a1 - a0);

        juce::Path track; track.addCentredArc (c.x, c.y, rad, rad, 0.f, a0, a1, true);
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.strokePath (track, juce::PathStrokeType (2.f));

        juce::Path val; val.addCentredArc (c.x, c.y, rad, rad, 0.f, a0, ang, true);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.strokePath (val, juce::PathStrokeType (6.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colours::white);
        g.strokePath (val, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float ir = rad * 0.66f;
        g.setColour (juce::Colour (0xff101010));
        g.fillEllipse (c.x - ir, c.y - ir, ir * 2.f, ir * 2.f);
        g.setColour (juce::Colours::white.withAlpha (0.28f));
        g.drawEllipse (c.x - ir, c.y - ir, ir * 2.f, ir * 2.f, 1.f);

        juce::Path p; p.addRectangle (-1.f, -ir * 0.95f, 2.f, ir * 0.55f);
        g.setColour (juce::Colours::white);
        g.fillPath (p, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (0.5f);
        g.setColour (juce::Colours::black); g.fillRect (r);
        g.setColour (juce::Colours::white.withAlpha (box.isMouseOver() ? 0.8f : 0.4f)); g.drawRect (r, 1.f);
        juce::Path a; const float cx = (float) w - 12.f, cy = (float) h * 0.5f;
        a.addTriangle (cx - 4.f, cy - 2.f, cx + 4.f, cy - 2.f, cx, cy + 3.f);
        g.setColour (juce::Colours::white); g.fillPath (a);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (12.f)); }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (down ? juce::Colours::white.withAlpha (0.25f) : juce::Colours::black); g.fillRect (r);
        g.setColour (juce::Colours::white.withAlpha (over ? 0.9f : 0.45f)); g.drawRect (r, 1.f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();
        g.setColour (on ? juce::Colours::white : juce::Colours::black); g.fillRect (r);
        g.setColour (juce::Colours::white.withAlpha (over || on ? 1.f : 0.5f)); g.drawRect (r, 1.f);
        g.setColour (on ? juce::Colours::black : juce::Colours::white);
        g.setFont (juce::Font (juce::FontOptions (11.f, juce::Font::bold)));
        g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
    }
};

struct Knob : public juce::Component
{
    juce::Slider s { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Label l;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;

    Knob (juce::AudioProcessorValueTreeState& a, const char* id, const juce::String& name)
    {
        addAndMakeVisible (s); addAndMakeVisible (l);
        l.setText (name.toUpperCase(), juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::Font (juce::FontOptions (10.f)));
        l.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.7f));
        l.setInterceptsMouseClicks (false, false);
        s.setPopupDisplayEnabled (true, true, nullptr);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, id, s);
    }
    void resized() override { auto b = getLocalBounds(); l.setBounds (b.removeFromBottom (16)); s.setBounds (b); }
};

struct Combo : public juce::ComboBox
{
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
    Combo (juce::AudioProcessorValueTreeState& a, const char* id, const juce::StringArray& items)
    {
        addItemList (items, 1);
        att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (a, id, *this);
    }
};

class AxiomaEditor : public juce::AudioProcessorEditor, private juce::Timer, private juce::ListBoxModel
{
public:
    explicit AxiomaEditor (AxiomaProcessor&);
    ~AxiomaEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override { repaint(); }
    int getNumRows() override { return (int) filtered.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void selectedRowsChanged (int row) override;

    void rebuild();
    void syncList();
    void step (int d);

    AxLook look;
    AxiomaProcessor& proc;

    std::array<std::unique_ptr<Knob>, axioma::NumParams> knobs;
    std::array<std::unique_ptr<Combo>, axioma::NumParams> combos;
    juce::ToggleButton arpBtn { "ARP" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> arpAtt;

    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    juce::ComboBox catBox;
    juce::TextEditor search;
    juce::ListBox list { "presets", nullptr };
    std::vector<int> filtered;
    bool ignoreSel = false;

    std::vector<std::pair<juce::Rectangle<int>, juce::String>> panels;
    juce::Rectangle<int> oscWave[3], scopeRect;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AxiomaEditor)
};
