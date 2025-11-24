// Pixel Shader for HDR10 rendering with PQ encoding

struct PSInput
{
    float4 position : SV_POSITION;
    float3 colorNits : COLOR;
};

// PQ (Perceptual Quantizer) ST.2084 constants
static const float PQ_M1 = 0.1593017578125;     // 2610 / 16384
static const float PQ_M2 = 78.84375;            // 2523 / 32
static const float PQ_C1 = 0.8359375;           // 3424 / 4096
static const float PQ_C2 = 18.8515625;          // 2413 / 128
static const float PQ_C3 = 18.6875;             // 2392 / 128
static const float PQ_MAX_NITS = 10000.0;

// Convert linear nits value to PQ (ST.2084) encoded value
float NitsToPQ(float nits)
{
    // Normalize to [0, 1] range based on max nits
    float Y = nits / PQ_MAX_NITS;
    
    if (Y <= 0.0)
        return 0.0;

    float Ypow = pow(Y, PQ_M1);
    float numerator = PQ_C1 + PQ_C2 * Ypow;
    float denominator = 1.0 + PQ_C3 * Ypow;
    float result = pow(numerator / denominator, PQ_M2);

    return result;
}

float3 NitsToPQ3(float3 nits)
{
    return float3(
        NitsToPQ(nits.r),
        NitsToPQ(nits.g),
        NitsToPQ(nits.b)
    );
}

float4 main(PSInput input) : SV_TARGET
{
    // Convert nits to PQ-encoded values
    float3 pqColor = NitsToPQ3(input.colorNits);
    
    // Output in R10G10B10A2 format (HDR10)
    return float4(pqColor, 1.0);
}
