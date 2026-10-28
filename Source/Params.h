#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cmath>

namespace axioma
{
enum P
{
    O1Wave, O1Oct, O1Semi, O1Fine, O1Lvl,
    O2Wave, O2Oct, O2Semi, O2Fine, O2Lvl,
    O3Wave, O3Oct, O3Semi, O3Fine, O3Lvl,
    UniVoices, UniSpread,
    FType, FCut, FRes, FEnv,
    AA, AD, AS, AR,
    FA, FD, FS, FR,
    LfoShape, LfoRate, LfoDepth, LfoDest,
    ArpOn, ArpMode, ArpRate, ArpOct, ArpGate,
    Chorus, DelayMix, DelayTime, DelayFb, RevMix, RevSize,
    Master,
    NumParams
};

// kind: 0 = float, 1 = int, 2 = choice, 3 = bool
struct PDef { const char* id; const char* name; float mn, mx, def; int kind; const char* choices; float skew; };

inline const PDef* defs()
{
    static const PDef d[NumParams] = {
        { "o1_wave", "Wave", 0, 3, 2, 0, nullptr, 1.f }, { "o1_oct", "Oct", -2, 2, 0, 1, nullptr, 1.f },
        { "o1_semi", "Semi", -12, 12, 0, 1, nullptr, 1.f }, { "o1_fine", "Fine", -50, 50, 0, 0, nullptr, 1.f },
        { "o1_lvl", "Level", 0, 1, 0.8f, 0, nullptr, 1.f },
        { "o2_wave", "Wave", 0, 3, 2, 0, nullptr, 1.f }, { "o2_oct", "Oct", -2, 2, 0, 1, nullptr, 1.f },
        { "o2_semi", "Semi", -12, 12, 0, 1, nullptr, 1.f }, { "o2_fine", "Fine", -50, 50, 8, 0, nullptr, 1.f },
        { "o2_lvl", "Level", 0, 1, 0.5f, 0, nullptr, 1.f },
        { "o3_wave", "Wave", 0, 3, 0, 0, nullptr, 1.f }, { "o3_oct", "Oct", -2, 2, -1, 1, nullptr, 1.f },
        { "o3_semi", "Semi", -12, 12, 0, 1, nullptr, 1.f }, { "o3_fine", "Fine", -50, 50, 0, 0, nullptr, 1.f },
        { "o3_lvl", "Level", 0, 1, 0.0f, 0, nullptr, 1.f },
        { "uni_voices", "Voices", 1, 7, 3, 1, nullptr, 1.f }, { "uni_spread", "Spread", 0, 50, 12, 0, nullptr, 1.f },
        { "f_type", "Type", 0, 2, 0, 2, "Low Pass|Band Pass|High Pass", 1.f },
        { "f_cut", "Cutoff", 20, 20000, 8000, 0, nullptr, 0.3f }, { "f_res", "Reso", 0, 1, 0.2f, 0, nullptr, 1.f },
        { "f_env", "Env Amt", -1, 1, 0.3f, 0, nullptr, 1.f },
        { "a_a", "Attack", 0.001f, 8, 0.01f, 0, nullptr, 0.35f }, { "a_d", "Decay", 0.001f, 8, 0.3f, 0, nullptr, 0.35f },
        { "a_s", "Sustain", 0, 1, 0.7f, 0, nullptr, 1.f }, { "a_r", "Release", 0.001f, 8, 0.3f, 0, nullptr, 0.35f },
        { "e_a", "Attack", 0.001f, 8, 0.01f, 0, nullptr, 0.35f }, { "e_d", "Decay", 0.001f, 8, 0.4f, 0, nullptr, 0.35f },
        { "e_s", "Sustain", 0, 1, 0.4f, 0, nullptr, 1.f }, { "e_r", "Release", 0.001f, 8, 0.3f, 0, nullptr, 0.35f },
        { "l_shape", "Shape", 0, 4, 0, 2, "Sine|Triangle|Saw|Square|S&H", 1.f },
        { "l_rate", "Rate", 0.05f, 20, 2, 0, nullptr, 0.4f }, { "l_depth", "Depth", 0, 1, 0, 0, nullptr, 1.f },
        { "l_dest", "Dest", 0, 4, 1, 2, "Off|Cutoff|Pitch|Wave|Amp", 1.f },
        { "arp_on", "Arp", 0, 1, 0, 3, nullptr, 1.f },
        { "arp_mode", "Mode", 0, 4, 0, 2, "Up|Down|Up/Down|Random|Played", 1.f },
        { "arp_rate", "Rate", 0, 3, 2, 2, "1/4|1/8|1/16|1/32", 1.f },
        { "arp_oct", "Octaves", 1, 4, 2, 1, nullptr, 1.f }, { "arp_gate", "Gate", 0.1f, 1, 0.6f, 0, nullptr, 1.f },
        { "fx_chorus", "Chorus", 0, 1, 0, 0, nullptr, 1.f }, { "fx_dmix", "Dly Mix", 0, 1, 0, 0, nullptr, 1.f },
        { "fx_dtime", "Dly Time", 20, 1000, 350, 0, nullptr, 1.f }, { "fx_dfb", "Dly Fdbk", 0, 0.9f, 0.35f, 0, nullptr, 1.f },
        { "fx_rmix", "Rev Mix", 0, 1, 0.2f, 0, nullptr, 1.f }, { "fx_rsize", "Rev Size", 0, 1, 0.6f, 0, nullptr, 1.f },
        { "master", "Master", 0, 1, 0.7f, 0, nullptr, 1.f }
    };
    return d;
}

inline juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    for (int i = 0; i < NumParams; ++i)
    {
        const auto& d = defs()[i];
        ParameterID pid { d.id, 1 };
        switch (d.kind)
        {
            case 0: l.add (std::make_unique<AudioParameterFloat> (pid, d.name, NormalisableRange<float> (d.mn, d.mx, 0.f, d.skew), d.def)); break;
            case 1: l.add (std::make_unique<AudioParameterInt> (pid, d.name, (int) d.mn, (int) d.mx, (int) d.def)); break;
            case 2: l.add (std::make_unique<AudioParameterChoice> (pid, d.name, StringArray::fromTokens (d.choices, "|", ""), (int) d.def)); break;
            default: l.add (std::make_unique<AudioParameterBool> (pid, d.name, d.def > 0.5f)); break;
        }
    }
    return l;
}

// ───────────────────────── 20.000 PRESETS (generados de forma determinista) ─────────────────────────
namespace presets
{
constexpr int perCat = 2500, numCats = 8, total = perCat * numCats;

inline const char* catName (int c)
{
    static const char* n[] = { "Pads", "Keys", "Lead", "Arp", "Bass", "Pluck", "Bells", "FX" };
    return n[c];
}

inline juce::String name (int idx)
{
    static const char* adj[] = { "Neon", "Glass", "Ghost", "Solar", "Lunar", "Cyber", "Velvet", "Chrome", "Silent", "Binary",
                                 "Prism", "Quantum", "Hollow", "Static", "Ivory", "Onyx", "Pulse", "Vapor", "Mono", "Atomic",
                                 "Frozen", "Liquid", "Digital", "Astral", "Noir", "Lucid", "Orbital", "Magnetic", "Zero", "Aurora" };
    static const char* noun[] = { "Horizon", "Echo", "Signal", "Matrix", "Drift", "Cathedral", "Engine", "Circuit", "Dream", "Skyline",
                                  "Bloom", "Vector", "Temple", "Machine", "Orbit", "Mirror", "Cascade", "Voltage", "Shadow", "Tide",
                                  "Gate", "Archive", "Nova", "Pulse", "Field", "Tower", "Wave", "Relic", "Crown", "Axiom" };
    uint32_t h = (uint32_t) (idx + 1) * 2654435761u; h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
    return juce::String (adj[h % 30]) + " " + noun[(h >> 8) % 30] + " " + juce::String (idx % perCat + 1).paddedLeft ('0', 4);
}

inline std::array<float, NumParams> make (int idx)
{
    using namespace juce;
    std::array<float, NumParams> v {};
    for (int i = 0; i < NumParams; ++i) v[(size_t) i] = defs()[i].def;

    Random rnd ((int64) idx * 2654435761LL + 12345);
    auto R  = [&] (float a, float b) { return a + (b - a) * rnd.nextFloat(); };
    auto RL = [&] (float a, float b) { return a * std::pow (b / a, rnd.nextFloat()); };
    auto I  = [&] (int a, int b) { return (float) (rnd.nextInt (b - a + 1) + a); };
    auto pick = [&] (std::initializer_list<int> l) { return (float) *(l.begin() + rnd.nextInt ((int) l.size())); };
    auto set = [&] (P p, float x) { v[(size_t) p] = x; };
    const int cat = idx / perCat;

    set (O1Lvl, R (0.6f, 1.f)); set (O2Lvl, R (0.2f, 0.8f)); set (O3Lvl, rnd.nextFloat() < 0.4f ? R (0.1f, 0.5f) : 0.f);
    set (O1Fine, R (-8, 8)); set (O2Fine, R (-25, 25)); set (O3Fine, R (-25, 25));
    set (O2Semi, rnd.nextFloat() < 0.3f ? pick ({ 0, 7, 12, -12, 5 }) : 0.f);
    set (LfoShape, I (0, 4)); set (LfoRate, RL (0.1f, 8.f)); set (LfoDepth, R (0, 0.2f));

    switch (cat)
    {
        case 0: // PADS
            set (O1Wave, R (0, 1.6f)); set (O2Wave, R (0.8f, 2.4f)); set (O3Wave, R (0, 3));
            set (UniVoices, I (3, 7)); set (UniSpread, R (10, 40));
            set (FType, 0); set (FCut, RL (500, 6000)); set (FRes, R (0.05f, 0.35f)); set (FEnv, R (-0.2f, 0.5f));
            set (AA, RL (0.4f, 2.5f)); set (AD, RL (0.5f, 2)); set (AS, R (0.6f, 0.95f)); set (AR, RL (1, 5));
            set (FA, RL (0.5f, 3)); set (FD, RL (0.5f, 3)); set (FS, R (0.3f, 0.9f)); set (FR, RL (1, 4));
            set (LfoDepth, R (0.05f, 0.4f)); set (LfoDest, rnd.nextBool() ? 1.f : 3.f);
            set (Chorus, R (0.3f, 0.7f)); set (DelayMix, R (0, 0.3f)); set (RevMix, R (0.3f, 0.65f)); set (RevSize, R (0.7f, 0.95f));
            break;
        case 1: // KEYS
            set (O1Wave, R (0, 1.2f)); set (O2Wave, R (0, 1)); set (O2Semi, pick ({ 0, 12, 19 }));
            set (UniVoices, I (1, 2)); set (UniSpread, R (2, 10));
            set (FCut, RL (1500, 9000)); set (FEnv, R (0, 0.5f)); set (FD, RL (0.3f, 1)); set (FS, R (0.1f, 0.4f)); set (FR, 0.3f);
            set (AA, RL (0.002f, 0.01f)); set (AD, RL (0.5f, 1.8f)); set (AS, R (0.1f, 0.5f)); set (AR, RL (0.25f, 0.9f));
            set (LfoDepth, R (0, 0.15f)); set (Chorus, R (0.1f, 0.5f)); set (RevMix, R (0.1f, 0.35f));
            break;
        case 2: // LEAD
            set (O1Wave, R (1.5f, 3)); set (O2Wave, R (1.5f, 3)); set (UniVoices, I (2, 5)); set (UniSpread, R (6, 24));
            set (FCut, RL (2500, 10000)); set (FRes, R (0.15f, 0.5f)); set (FEnv, R (0.1f, 0.5f));
            set (AA, RL (0.002f, 0.03f)); set (AD, RL (0.1f, 0.5f)); set (AS, R (0.7f, 1)); set (AR, RL (0.1f, 0.5f));
            set (FD, RL (0.2f, 0.8f)); set (FS, R (0.4f, 0.9f));
            set (DelayMix, R (0.15f, 0.4f)); set (DelayTime, R (250, 500)); set (DelayFb, R (0.25f, 0.5f));
            set (RevMix, R (0.1f, 0.3f)); set (Chorus, R (0, 0.3f));
            set (LfoDest, 2); set (LfoDepth, R (0, 0.12f)); set (LfoRate, R (4, 7)); set (LfoShape, 0);
            break;
        case 3: // ARP
            set (ArpOn, 1); set (ArpMode, I (0, 4)); set (ArpRate, I (1, 3)); set (ArpOct, I (1, 4)); set (ArpGate, R (0.3f, 0.8f));
            set (O1Wave, R (1, 3)); set (O2Wave, R (0, 3)); set (UniVoices, I (1, 3)); set (UniSpread, R (4, 20));
            set (FCut, RL (1200, 8000)); set (FRes, R (0.1f, 0.45f)); set (FEnv, R (0.2f, 0.7f));
            set (AA, 0.002f); set (AD, RL (0.1f, 0.4f)); set (AS, R (0, 0.5f)); set (AR, RL (0.1f, 0.4f));
            set (FD, RL (0.1f, 0.4f)); set (FS, R (0, 0.3f));
            set (DelayMix, R (0.2f, 0.5f)); set (DelayTime, R (200, 450)); set (DelayFb, R (0.3f, 0.55f)); set (RevMix, R (0.15f, 0.4f));
            break;
        case 4: // BASS
            set (O1Oct, pick ({ -1, -2 })); set (O2Oct, -1); set (O3Lvl, 0); set (UniVoices, 1);
            set (O1Wave, R (1.5f, 3)); set (O2Wave, R (0, 2.2f));
            set (FCut, RL (150, 1200)); set (FRes, R (0.1f, 0.5f)); set (FEnv, R (0.2f, 0.8f));
            set (AA, 0.002f); set (AD, RL (0.15f, 0.6f)); set (AS, R (0.6f, 1)); set (AR, RL (0.08f, 0.3f));
            set (FD, RL (0.1f, 0.4f)); set (FS, R (0, 0.3f)); set (Chorus, R (0, 0.1f)); set (RevMix, R (0, 0.1f));
            break;
        case 5: // PLUCK
            set (O1Wave, R (1, 3)); set (O2Wave, R (1, 3)); set (UniVoices, I (1, 3)); set (UniSpread, R (3, 15));
            set (AA, 0.001f); set (AD, RL (0.1f, 0.45f)); set (AS, 0); set (AR, RL (0.15f, 0.5f));
            set (FCut, RL (300, 3000)); set (FEnv, R (0.4f, 1)); set (FD, RL (0.1f, 0.35f)); set (FS, 0);
            set (DelayMix, R (0.1f, 0.35f)); set (DelayTime, R (200, 450)); set (RevMix, R (0.15f, 0.4f));
            break;
        case 6: // BELLS
            set (O1Wave, R (0, 1)); set (O2Wave, R (0, 1)); set (O2Semi, pick ({ 12, 19, 24, 7 }));
            set (O3Semi, pick ({ 24, 28, 31 })); set (O3Lvl, R (0.15f, 0.5f)); set (UniVoices, I (1, 3));
            set (FCut, RL (4000, 16000)); set (FEnv, R (0, 0.3f));
            set (AA, 0.002f); set (AD, RL (1.5f, 5)); set (AS, R (0, 0.1f)); set (AR, RL (1.5f, 5));
            set (RevMix, R (0.3f, 0.6f)); set (RevSize, R (0.7f, 0.95f)); set (Chorus, R (0, 0.3f));
            break;
        default: // FX
            set (O1Wave, R (0, 3)); set (O2Wave, R (0, 3)); set (O3Wave, R (0, 3)); set (O3Lvl, R (0.2f, 0.7f));
            set (UniVoices, I (1, 7)); set (UniSpread, R (10, 50));
            set (FType, I (0, 2)); set (FCut, RL (200, 12000)); set (FRes, R (0.2f, 0.8f)); set (FEnv, R (-1, 1));
            set (AA, RL (0.01f, 3)); set (AD, RL (0.3f, 3)); set (AS, R (0, 1)); set (AR, RL (0.5f, 6));
            set (LfoDepth, R (0.3f, 1)); set (LfoRate, RL (0.1f, 15)); set (LfoDest, I (1, 4));
            set (Chorus, R (0, 1)); set (DelayMix, R (0, 0.6f)); set (DelayFb, R (0.3f, 0.8f)); set (RevMix, R (0.3f, 0.8f)); set (RevSize, R (0.7f, 1));
            break;
    }
    return v;
}
} // namespace presets
} // namespace axioma
