#version 330 core

// Support both projA (simple shapes) and projB (GLB models) vertex formats
layout(location = 0) in vec3 objectPos;      // Position (projA) or pos (projB)
layout(location = 1) in vec3 objectNormal;   // Normal (projA) or norm (projB)

// Optional attributes for GLB models (may not exist for simple shapes)
layout(location = 2) in ivec4 joints;        // Bone indices (GLB only) - MUST be integer type
layout(location = 3) in vec4 weights;        // Bone weights (GLB only)
layout(location = 4) in vec2 texCoord;       // Texture coordinates (GLB only)

// Support both uniform naming conventions
uniform mat4 model;      // ProjA naming
uniform mat4 view;       // ProjA naming
uniform mat4 proj;       // ProjA naming
uniform mat4 m_model;    // ProjB naming (optional, for GLB)
uniform mat4 m_view;     // ProjB naming (optional, for GLB)
uniform mat4 m_proj;     // ProjB naming (optional, for GLB)

// Bone matrices for skinning (GLB models)
uniform mat4 u_boneMatrices[200];
uniform bool u_useSkinning = false;

// Outputs - support both naming conventions
out vec3 worldPos;              // ProjA naming
out vec3 worldNormal;           // ProjA naming
out vec3 fragPosWorld;          // ProjB naming
out vec3 fragNormalWorld;       // ProjB naming
out vec3 fragNormalView;        // ProjB naming
out vec3 fragPosView;           // ProjB naming
out vec2 fragTexCoord;          // ProjB naming (for GLB textures)

void main()
{
    vec3 pos = objectPos;
    vec3 norm = objectNormal;
    
    // Determine which uniform naming convention to use
    mat4 u_model = model;  // Default to projA naming
    mat4 u_view = view;
    mat4 u_proj = proj;
    
    // Check if projB uniforms are available (non-zero matrix indicates they exist)
    // Note: This is a workaround since we can't directly check uniform existence
    // We'll rely on the application to set the correct uniforms
    
    vec4 finalPos = vec4(pos, 1.0);
    vec3 finalNorm = norm;
    
    // Apply skinning if enabled and weights are non-zero (for GLB models)
    if (u_useSkinning && (weights.x > 0.0 || weights.y > 0.0 || weights.z > 0.0 || weights.w > 0.0)) {
        // Normalize weights
        float totalWeight = weights.x + weights.y + weights.z + weights.w;
        vec4 normalizedWeights = (totalWeight > 0.0) ? weights / totalWeight : weights;
        
        // Skinning: blend bone transformations weighted by vertex weights
        vec4 skinnedPos = vec4(0.0);
        vec3 skinnedNorm = vec3(0.0);
        
        // Clamp joint indices to valid range [0, 199]
        int idx0 = clamp(joints.x, 0, 199);
        int idx1 = clamp(joints.y, 0, 199);
        int idx2 = clamp(joints.z, 0, 199);
        int idx3 = clamp(joints.w, 0, 199);
        
        if (normalizedWeights.x > 0.0) {
            vec4 pos1 = u_boneMatrices[idx0] * vec4(pos, 1.0);
            skinnedPos += pos1 * normalizedWeights.x;
            skinnedNorm += mat3(u_boneMatrices[idx0]) * norm * normalizedWeights.x;
        }
        if (normalizedWeights.y > 0.0) {
            vec4 pos2 = u_boneMatrices[idx1] * vec4(pos, 1.0);
            skinnedPos += pos2 * normalizedWeights.y;
            skinnedNorm += mat3(u_boneMatrices[idx1]) * norm * normalizedWeights.y;
        }
        if (normalizedWeights.z > 0.0) {
            vec4 pos3 = u_boneMatrices[idx2] * vec4(pos, 1.0);
            skinnedPos += pos3 * normalizedWeights.z;
            skinnedNorm += mat3(u_boneMatrices[idx2]) * norm * normalizedWeights.z;
        }
        if (normalizedWeights.w > 0.0) {
            vec4 pos4 = u_boneMatrices[idx3] * vec4(pos, 1.0);
            skinnedPos += pos4 * normalizedWeights.w;
            skinnedNorm += mat3(u_boneMatrices[idx3]) * norm * normalizedWeights.w;
        }
        
        finalPos = skinnedPos;
        finalNorm = normalize(skinnedNorm);
    }
    
    // Transform to world space
    vec4 worldPos4 = u_model * finalPos;
    worldPos = worldPos4.xyz;
    
    // Transform normal to world space
    mat3 normalMatrix = mat3(transpose(inverse(u_model)));
    worldNormal = normalize(normalMatrix * finalNorm);
    
    // For ProjB compatibility (view space)
    vec4 viewPos = u_view * worldPos4;
    fragPosWorld = worldPos;
    fragPosView = viewPos.xyz;
    fragNormalWorld = worldNormal;
    mat3 normalMatrixView = mat3(u_view) * normalMatrix;
    fragNormalView = normalize(normalMatrixView * finalNorm);
    
    // Texture coordinates (for GLB models, default to (0,0) for simple shapes)
    fragTexCoord = texCoord;
    
    // Final position
    gl_Position = u_proj * u_view * worldPos4;
}
