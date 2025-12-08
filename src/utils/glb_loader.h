#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <GL/glew.h>

// Forward declaration
namespace tinygltf {
    class Model;
}

// Structure to store a texture from GLB file
struct GLBTexture {
    GLuint textureId = 0;  // OpenGL texture ID
    int width = 0;
    int height = 0;
    int channels = 0;
    std::string path;      // For debugging
    bool loaded = false;
};

// Structure to store a material from GLB file
struct GLBMaterial {
    std::string name;
    
    // PBR material parameters (from glTF)
    glm::vec4 baseColorFactor = glm::vec4(1.0f);  // RGBA
    float metallicFactor = 0.0f;
    float roughnessFactor = 0.5f;
    glm::vec3 emissiveFactor = glm::vec3(0.0f);
    
    // Texture indices (into GLBModel::textures array)
    int baseColorTextureIndex = -1;  // Main texture
    int normalTextureIndex = -1;
    int emissiveTextureIndex = -1;
    int metallicRoughnessTextureIndex = -1;
    
    // Converted to Phong material parameters
    glm::vec3 ambient = glm::vec3(0.1f);
    glm::vec3 diffuse = glm::vec3(0.7f);
    glm::vec3 specular = glm::vec3(0.2f);
    float shininess = 32.0f;
    
    // Whether this material has a base color texture
    bool hasBaseColorTexture = false;
};

// Structure to store a single mesh from GLB file
struct GLBMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint jointsVbo = 0;  // Separate VBO for bone IDs (integers)
    int indexCount = 0;
    int materialIndex = -1;
    bool hasIndices = false;
    bool hasSkin = false;  // Whether this mesh has skinning data
};

// Structure to store a single joint in the skeleton
struct GLBJoint {
    int nodeIndex = -1;                    // Index into the nodes array
    std::string name;                      // Joint name
    glm::mat4 inverseBindMatrix{1.0f};    // Inverse bind matrix (T-pose to bind pose)
    glm::mat4 localTransform{1.0f};       // Local transformation matrix
    glm::mat4 globalTransform{1.0f};       // Global transformation matrix (computed)
    std::vector<int> children;              // Indices of child joints
    int parentIndex = -1;                  // Index of parent joint (-1 for root)
    
    // Pre-decomposed bind pose TRS (for performance, avoid decomposing every frame)
    glm::vec3 bindTranslation{0.0f};
    glm::quat bindRotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 bindScale{1.0f};
};

// Structure to store skeleton/skin data
struct GLBSkin {
    std::string name;
    std::vector<GLBJoint> joints;          // All joints in the skeleton
    int rootJointIndex = -1;               // Index of root joint
    int skeletonRootNodeIndex = -1;        // Skeleton root node index (if specified in glTF)
    std::vector<glm::mat4> boneMatrices;   // Final bone matrices for shader (computed)
    std::vector<glm::mat4> initialTransforms;  // Initial transforms for bind pose
    std::unordered_map<int, int> nodeToJointMap;  // Cached map: node index -> joint index (for performance)
    glm::mat4 skeletonRootTransform{1.0f}; // Transform of skeleton root node (if specified)
};

// Structure to store animation channel data
struct GLBAnimationChannel {
    int nodeIndex = -1;                    // Target node index
    std::string path;                      // "translation", "rotation", or "scale"
    std::vector<float> times;              // Keyframe times
    std::vector<glm::vec3> translations;  // Translation keyframes (if path == "translation")
    std::vector<glm::quat> rotations;     // Rotation keyframes (if path == "rotation")
    std::vector<glm::vec3> scales;        // Scale keyframes (if path == "scale")
    int interpolation = 0;                 // 0=LINEAR, 1=STEP, 2=CUBICSPLINE
};

// Structure to store a single animation
struct GLBAnimation {
    std::string name;
    float duration = 0.0f;                 // Animation duration in seconds
    std::vector<GLBAnimationChannel> channels;  // Animation channels
};

// Structure to store a complete GLB model
struct GLBModel {
    std::vector<GLBMesh> meshes;
    std::string filepath;
    bool loaded = false;
    
    // Material and texture data (Stage 3)
    std::vector<GLBMaterial> materials;
    std::vector<GLBTexture> textures;
    
    // Skeleton/skin data (Stage 4)
    GLBSkin skin;
    bool hasSkin = false;
    
    // Animation data (Stage 5)
    std::vector<GLBAnimation> animations;
};

// GLB file loader class
class GLBLoader {
public:
    // Load a GLB file and populate the model structure
    // @param filepath Path to the .glb file
    // @param model Output model structure
    // @return true if loading succeeded, false otherwise
    static bool loadGLB(const std::string& filepath, GLBModel& model);
    
    // Clean up OpenGL resources for a model
    // @param model Model to clean up
    static void cleanup(GLBModel& model);
    
    // Print basic information about the loaded model (for debugging)
    // @param model Model to print info about
    static void printModelInfo(const GLBModel& model);
    
    // Update animation and compute bone matrices (Stage 6-7)
    // @param model Model to update
    // @param currentTime Current animation time in seconds
    // @param animationIndex Index of animation to play (-1 for no animation)
    // @param ignoreRootTranslation If true, ignore translation on root joint
    // @return true if update succeeded
    static bool updateAnimation(GLBModel& model, float currentTime, int animationIndex = 0, bool ignoreRootTranslation = false);
};

