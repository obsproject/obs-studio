/******************************************************************************
 * OBS Studio — GPU Color Space Conversion Compute Shader (Phase 6.3)
 *
 * Offloads YUV → RGBA conversion from CPU to GPU using Direct3D 11 Compute
 * Shader 5.0.  Processes 8×8 tiles in parallel; each thread converts one
 * pixel from planar YUV (I420/NV12) to packed RGBA.
 *
 * Expected CPU reduction: 15-20 % when using GPU for color-space conversion
 * instead of libobs software fallback (obs-ffmpeg color-conversion).
 ******************************************************************************/

// Input YUV planes (shader resource views)
Texture2D<float>  planeY : register(t0);
Texture2D<float2> planeUV : register(t1);  // for NV12
Texture2D<float>  planeU : register(t2);   // for I420
Texture2D<float>  planeV : register(t3);   // for I420

// Output RGBA texture (unordered access view)
RWTexture2D<float4> outputRGBA : register(u0);

// Constants
cbuffer CSConstants : register(b0)
{
    uint width;
    uint height;
    uint format;       // 0 = I420, 1 = NV12
    uint colorspace;   // 0 = BT.601, 1 = BT.709, 2 = BT.2020
};

// BT.601 (SD) YUV → RGB matrix
static const float3x3 BT601_MATRIX = float3x3(
    1.164383,  0.000000,  1.596027,
    1.164383, -0.391762, -0.812968,
    1.164383,  2.017232,  0.000000
);

// BT.709 (HD) YUV → RGB matrix
static const float3x3 BT709_MATRIX = float3x3(
    1.164384,  0.000000,  1.792741,
    1.164384, -0.213249, -0.532909,
    1.164384,  2.112402,  0.000000
);

// BT.2020 (UHD) YUV → RGB matrix
static const float3x3 BT2020_MATRIX = float3x3(
    1.164384,  0.000000,  1.678674,
    1.164384, -0.187326, -0.650424,
    1.164384,  2.141772,  0.000000
);

float3 yuv_to_rgb(float3 yuv, uint cs)
{
    // Normalize YUV from [16, 235] (Y) and [16, 240] (UV) to [0, 1]
    yuv.x = (yuv.x - 0.062745) * 1.164384;  // (Y - 16/255) * 255/219
    yuv.yz = (yuv.yz - 0.501961);            // (U,V - 128/255)

    float3 rgb;
    if (cs == 0)       rgb = mul(BT601_MATRIX,  yuv);
    else if (cs == 1)  rgb = mul(BT709_MATRIX,  yuv);
    else               rgb = mul(BT2020_MATRIX, yuv);

    return saturate(rgb);  // clamp [0, 1]
}

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint x = dispatchThreadID.x;
    uint y = dispatchThreadID.y;

    if (x >= width || y >= height)
        return;

    // Sample Y plane (full resolution)
    float Y = planeY[uint2(x, y)];

    // Sample chroma planes (half resolution for 4:2:0 subsampling)
    uint2 uvCoord = uint2(x >> 1, y >> 1);
    float U, V;

    if (format == 1) {
        // NV12: interleaved UV
        float2 uv = planeUV[uvCoord];
        U = uv.x;
        V = uv.y;
    } else {
        // I420: separate U/V planes
        U = planeU[uvCoord];
        V = planeV[uvCoord];
    }

    float3 yuv = float3(Y, U, V);
    float3 rgb = yuv_to_rgb(yuv, colorspace);

    outputRGBA[uint2(x, y)] = float4(rgb, 1.0);
}
