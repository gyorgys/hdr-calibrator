#pragma once

#include <cmath>

// HDR10 utilities for converting between linear nits and PQ (ST.2084) values

namespace HDR
{
    // PQ (Perceptual Quantizer) ST.2084 constants
    constexpr float PQ_M1 = 0.1593017578125f;      // 2610 / 16384
    constexpr float PQ_M2 = 78.84375f;             // 2523 / 32
    constexpr float PQ_C1 = 0.8359375f;            // 3424 / 4096
    constexpr float PQ_C2 = 18.8515625f;           // 2413 / 128
    constexpr float PQ_C3 = 18.6875f;              // 2392 / 128
    constexpr float PQ_MAX_NITS = 10000.0f;

    // Convert linear nits value to PQ (ST.2084) encoded value
    inline float NitsToPQ(float nits)
    {
        // Normalize to [0, 1] range based on max nits
        float Y = nits / PQ_MAX_NITS;
        
        if (Y <= 0.0f)
            return 0.0f;

        float Ypow = powf(Y, PQ_M1);
        float numerator = PQ_C1 + PQ_C2 * Ypow;
        float denominator = 1.0f + PQ_C3 * Ypow;
        float result = powf(numerator / denominator, PQ_M2);

        return result;
    }

    // Convert PQ (ST.2084) encoded value to linear nits
    inline float PQToNits(float pq)
    {
        if (pq <= 0.0f)
            return 0.0f;

        float pqPow = powf(pq, 1.0f / PQ_M2);
        float numerator = fmaxf(pqPow - PQ_C1, 0.0f);
        float denominator = PQ_C2 - PQ_C3 * pqPow;
        
        if (denominator <= 0.0f)
            return 0.0f;

        float Y = powf(numerator / denominator, 1.0f / PQ_M1);
        
        return Y * PQ_MAX_NITS;
    }

    // Helper struct to represent RGB values in nits
    struct RGBNits
    {
        float r, g, b;

        RGBNits() : r(0.0f), g(0.0f), b(0.0f) {}
        RGBNits(float red, float green, float blue) : r(red), g(green), b(blue) {}

        // Convert to PQ-encoded values (0-1 range)
        void ToPQ(float& outR, float& outG, float& outB) const
        {
            outR = NitsToPQ(r);
            outG = NitsToPQ(g);
            outB = NitsToPQ(b);
        }

        // Create from a grayscale nits value
        static RGBNits FromGray(float nits)
        {
            return RGBNits(nits, nits, nits);
        }

        // Common preset values
        static RGBNits White100Nits() { return RGBNits(100.0f, 100.0f, 100.0f); }
        static RGBNits White203Nits() { return RGBNits(203.0f, 203.0f, 203.0f); } // SDR reference white
        static RGBNits White1000Nits() { return RGBNits(1000.0f, 1000.0f, 1000.0f); }
        static RGBNits Black() { return RGBNits(0.0f, 0.0f, 0.0f); }
    };

    // Helper to convert SDR (0-1) to HDR nits, assuming SDR white = 80 nits (common default)
    inline float SDRToNits(float sdr, float sdrWhiteNits = 80.0f)
    {
        return sdr * sdrWhiteNits;
    }

    // Helper to convert HDR nits to SDR (0-1)
    inline float NitsToSDR(float nits, float sdrWhiteNits = 80.0f)
    {
        return nits / sdrWhiteNits;
    }
}
