#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <vector>
#include "Params.h"
#include "Wave.h"

struct Snap
{
    float wave[3] {}, lvl[3] {}, oct[3] {}, semi[3] {}, fine[3] {};
    int nU = 1;
    float ratio[7] {}, gl[7] {}, gr[7] {};
    float uniGain = 1.f, cut = 1000.f, q = 1.f, fenv = 0.f, lfoDepth = 0.f, bend = 0.f;
    int lfoDest = 0;
    float invSr = 1.f / 44100.f, maxCut = 20000.f;
};

struct Voice
{
    bool active = false, releasing = false;
    int note = 0, ctr = 0;
    float vel = 0.f;
    uint32_t age = 0;
    float phase[3][7] {};
    juce::ADSR amp, flt;
    juce::dsp::StateVariableTPTFilter<float> filter;

    void prepare (double sr)
    {
        amp.setSampleRate (sr);
        flt.setSampleRate (sr);
        filter.prepare ({ sr, 512, 2 });
    }

    void start (int n, float v, uint32_t a, const juce::ADSR::Parameters& ap, const juce::ADSR::Parameters& fp)
    {
        active = true; releasing = false; note = n; vel = v; age = a; ctr = 0;
        for (int o = 0; o < 3; ++o)
            for (int u = 0; u < 7; ++u)
                phase[o][u] = std::fmod (0.61803f * (float) (u + 1 + o * 7) + 0.137f * (float) n, 1.f);
        filter.reset();
        amp.setParameters (ap); flt.setParameters (fp);
        amp.noteOn(); flt.noteOn();
    }

    void release() { releasing = true; amp.noteOff(); flt.noteOff(); }

    void render (const Snap& s, float lfo, float& oL, float& oR)
    {
        float l = 0.f, r = 0.f;
        for (int o = 0; o < 3; ++o)
        {
            if (s.lvl[o] < 0.001f) continue;
            float m = s.wave[o];
            if (s.lfoDest == 3) m = juce::jlimit (0.f, 3.f, m + lfo * s.lfoDepth * 1.5f);
            float semis = (float) note + s.oct[o] * 12.f + s.semi[o] + s.fine[o] / 100.f + s.bend;
            if (s.lfoDest == 2) semis += lfo * s.lfoDepth * 2.f;
            const float f = 440.f * std::pow (2.f, (semis - 69.f) / 12.f);
            for (int u = 0; u < s.nU; ++u)
            {
                const float inc = juce::jmin (0.45f, f * s.ratio[u] * s.invSr);
                float& p = phase[o][u];
                const float x = axioma::morphWave (p, inc, m) * s.lvl[o];
                l += x * s.gl[u]; r += x * s.gr[u];
                p += inc; if (p >= 1.f) p -= 1.f;
            }
        }
        l *= s.uniGain; r *= s.uniGain;

        const float a  = amp.getNextSample();
        const float fe = flt.getNextSample();
        if (! amp.isActive()) { active = false; oL = oR = 0.f; return; }

        if ((ctr++ & 7) == 0)
        {
            float oct = s.fenv * fe * 5.f;
            if (s.lfoDest == 1) oct += lfo * s.lfoDepth * 3.f;
            const float c = juce::jlimit (20.f, s.maxCut, s.cut * std::pow (2.f, oct));
            filter.setCutoffFrequency (c);
            filter.setResonance (s.q);
        }
        l = filter.processSample (0, l);
        r = filter.processSample (1, r);

        float g = a * vel * 0.35f;
        if (s.lfoDest == 4) g *= 1.f - s.lfoDepth * 0.5f * (lfo + 1.f);
        oL = l * g; oR = r * g;
    }
};

class AxiomaProcessor : public juce::AudioProcessor
{
public:
    AxiomaProcessor();
    ~AxiomaProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "AXIOMA"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void loadPreset (int idx);

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> currentPreset { 0 };
    float scopeBuf[1024] {};
    std::atomic<int> scopePos { 0 };

private:
    void voiceOn (int note, float vel);
    void voiceOff (int note);
    void handle (const juce::MidiMessage& m, bool arpOn);
    int nextArpNote (int mode, int octs);

    std::array<Voice, 16> voices;
    std::atomic<float>* raw[axioma::NumParams] {};
    Snap snap;
    juce::ADSR::Parameters ampP, fltP;
    uint32_t ageCounter = 0;
    double sr = 44100.0;

    float lfoPhase = 0.f, shValue = 0.f, bend = 0.f, modWheel = 0.f;
    juce::Random rng;

    std::vector<int> held;
    int arpIdx = 0, arpPlaying = -1;
    double arpPos = 0.0;
    bool prevArpOn = false;

    std::vector<float> dlyL, dlyR;
    int dlyW = 0, scopeDec = 0;
    juce::dsp::Chorus<float> chorus;
    juce::Reverb reverb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AxiomaProcessor)
};
