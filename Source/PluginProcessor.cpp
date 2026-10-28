#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace axioma;

AxiomaProcessor::AxiomaProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AXIOMA", makeLayout())
{
    for (int i = 0; i < NumParams; ++i)
        raw[i] = apvts.getRawParameterValue (defs()[i].id);
    loadPreset (0);
}

void AxiomaProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    for (auto& v : voices) { v.prepare (sr); v.active = false; }
    dlyL.assign ((size_t) (sr * 1.2) + 4, 0.f);
    dlyR.assign (dlyL.size(), 0.f);
    dlyW = 0;
    chorus.prepare ({ sr, (juce::uint32) juce::jmax (1, samplesPerBlock), 2 });
    chorus.setRate (0.7f); chorus.setDepth (0.35f); chorus.setCentreDelay (8.f); chorus.setFeedback (0.f);
    reverb.setSampleRate (sr);
    held.clear(); arpPlaying = -1; arpPos = 0.0;
}

void AxiomaProcessor::voiceOn (int note, float vel)
{
    Voice* v = nullptr;
    for (auto& x : voices) if (! x.active) { v = &x; break; }
    if (v == nullptr) { v = &voices[0]; for (auto& x : voices) if (x.age < v->age) v = &x; }
    v->start (note, vel, ++ageCounter, ampP, fltP);
}

void AxiomaProcessor::voiceOff (int note)
{
    for (auto& v : voices) if (v.active && ! v.releasing && v.note == note) v.release();
}

void AxiomaProcessor::handle (const juce::MidiMessage& m, bool arpOn)
{
    if (m.isNoteOn())
    {
        if (arpOn)
        {
            if (held.empty()) { arpIdx = 0; arpPos = 1.0e9; } // dispara el primer paso al instante
            held.push_back (m.getNoteNumber());
        }
        else voiceOn (m.getNoteNumber(), m.getFloatVelocity());
    }
    else if (m.isNoteOff())
    {
        if (arpOn) held.erase (std::remove (held.begin(), held.end(), m.getNoteNumber()), held.end());
        else voiceOff (m.getNoteNumber());
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        held.clear();
        for (auto& v : voices) if (v.active) v.release();
    }
    else if (m.isPitchWheel())  bend = (float) (m.getPitchWheelValue() - 8192) / 8192.f * 2.f;
    else if (m.isController() && m.getControllerNumber() == 1) modWheel = (float) m.getControllerValue() / 127.f;
}

int AxiomaProcessor::nextArpNote (int mode, int octs)
{
    std::vector<int> base = held;
    if (mode != 4) std::sort (base.begin(), base.end());
    std::vector<int> seq;
    for (int o = 0; o < octs; ++o) for (int n : base) seq.push_back (n + 12 * o);
    if (seq.empty()) return -1;
    const int sz = (int) seq.size();
    if (arpIdx > 1000000) arpIdx = 0;
    switch (mode)
    {
        case 1:  { int n = seq[(size_t) (sz - 1 - (arpIdx % sz))]; ++arpIdx; return n; }
        case 2:  { if (sz == 1) return seq[0]; int cyc = 2 * sz - 2, k = arpIdx % cyc; ++arpIdx; return seq[(size_t) (k < sz ? k : cyc - k)]; }
        case 3:  return seq[(size_t) rng.nextInt (sz)];
        default: { int n = seq[(size_t) (arpIdx % sz)]; ++arpIdx; return n; }
    }
}

void AxiomaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (buffer.getNumChannels() < 2) return;
    float* outL = buffer.getWritePointer (0);
    float* outR = buffer.getWritePointer (1);
    auto g = [this] (int p) { return raw[p]->load(); };

    // ── snapshot de parámetros ──
    for (int o = 0; o < 3; ++o)
    {
        const int b = O1Wave + o * 5;
        snap.wave[o] = g (b); snap.oct[o] = g (b + 1); snap.semi[o] = g (b + 2); snap.fine[o] = g (b + 3); snap.lvl[o] = g (b + 4);
    }
    const int nU = juce::jlimit (1, 7, (int) g (UniVoices));
    const float spread = g (UniSpread);
    snap.nU = nU; snap.uniGain = 1.f / std::sqrt ((float) nU);
    for (int u = 0; u < nU; ++u)
    {
        const float t = nU > 1 ? (float) u / (float) (nU - 1) : 0.5f;
        snap.ratio[u] = std::pow (2.f, (nU > 1 ? (t * 2.f - 1.f) * spread : 0.f) / 1200.f);
        snap.gl[u] = std::cos (t * juce::MathConstants<float>::halfPi);
        snap.gr[u] = std::sin (t * juce::MathConstants<float>::halfPi);
    }
    snap.cut = g (FCut); snap.q = 0.707f + g (FRes) * 9.f; snap.fenv = g (FEnv);
    snap.lfoDest = (int) g (LfoDest);
    snap.lfoDepth = snap.lfoDest == 0 ? 0.f : juce::jmax (g (LfoDepth), modWheel);
    snap.bend = bend; snap.invSr = 1.f / (float) sr; snap.maxCut = (float) sr * 0.45f;

    ampP = { g (AA), g (AD), g (AS), g (AR) };
    fltP = { g (FA), g (FD), g (FS), g (FR) };
    const int ft = (int) g (FType);
    const auto ftype = ft == 0 ? juce::dsp::StateVariableTPTFilterType::lowpass
                     : ft == 1 ? juce::dsp::StateVariableTPTFilterType::bandpass
                               : juce::dsp::StateVariableTPTFilterType::highpass;
    for (auto& v : voices) { v.filter.setType (ftype); if (v.active) { v.amp.setParameters (ampP); v.flt.setParameters (fltP); } }

    const bool arpOn = g (ArpOn) > 0.5f;
    if (prevArpOn && ! arpOn)
    {
        if (arpPlaying >= 0) voiceOff (arpPlaying);
        arpPlaying = -1; held.clear();
    }
    prevArpOn = arpOn;

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = *b;
    static const double beats[] = { 1.0, 0.5, 0.25, 0.125 };
    const double stepLen = 60.0 / juce::jmax (20.0, bpm) * beats[juce::jlimit (0, 3, (int) g (ArpRate))] * sr;
    const double gateLen = stepLen * g (ArpGate);
    const int arpMode = (int) g (ArpMode), arpOcts = (int) g (ArpOct);

    const int lShape = (int) g (LfoShape);
    const float lRate = g (LfoRate);

    auto it = midi.begin(); const auto end = midi.end();

    for (int i = 0; i < n; ++i)
    {
        while (it != end && (*it).samplePosition <= i) { handle ((*it).getMessage(), arpOn); ++it; }

        if (arpOn)
        {
            if (arpPos >= stepLen)
            {
                arpPos = juce::jmin (arpPos - stepLen, stepLen);
                if (arpPlaying >= 0) { voiceOff (arpPlaying); arpPlaying = -1; }
                if (! held.empty())
                {
                    const int note = nextArpNote (arpMode, arpOcts);
                    if (note >= 0) { voiceOn (note, 0.8f); arpPlaying = note; }
                }
            }
            else if (arpPlaying >= 0 && arpPos >= gateLen) { voiceOff (arpPlaying); arpPlaying = -1; }
            arpPos += 1.0;
        }

        float lfo = 0.f;
        switch (lShape)
        {
            case 0: lfo = std::sin (juce::MathConstants<float>::twoPi * lfoPhase); break;
            case 1: lfo = 4.f * std::fabs (lfoPhase - 0.5f) - 1.f; break;
            case 2: lfo = 2.f * lfoPhase - 1.f; break;
            case 3: lfo = lfoPhase < 0.5f ? 1.f : -1.f; break;
            default: lfo = shValue; break;
        }
        lfoPhase += lRate / (float) sr;
        if (lfoPhase >= 1.f) { lfoPhase -= 1.f; shValue = rng.nextFloat() * 2.f - 1.f; }

        float l = 0.f, r = 0.f;
        for (auto& v : voices)
            if (v.active) { float a, b; v.render (snap, lfo, a, b); l += a; r += b; }
        outL[i] = l; outR[i] = r;
    }

    // ── efectos ──
    {
        juce::dsp::AudioBlock<float> blk (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (blk);
        chorus.setMix (g (Chorus));
        chorus.process (ctx);
    }

    const int dsz = (int) dlyL.size();
    const int ds = juce::jlimit (1, dsz - 1, (int) (g (DelayTime) * 0.001 * sr));
    const float dm = g (DelayMix), fb = g (DelayFb), master = g (Master);
    for (int i = 0; i < n; ++i)
    {
        int rp = dlyW - ds; if (rp < 0) rp += dsz;
        const float dL = dlyL[(size_t) rp], dR = dlyR[(size_t) rp];
        const float l = outL[i], r = outR[i];
        dlyL[(size_t) dlyW] = l + dR * fb;   // ping-pong
        dlyR[(size_t) dlyW] = r + dL * fb;
        outL[i] = l + dL * dm; outR[i] = r + dR * dm;
        if (++dlyW >= dsz) dlyW = 0;
    }

    juce::Reverb::Parameters rp;
    rp.roomSize = g (RevSize); rp.damping = 0.45f; rp.wetLevel = g (RevMix) * 0.45f; rp.dryLevel = 1.f; rp.width = 1.f; rp.freezeMode = 0.f;
    reverb.setParameters (rp);
    reverb.processStereo (outL, outR, n);

    for (int i = 0; i < n; ++i)
    {
        outL[i] = std::tanh (outL[i] * master * 1.6f);
        outR[i] = std::tanh (outR[i] * master * 1.6f);
        if ((++scopeDec & 1) == 0)
        {
            const int p = scopePos.load();
            scopeBuf[p] = (outL[i] + outR[i]) * 0.5f;
            scopePos.store ((p + 1) & 1023);
        }
    }
}

void AxiomaProcessor::loadPreset (int idx)
{
    idx = juce::jlimit (0, presets::total - 1, idx);
    currentPreset.store (idx);
    const auto v = presets::make (idx);
    for (int i = 0; i < NumParams; ++i)
        if (auto* p = apvts.getParameter (defs()[i].id))
            p->setValueNotifyingHost (p->convertTo0to1 (v[(size_t) i]));
}

void AxiomaProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void AxiomaProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentPreset.store (juce::jlimit (0, presets::total - 1, (int) vt.getProperty ("preset", 0)));
            apvts.replaceState (vt);
        }
}

juce::AudioProcessorEditor* AxiomaProcessor::createEditor() { return new AxiomaEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AxiomaProcessor(); }
