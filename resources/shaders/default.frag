#version 330 core

in vec3 worldPos;
in vec3 worldNormal;

out vec4 fragColor;

// ========== scene globals ===========
uniform float global_ka;
uniform float global_kd;
uniform float global_ks;

// ========== camera ===========
uniform vec3 cameraPos;

// ========== material ===========
uniform vec4 matAmbient;
uniform vec4 matDiffuse;
uniform vec4 matSpecular;
uniform float matShininess;
uniform vec4 matEmissive;
uniform bool useBackgroundTex;
uniform sampler2D backgroundTex;
uniform float timeSec;
uniform float bgScrollOffset;
uniform float starScrollSpeed;

// ========== light ===========
uniform vec3 lightPos;
uniform vec3 lightColor;

// For monster
in vec2 fragTexCoord;

uniform bool useMeshTexture;
uniform sampler2D meshTexture;
uniform bool useNormalMap;
uniform sampler2D normalMapTexture;
uniform vec3 meshEmissive;
uniform bool useMeshEmissiveTex;
uniform sampler2D meshEmissiveTex;
uniform bool enableStarfield;


// ===============================================
// STARFIELD — Procedural far-field galaxy glow
// ===============================================

// hash: deterministic pseudo-random
float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

// Simple 2D noise
float noise(vec2 p){
    vec2 i = floor(p);
    vec2 f = fract(p);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(mix(a, b, u.x),
               mix(c, d, u.x), u.y);
}


// ======================================
// Compute star color (in world space)
// ======================================
vec3 computeStarfield(vec3 wp)
{
    // Map the world coordinates to the sky texture coordinates
    // The larger, the sparser
    float scale = 0.03;
    vec2 uv = wp.xz * scale;
    uv.x += timeSec * starScrollSpeed;

    // Multi-layer noise superposition (galactic sensation)
    float n = 0.0;
    n += noise(uv * 1.0) * 0.6;
    n += noise(uv * 2.0) * 0.3;
    n += noise(uv * 4.0) * 0.1;

    // Only the brightest spots are stars
    float starMask = smoothstep(0.98, 1.0, n);

    if (starMask < 0.001)
        return vec3(0.0); // No stars

    // stars lightness (will do bright-pass → bloom)
    float brightness = pow(n, 50.0) * 4.0;

    // Star color: Light yellow,
    vec3 starColor = vec3(1.2, 1.15, 0.85) * brightness;

    return starColor;
}




const float PI = 3.14159265359;

vec2 dirToEquirectUV(vec3 dir) {
    float u = atan(dir.z, dir.x) / (2.0 * PI) + 0.5;
    float v = 0.5 - asin(clamp(dir.y, -1.0, 1.0)) / PI;
    return vec2(u, v);
}

vec2 zoomUV(vec2 uv, float zoom, vec2 offset) {
    return (uv - 0.5) * zoom + 0.5 + offset;
}

void main()
{
    if (useBackgroundTex) {
        const float bgZoom = 1.3;    // gentler zoom to reduce distortion
        const vec2 bgOffset = vec2(0.02, -0.03);
        vec3 dir = normalize(worldPos);
        vec2 uv = dirToEquirectUV(dir);
        uv.x = fract(uv.x + bgScrollOffset);
        uv = zoomUV(uv, bgZoom, bgOffset);
        uv.y = clamp(uv.y, 0.10, 0.90); // avoid pole regions that stretch the texture
        vec3 texColor = texture(backgroundTex, uv).rgb;
        fragColor = vec4(texColor, 0.0); // alpha=0 so bloom ignores background
        return;
    }

    // Original light（object's shading）
    vec3 N = normalize(worldNormal);
    vec3 V = normalize(cameraPos - worldPos);
    vec3 L = normalize(lightPos - worldPos);

    vec3 baseColor = matDiffuse.rgb;
    if (useMeshTexture) {
        if (!useNormalMap) {
            fragColor = texture(meshTexture, fragTexCoord);
            return;
        }
        baseColor = texture(meshTexture, fragTexCoord).rgb;
    }

    if (useMeshTexture && useNormalMap) {
        vec3 tangent = normalize(cross(N, vec3(0.0, 1.0, 0.0)));
        if (length(tangent) < 0.1) {
            tangent = normalize(cross(N, vec3(1.0, 0.0, 0.0)));
        }
        vec3 bitangent = normalize(cross(N, tangent));
        vec3 normalSample = texture(normalMapTexture, fragTexCoord).rgb * 2.0 - 1.0;
        N = normalize(tangent * normalSample.x + bitangent * normalSample.y + N * normalSample.z);
    }

    vec3 ambient = global_ka * matAmbient.rgb;
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = global_kd * diff * baseColor * lightColor;

    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), matShininess);
    vec3 specular = global_ks * spec * matSpecular.rgb * lightColor;

    vec3 shading = ambient + diffuse + specular;

    vec3 stars = enableStarfield ? computeStarfield(worldPos) : vec3(0.0);
vec3 emissive = matEmissive.rgb;
if (useMeshEmissiveTex) {
    emissive += meshEmissive * texture(meshEmissiveTex, fragTexCoord).rgb;
} else {
    emissive += meshEmissive;
}

    vec3 finalColor = shading + stars + emissive;

    fragColor = vec4(finalColor, 1.0);
}
