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

// ========== light ===========
uniform vec3 lightPos;
uniform vec3 lightColor;


void main()
{
    vec3 N = normalize(worldNormal);
    vec3 V = normalize(cameraPos - worldPos);
    vec3 L = normalize(lightPos - worldPos);

    vec3 result = vec3(0.0);

    // ambient
    vec3 ambient = global_ka * matAmbient.rgb;

    // diffuse
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = global_kd * diff * matDiffuse.rgb * lightColor;

    // specular
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), matShininess);
    vec3 specular = global_ks * spec * matSpecular.rgb * lightColor;

    vec3 color = ambient + diffuse + specular;

    fragColor = vec4(color, 1.0);
}
