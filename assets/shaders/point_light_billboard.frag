#version 460
#extension GL_GOOGLE_include_directive : require

#include "common/math.glsl"
#include "ubo/global_ubo.glsl"

layout(location = 0) in vec2 fragOffset;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
  vec4 position;
  vec4 color;
  float radius;
} push;

void main() {
    float dis = sqrt(dot(fragOffset, fragOffset));
    if (dis >= 1.0) {
        discard;
    }

    float alpha = 0.5 * (cos(dis * PI) + 1.0);

    outColor = vec4(push.color.xyz, alpha);
}