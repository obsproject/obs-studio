# Shader Compilation Script
# Compiles GLSL shaders to SPIR-V using glslc (from Vulkan SDK)

set -e

SHADER_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_DIR="$SHADER_DIR/compiled"

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo "Compiling Vulkan shaders..."

# Compile compute shader
echo "  stitch.comp -> stitch.comp.spv"
glslc -fshader-stage=compute "$SHADER_DIR/stitch.comp" -o "$OUTPUT_DIR/stitch.comp.spv"

# Compile vertex shader
echo "  stereo.vert -> stereo.vert.spv"
glslc -fshader-stage=vertex "$SHADER_DIR/stereo.vert" -o "$OUTPUT_DIR/stereo.vert.spv"

# Compile fragment shader
echo "  stereo.frag -> stereo.frag.spv"
glslc -fshader-stage=fragment "$SHADER_DIR/stereo.frag" -o "$OUTPUT_DIR/stereo.frag.spv"

# Compile preview SBS shader
echo "  preview_sbs.frag -> preview_sbs.frag.spv"
glslc -fshader-stage=fragment "$SHADER_DIR/preview_sbs.frag" -o "$OUTPUT_DIR/preview_sbs.frag.spv"

# Compile HDR tonemap shader
echo "  hdr_tonemap.comp -> hdr_tonemap.comp.spv"
glslc -fshader-stage=compute "$SHADER_DIR/hdr_tonemap.comp" -o "$OUTPUT_DIR/hdr_tonemap.comp.spv"

echo "Shader compilation complete!"
echo "Output: $OUTPUT_DIR"
