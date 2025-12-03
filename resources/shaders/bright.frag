#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D uScene;

void main()
{
    vec4 scene = texture(uScene, TexCoords);
    if (scene.a < 0.5) {
        FragColor = vec4(0.0);
        return;
    }

    vec3 color = scene.rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

    if (brightness > 0.7)
        FragColor = vec4(color, 1.0);
    else
        FragColor = vec4(0.0);
}
