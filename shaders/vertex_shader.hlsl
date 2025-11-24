// Vertex Shader for HDR rendering
struct VSInput
{
    float3 position : POSITION;
    float3 color : COLOR; // Color in nits
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 colorNits : COLOR;
};

PSInput main(VSInput input)
{
    PSInput output;
    output.position = float4(input.position, 1.0f);
    output.colorNits = input.color;
    return output;
}
