uniform texture2d remap_texture <string label = "remap_texture";>;
sampler_state uvshaderSampler {
        Filter      = Linear;
        AddressU    = Border;
        AddressV    = Border;
        BorderColor = 00000000;
};
float4 mainImage(VertData v_in) : TARGET
{
float uvX = (remap_texture.Sample(uvshaderSampler, v_in.uv).r);
float uvY = 1 - (remap_texture.Sample(uvshaderSampler, v_in.uv).g);
float2 uvmap = float2(uvX,uvY);
return image.Sample(uvshaderSampler, uvmap);
}