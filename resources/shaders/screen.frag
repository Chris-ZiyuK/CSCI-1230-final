#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D sceneTex;
uniform sampler2D bloomTex;
uniform float bloomStrength;
uniform vec2 motionUV;     // screen-space motion direction in UV units
uniform float motionAmount; // 0..~0.05 typical

void main()
{
    // motion blur: sample along motion direction
    vec3 accum = vec3(0.0);
    float total = 0.0;
    vec2 dir = motionUV;
    float len = length(dir);
    if (len > 0.0) dir /= len;
    vec2 stepUV = dir * motionAmount;

    // 7-tap symmetric blur for stronger trail
    const float weights[7] = float[](0.26, 0.22, 0.16, 0.10, 0.10, 0.08, 0.08);
    for (int i = -3; i <= 3; ++i) {
        float w = weights[abs(i)];
        vec2 offset = TexCoords + stepUV * float(i);
        accum += texture(sceneTex, offset).rgb * w;
        total += w;
    }
    vec3 sceneColor = (total > 0.0) ? accum / total : texture(sceneTex, TexCoords).rgb;

    vec3 bloomColor = texture(bloomTex, TexCoords).rgb;
    vec3 halo = bloomColor;
    vec3 result = sceneColor + bloomStrength * halo;
    FragColor = vec4(result, 1.0);
}
