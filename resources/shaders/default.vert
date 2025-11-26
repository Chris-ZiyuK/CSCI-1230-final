#version 330 core

layout(location = 0) in vec3 objectPos;
layout(location = 1) in vec3 objectNormal;

out vec3 worldPos;
out vec3 worldNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
    worldPos = vec3(model * vec4(objectPos, 1.0));

    // use inverse matrix
    mat3 inversedMatrix = mat3(transpose(inverse(model)));
    worldNormal = normalize(inversedMatrix * objectNormal);

    gl_Position = proj * view * model * vec4(objectPos, 1.0);
}
