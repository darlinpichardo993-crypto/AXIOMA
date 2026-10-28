#pragma once
#include <cmath>

namespace axioma
{
inline float polyblep (float t, float dt)
{
    if (t < dt)         { t /= dt;               return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt)  { t = (t - 1.0f) / dt;   return t * t + t + t + 1.0f; }
    return 0.0f;
}

// 0 = seno, 1 = triángulo, 2 = sierra, 3 = cuadrada
inline float shape (int k, float p, float inc)
{
    switch (k)
    {
        case 0:  return std::sin (6.28318530718f * p);
        case 1:  return 4.0f * std::fabs (p - 0.5f) - 1.0f;
        case 2:  return 2.0f * p - 1.0f - polyblep (p, inc);
        default:
        {
            float pp = p + 0.5f; if (pp >= 1.0f) pp -= 1.0f;
            float v = (p < 0.5f ? 1.0f : -1.0f) + polyblep (p, inc) - polyblep (pp, inc);
            return v * 0.8f;
        }
    }
}

// Oscilador "morph": m = 0..3 mezcla continuamente seno -> triángulo -> sierra -> cuadrada
inline float morphWave (float p, float inc, float m)
{
    if (m < 0.0f) m = 0.0f;
    if (m > 3.0f) m = 3.0f;
    int i = (int) m; if (i > 2) i = 2;
    const float f = m - (float) i;
    const float a = shape (i, p, inc);
    if (f <= 0.0001f) return a;
    return a + (shape (i + 1, p, inc) - a) * f;
}
}
