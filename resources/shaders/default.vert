#version 330 core

layout(location = 0) in vec3 objectPos;
layout(location = 1) in vec3 objectNormal;

// For monster
layout(location = 2) in ivec4 boneIds;
layout(location = 3) in vec4 boneWeights;
layout(location = 4) in vec2 meshUV;

out vec3 worldPos;
out vec3 worldNormal;
// For monster
out vec2 fragTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

// For monster
uniform bool useSkinning;          // Bone index setting: GLB mesh=true, others are false
uniform mat4 boneMatrices[200];    // Upload the skeletal matrix (insufficient to fill in identity)

void main()
{
    // For monster
    vec4 finalPos = vec4(objectPos, 1.0);
    vec3 finalNormal = objectNormal;

    if (useSkinning && (boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w > 0.0)) {
        vec4 skinnedPos = vec4(0.0);
        vec3 skinnedNormal = vec3(0.0);

        vec4 normalizedWeights = boneWeights;
        float totalWeight = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;
        if (totalWeight > 0.0) normalizedWeights /= totalWeight;

        ivec4 ids = clamp(boneIds, ivec4(0), ivec4(199)); // 防止越界

        if (normalizedWeights.x > 0.0) {
            skinnedPos   += boneMatrices[ids.x] * vec4(objectPos, 1.0) * normalizedWeights.x;
            skinnedNormal += mat3(boneMatrices[ids.x]) * objectNormal * normalizedWeights.x;
        }
        if (normalizedWeights.y > 0.0) {
            skinnedPos   += boneMatrices[ids.y] * vec4(objectPos, 1.0) * normalizedWeights.y;
            skinnedNormal += mat3(boneMatrices[ids.y]) * objectNormal * normalizedWeights.y;
        }
        if (normalizedWeights.z > 0.0) {
            skinnedPos   += boneMatrices[ids.z] * vec4(objectPos, 1.0) * normalizedWeights.z;
            skinnedNormal += mat3(boneMatrices[ids.z]) * objectNormal * normalizedWeights.z;
        }
        if (normalizedWeights.w > 0.0) {
            skinnedPos   += boneMatrices[ids.w] * vec4(objectPos, 1.0) * normalizedWeights.w;
            skinnedNormal += mat3(boneMatrices[ids.w]) * objectNormal * normalizedWeights.w;
        }

        finalPos = skinnedPos;
        finalNormal = normalize(skinnedNormal);
    }



    //worldPos = vec3(model * vec4(objectPos, 1.0));
    // For monster
    worldPos = vec3(model * finalPos);

    // use inverse matrix
    mat3 inversedMatrix = mat3(transpose(inverse(model)));
    // worldNormal = normalize(inversedMatrix * objectNormal);
    // For monster
    worldNormal = normalize(inversedMatrix * finalNormal);
    fragTexCoord = meshUV;

    // gl_Position = proj * view * model * vec4(objectPos, 1.0);
    // For monster
    gl_Position = proj * view * vec4(worldPos, 1.0);
}
