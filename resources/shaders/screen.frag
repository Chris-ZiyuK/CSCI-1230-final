#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D sceneTex;
uniform sampler2D bloomTex;
uniform float bloomStrength;
uniform vec2 motionUV;     // screen-space motion direction in UV units (legacy approx)
uniform float motionAmount; // clamp for legacy motion
uniform sampler2D depthTex;
uniform mat4 currViewProjInv;
uniform mat4 prevViewProj;
uniform int blurEnabled;

void main()
{
    if (blurEnabled == 0) {
        vec3 sceneColor0 = texture(sceneTex, TexCoords).rgb;
        vec3 bloomColor0 = texture(bloomTex, TexCoords).rgb;
        FragColor = vec4(sceneColor0 + bloomStrength * bloomColor0, 1.0);
        return;
    }

    // Reconstruct velocity from depth (G-buffer generic)
    float depth = texture(depthTex, TexCoords).r;
    vec4 ndc = vec4(TexCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 worldPos = currViewProjInv * ndc;
    worldPos /= worldPos.w;
    vec4 prevClip = prevViewProj * worldPos;
    prevClip /= prevClip.w;
    vec2 vDepth = (ndc.xy - prevClip.xy) * 0.5; // screen velocity from depth

    vec2 dir = motionUV;
    float len = length(dir);
    if (len > 0.0) dir /= len;
    float legacyAmt = motionAmount;

    // combine legacy motion (camera/fish) and per-pixel velocity
    float depthLen = length(vDepth);
    vec2 vel = vDepth;
    if (depthLen < 1e-5) {
        vel = dir * legacyAmt;
    } else {
        // blend: depth-based dominant, plus small legacy
        vel = vDepth + dir * legacyAmt * 0.5;
    }
    float velLen = length(vel);
    vec2 motionDir = velLen > 1e-5 ? vel / velLen : vec2(0.0);
    float motionAmt = clamp(velLen, 0.0, 0.08); // keep similar strength
    // Boost far objects (stars/background) so their trail is more visible; near objects stay similar
    float depthBoost = smoothstep(0.5, 1.0, depth);          // far plane depth -> boost
    motionAmt *= mix(1.0, 2.8, depthBoost);                  // up to +180% for far depth
    // ensure a tiny minimum for very far stars
    float minTrail = mix(0.0, 0.004, depthBoost);
    motionAmt = max(motionAmt, minTrail);
    // strongly reduce foreground blur to avoid self-shadow/drag on titan wings
    float fgDamp = smoothstep(0.15, 0.35, depth); // near -> 0, mid -> 1
    motionAmt *= mix(0.1, 1.0, fgDamp);

    float depthCenter = depth;

    // motion blur: sample along motion direction (depth-aware)
    vec3 accum = vec3(0.0);
    float total = 0.0;
    vec2 stepUV = motionDir * motionAmt;

    // 5-tap symmetric blur, lighter weights to reduce smear
    const float weights[5] = float[](0.5, 0.2, 0.12, 0.1, 0.08);
    for (int i = -2; i <= 2; ++i) {
        float w = weights[abs(i)];
        vec2 offset = TexCoords + stepUV * float(i);
        vec3 sampleCol = texture(sceneTex, offset).rgb;
        float sampleDepth = texture(depthTex, offset).r;
        // depth-aware gate: reduce bleeding of background onto foreground
        float depthDiff = sampleDepth - depthCenter;
        // hard reject noticeably farther samples, keep equal/closer
        float gate = 0.0;
        if (depthDiff <= 0.002) {
            float adiff = abs(depthDiff);
            gate = smoothstep(0.0, 0.01, 0.02 - adiff);
        }
        float finalW = w * gate;
        accum += sampleCol * finalW;
        total += finalW;
    }
    vec3 sceneColor = (total > 0.0) ? accum / total : texture(sceneTex, TexCoords).rgb;

    vec3 bloomColor = texture(bloomTex, TexCoords).rgb;
    vec3 halo = bloomColor;
    vec3 result = sceneColor + bloomStrength * halo;
    FragColor = vec4(result, 1.0);
}
