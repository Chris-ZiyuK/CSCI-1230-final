#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D sceneTex;
uniform sampler2D bloomTex;
uniform float bloomStrength;

void main()
{
    vec3 sceneColor = texture(sceneTex, TexCoords).rgb;
    vec3 bloomColor = texture(bloomTex, TexCoords).rgb;

    vec3 halo = bloomColor;
    vec3 result = sceneColor + bloomStrength * halo;
    FragColor = vec4(result, 1.0);
}
