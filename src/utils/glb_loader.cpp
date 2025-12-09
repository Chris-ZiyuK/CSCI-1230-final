#include "glb_loader.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <set>
#include <functional>
#include <cmath>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/string_cast.hpp>

// Define implementations in this file
// Note: tinygltf will include stb_image.h and json, so we only need to define implementations
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

// Include tinygltf (it will handle including json and stb_image internally)
#include "tiny_gltf.h"

// Forward declarations for helper functions
static bool getAccessorData(const tinygltf::Model& model, int accessorIndex, std::vector<float>& outData);
static bool getIntegerAccessorData(const tinygltf::Model& model, int accessorIndex, std::vector<unsigned int>& outData);
static bool getIndexData(const tinygltf::Model& model, int accessorIndex, std::vector<unsigned int>& outIndices);
static bool processMeshes(const tinygltf::Model& gltfModel, GLBModel& model);
static bool processSkin(const tinygltf::Model& gltfModel, GLBModel& model);
static glm::mat4 getNodeTransform(const tinygltf::Node& node);
static bool processAnimations(const tinygltf::Model& gltfModel, GLBModel& model);
static bool processTextures(const tinygltf::Model& gltfModel, GLBModel& model, const std::string& glbFilePath);
static bool processMaterials(const tinygltf::Model& gltfModel, GLBModel& model);
static void convertPBRToPhong(const GLBMaterial& pbrMaterial, GLBMaterial& phongMaterial);
static std::string resolveTexturePath(const std::string& glbFilePath, const std::string& textureUri);

bool GLBLoader::loadGLB(const std::string& filepath, GLBModel& model) {
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    
    // Try to load as binary GLB first
    bool ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filepath);
    
    if (!warn.empty()) {
        std::cout << "GLB Loader Warning: " << warn << std::endl;
    }
    
    if (!err.empty()) {
        std::cerr << "GLB Loader Error: " << err << std::endl;
    }
    
    if (!ret) {
        std::cerr << "Failed to load GLB file: " << filepath << std::endl;
        return false;
    }
    
    // Store filepath
    model.filepath = filepath;
    
    // Print detailed info about the loaded model
    std::cout << "\n========================================" << std::endl;
    std::cout << "Successfully loaded GLB file: " << filepath << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Basic Statistics:" << std::endl;
    std::cout << "  Meshes: " << gltfModel.meshes.size() << std::endl;
    std::cout << "  Nodes: " << gltfModel.nodes.size() << std::endl;
    std::cout << "  Materials: " << gltfModel.materials.size() << std::endl;
    std::cout << "  Textures: " << gltfModel.textures.size() << std::endl;
    std::cout << "  Images: " << gltfModel.images.size() << std::endl;
    std::cout << "  Animations: " << gltfModel.animations.size() << std::endl;
    std::cout << "  Skins: " << gltfModel.skins.size() << std::endl;
    std::cout << "  Scenes: " << gltfModel.scenes.size() << std::endl;
    
    // Print mesh details
    if (!gltfModel.meshes.empty()) {
        std::cout << "\nMesh Details:" << std::endl;
        for (size_t i = 0; i < gltfModel.meshes.size(); ++i) {
            const auto& mesh = gltfModel.meshes[i];
            std::cout << "  Mesh " << i << ": " << (mesh.name.empty() ? "(unnamed)" : mesh.name) << std::endl;
            std::cout << "    Primitives: " << mesh.primitives.size() << std::endl;
            for (size_t j = 0; j < mesh.primitives.size(); ++j) {
                const auto& prim = mesh.primitives[j];
                std::cout << "      Primitive " << j << ":" << std::endl;
                std::cout << "        Attributes: " << prim.attributes.size() << std::endl;
                std::cout << "        Has indices: " << (prim.indices >= 0 ? "Yes" : "No") << std::endl;
                std::cout << "        Material: " << (prim.material >= 0 ? std::to_string(prim.material) : "None") << std::endl;
            }
        }
    }
    
    // Print animation details
    if (!gltfModel.animations.empty()) {
        std::cout << "\nAnimation Details:" << std::endl;
        for (size_t i = 0; i < gltfModel.animations.size(); ++i) {
            const auto& anim = gltfModel.animations[i];
            std::cout << "  Animation " << i << ": " << (anim.name.empty() ? "(unnamed)" : anim.name) << std::endl;
            std::cout << "    Channels: " << anim.channels.size() << std::endl;
            std::cout << "    Samplers: " << anim.samplers.size() << std::endl;
        }
    }
    
    // Print skin details
    if (!gltfModel.skins.empty()) {
        std::cout << "\nSkin Details:" << std::endl;
        for (size_t i = 0; i < gltfModel.skins.size(); ++i) {
            const auto& skin = gltfModel.skins[i];
            std::cout << "  Skin " << i << ": " << (skin.name.empty() ? "(unnamed)" : skin.name) << std::endl;
            std::cout << "    Joints: " << skin.joints.size() << std::endl;
        }
    }
    
    std::cout << "========================================\n" << std::endl;
    
    // Process textures first (needed by materials)
    if (!processTextures(gltfModel, model, filepath)) {
        std::cerr << "Warning: Failed to process some textures from GLB file" << std::endl;
    }
    
    // Process materials (needed by meshes)
    if (!processMaterials(gltfModel, model)) {
        std::cerr << "Warning: Failed to process some materials from GLB file" << std::endl;
    }
    
    // Process meshes and create OpenGL resources (Stage 2)
    if (!processMeshes(gltfModel, model)) {
        std::cerr << "Failed to process meshes from GLB file" << std::endl;
        return false;
    }
    
    // Process skeleton/skin data (Stage 4)
    if (!gltfModel.skins.empty()) {
        if (processSkin(gltfModel, model)) {
            model.hasSkin = true;
            std::cout << "Successfully processed skeleton with " << model.skin.joints.size() << " joints" << std::endl;
        } else {
            std::cerr << "Warning: Failed to process skeleton data" << std::endl;
        }
    }
    
    // Process animations (Stage 5)
    if (!gltfModel.animations.empty()) {
        if (processAnimations(gltfModel, model)) {
            std::cout << "Successfully processed " << model.animations.size() << " animations" << std::endl;
        } else {
            std::cerr << "Warning: Failed to process animation data" << std::endl;
        }
    }
    
    model.loaded = true;
    
    return true;
}

// Helper function to get data from accessor
static bool getAccessorData(const tinygltf::Model& model, int accessorIndex, 
                           std::vector<float>& outData) {
    if (accessorIndex < 0 || accessorIndex >= model.accessors.size()) {
        return false;
    }
    
    const auto& accessor = model.accessors[accessorIndex];
    if (accessor.bufferView < 0 || accessor.bufferView >= model.bufferViews.size()) {
        return false;
    }
    
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    if (bufferView.buffer < 0 || bufferView.buffer >= model.buffers.size()) {
        return false;
    }
    
    const auto& buffer = model.buffers[bufferView.buffer];
    
    // Calculate data offset
    size_t byteOffset = bufferView.byteOffset + accessor.byteOffset;
    size_t componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    size_t numComponents = tinygltf::GetNumComponentsInType(accessor.type);
    size_t stride = bufferView.byteStride > 0 ? bufferView.byteStride : (componentSize * numComponents);
    size_t count = accessor.count;
    
    outData.resize(count * numComponents);
    
    // Copy data
    for (size_t i = 0; i < count; ++i) {
        const unsigned char* src = buffer.data.data() + byteOffset + i * stride;
        for (size_t j = 0; j < numComponents; ++j) {
            float value = 0.0f;
            switch (accessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    value = *reinterpret_cast<const float*>(src + j * componentSize);
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    value = static_cast<float>(*reinterpret_cast<const unsigned short*>(src + j * componentSize));
                    break;
                case TINYGLTF_COMPONENT_TYPE_SHORT:
                    value = static_cast<float>(*reinterpret_cast<const short*>(src + j * componentSize));
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    value = static_cast<float>(*reinterpret_cast<const unsigned int*>(src + j * componentSize));
                    break;
                case TINYGLTF_COMPONENT_TYPE_INT:
                    value = static_cast<float>(*reinterpret_cast<const int*>(src + j * componentSize));
                    break;
                default:
                    return false;
            }
            outData[i * numComponents + j] = value;
        }
    }
    
    return true;
}

// Helper function to get integer data from accessor (for JOINTS_0)
static bool getIntegerAccessorData(const tinygltf::Model& model, int accessorIndex, 
                                  std::vector<unsigned int>& outData) {
    if (accessorIndex < 0 || accessorIndex >= model.accessors.size()) {
        return false;
    }
    
    const auto& accessor = model.accessors[accessorIndex];
    if (accessor.bufferView < 0 || accessor.bufferView >= model.bufferViews.size()) {
        return false;
    }
    
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    if (bufferView.buffer < 0 || bufferView.buffer >= model.buffers.size()) {
        return false;
    }
    
    const auto& buffer = model.buffers[bufferView.buffer];
    
    // Calculate data offset
    size_t byteOffset = bufferView.byteOffset + accessor.byteOffset;
    size_t componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    size_t numComponents = tinygltf::GetNumComponentsInType(accessor.type);
    size_t stride = bufferView.byteStride > 0 ? bufferView.byteStride : (componentSize * numComponents);
    size_t count = accessor.count;
    
    outData.resize(count * numComponents);
    
    // Copy data as integers
    for (size_t i = 0; i < count; ++i) {
        const unsigned char* src = buffer.data.data() + byteOffset + i * stride;
        for (size_t j = 0; j < numComponents; ++j) {
            unsigned int value = 0;
            switch (accessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    value = static_cast<unsigned int>(*reinterpret_cast<const unsigned char*>(src + j * componentSize));
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    value = static_cast<unsigned int>(*reinterpret_cast<const unsigned short*>(src + j * componentSize));
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    value = *reinterpret_cast<const unsigned int*>(src + j * componentSize);
                    break;
                case TINYGLTF_COMPONENT_TYPE_BYTE:
                    value = static_cast<unsigned int>(*reinterpret_cast<const char*>(src + j * componentSize));
                    break;
                case TINYGLTF_COMPONENT_TYPE_SHORT:
                    value = static_cast<unsigned int>(*reinterpret_cast<const short*>(src + j * componentSize));
                    break;
                case TINYGLTF_COMPONENT_TYPE_INT:
                    value = static_cast<unsigned int>(*reinterpret_cast<const int*>(src + j * componentSize));
                    break;
                default:
                    return false;
            }
            outData[i * numComponents + j] = value;
        }
    }
    
    return true;
}

// Helper function to get index data
static bool getIndexData(const tinygltf::Model& model, int accessorIndex,
                        std::vector<unsigned int>& outIndices) {
    if (accessorIndex < 0 || accessorIndex >= model.accessors.size()) {
        return false;
    }
    
    const auto& accessor = model.accessors[accessorIndex];
    if (accessor.bufferView < 0 || accessor.bufferView >= model.bufferViews.size()) {
        return false;
    }
    
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    if (bufferView.buffer < 0 || bufferView.buffer >= model.buffers.size()) {
        return false;
    }
    
    const auto& buffer = model.buffers[bufferView.buffer];
    
    size_t byteOffset = bufferView.byteOffset + accessor.byteOffset;
    size_t componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    size_t count = accessor.count;
    
    outIndices.resize(count);
    
    for (size_t i = 0; i < count; ++i) {
        const unsigned char* src = buffer.data.data() + byteOffset + i * componentSize;
        unsigned int index = 0;
        switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                index = static_cast<unsigned int>(*reinterpret_cast<const unsigned short*>(src));
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                index = *reinterpret_cast<const unsigned int*>(src);
                break;
            case TINYGLTF_COMPONENT_TYPE_SHORT:
                index = static_cast<unsigned int>(*reinterpret_cast<const short*>(src));
                break;
            case TINYGLTF_COMPONENT_TYPE_INT:
                index = static_cast<unsigned int>(*reinterpret_cast<const int*>(src));
                break;
            default:
                return false;
        }
        outIndices[i] = index;
    }
    
    return true;
}

// Process meshes and create OpenGL resources
static bool processMeshes(const tinygltf::Model& gltfModel, GLBModel& model) {
    model.meshes.clear();
    model.meshes.reserve(gltfModel.meshes.size());
    
    for (size_t meshIdx = 0; meshIdx < gltfModel.meshes.size(); ++meshIdx) {
        const auto& gltfMesh = gltfModel.meshes[meshIdx];
        std::cout << "Processing mesh " << meshIdx << ": " << (gltfMesh.name.empty() ? "(unnamed)" : gltfMesh.name) << std::endl;
        
        for (size_t primIdx = 0; primIdx < gltfMesh.primitives.size(); ++primIdx) {
            const auto& primitive = gltfMesh.primitives[primIdx];
            GLBMesh mesh;
            mesh.materialIndex = primitive.material;
            std::cout << "  Processing primitive " << primIdx << ", materialIndex=" << mesh.materialIndex << std::endl;
            
            // Get vertex positions
            std::vector<float> positions;
            if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
                if (!getAccessorData(gltfModel, primitive.attributes.at("POSITION"), positions)) {
                    std::cerr << "Failed to get POSITION data" << std::endl;
                    continue;
                }
            } else {
                std::cerr << "Mesh missing POSITION attribute" << std::endl;
                continue;
            }
            
            // Get normals
            std::vector<float> normals;
            if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                if (!getAccessorData(gltfModel, primitive.attributes.at("NORMAL"), normals)) {
                    std::cerr << "Warning: Failed to get NORMAL data, using default" << std::endl;
                    normals.resize(positions.size());
                    std::fill(normals.begin(), normals.end(), 0.0f);
                }
            } else {
                // Generate default normals
                normals.resize(positions.size());
                std::fill(normals.begin(), normals.end(), 0.0f);
            }
            
            // Get UV coordinates (TEXCOORD_0)
            std::vector<float> texCoords;
            bool hasTexCoords = false;
            if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                if (getAccessorData(gltfModel, primitive.attributes.at("TEXCOORD_0"), texCoords)) {
                    hasTexCoords = true;
                    // Note: glTF 2.0 texture coordinates use the same convention as OpenGL
                    // (bottom-left origin), so no flipping is needed
                    // Debug: check UV coordinate range
                    if (!texCoords.empty()) {
                        float minU = texCoords[0], maxU = texCoords[0];
                        float minV = texCoords[1], maxV = texCoords[1];
                        for (size_t i = 0; i < texCoords.size(); i += 2) {
                            minU = std::min(minU, texCoords[i]);
                            maxU = std::max(maxU, texCoords[i]);
                            minV = std::min(minV, texCoords[i+1]);
                            maxV = std::max(maxV, texCoords[i+1]);
                        }
                        std::cout << "    Mesh UV range: U=[" << minU << ", " << maxU << "], V=[" << minV << ", " << maxV << "]" << std::endl;
                    }
                }
            }
            
            // If no UV coordinates, generate default ones (0,0)
            if (!hasTexCoords) {
                size_t vertexCount = positions.size() / 3;
                texCoords.resize(vertexCount * 2);
                std::fill(texCoords.begin(), texCoords.end(), 0.0f);
                std::cout << "    Warning: Mesh has no UV coordinates, using default (0,0)" << std::endl;
            }
            
            // Get indices
            std::vector<unsigned int> indices;
            if (primitive.indices >= 0) {
                if (!getIndexData(gltfModel, primitive.indices, indices)) {
                    std::cerr << "Failed to get index data" << std::endl;
                    continue;
                }
                mesh.hasIndices = true;
                mesh.indexCount = static_cast<int>(indices.size());
            } else {
                // No indices, generate them
                indices.resize(positions.size() / 3);
                for (size_t i = 0; i < indices.size(); ++i) {
                    indices[i] = static_cast<unsigned int>(i);
                }
                mesh.hasIndices = false;
                mesh.indexCount = static_cast<int>(indices.size());
            }
            
            // Get skinning data (JOINTS_0 and WEIGHTS_0) if available
            // JOINTS_0 is typically UNSIGNED_BYTE or UNSIGNED_SHORT - read as integers directly
            // The indices in JOINTS_0 correspond to indices in the skin.joints array
            std::vector<unsigned int> joints;
            std::vector<float> weights;
            bool hasSkinData = false;
            
            if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end() &&
                primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
                if (getIntegerAccessorData(gltfModel, primitive.attributes.at("JOINTS_0"), joints) &&
                    getAccessorData(gltfModel, primitive.attributes.at("WEIGHTS_0"), weights)) {
                    hasSkinData = true;
                    mesh.hasSkin = true;
                    std::cout << "    Mesh has skinning data: " << joints.size() / 4 << " vertices with joints" << std::endl;
                }
            }
            
            // Interleave vertex data
            // Format: position(3) + normal(3) + texCoord(2) + weights(4) = 12 floats per vertex (if skinning)
            // Or: position(3) + normal(3) + texCoord(2) = 8 floats if no skinning
            size_t vertexCount = positions.size() / 3;
            size_t floatsPerVertex = hasSkinData ? 12 : 8;  // Added 2 for UV coordinates
            std::vector<float> interleavedData;
            interleavedData.reserve(vertexCount * floatsPerVertex);
            
            // Separate integer array for bone IDs (joints)
            std::vector<unsigned int> jointsData;
            if (hasSkinData) {
                jointsData.reserve(vertexCount * 4);
            }
            
            for (size_t i = 0; i < vertexCount; ++i) {
                // Position
                interleavedData.push_back(positions[i * 3 + 0]);
                interleavedData.push_back(positions[i * 3 + 1]);
                interleavedData.push_back(positions[i * 3 + 2]);
                // Normal
                interleavedData.push_back(normals[i * 3 + 0]);
                interleavedData.push_back(normals[i * 3 + 1]);
                interleavedData.push_back(normals[i * 3 + 2]);
                // Texture coordinates
                interleavedData.push_back(texCoords[i * 2 + 0]);
                interleavedData.push_back(texCoords[i * 2 + 1]);
                
                // Joints and weights (if available)
                if (hasSkinData) {
                    // Joints (4 integers) - already integers
                    // JOINTS_0 indices are relative to skin.joints array
                    // Note: We don't clamp here because the data should be valid from glTF
                    // Shader will clamp to [0, 199] for safety
                    if (i * 4 + 3 < joints.size()) {
                        // Use joints directly - they should be valid indices into the skin.joints array
                        jointsData.push_back(joints[i * 4 + 0]);
                        jointsData.push_back(joints[i * 4 + 1]);
                        jointsData.push_back(joints[i * 4 + 2]);
                        jointsData.push_back(joints[i * 4 + 3]);
                    } else {
                        jointsData.push_back(0);
                        jointsData.push_back(0);
                        jointsData.push_back(0);
                        jointsData.push_back(0);
                    }
                    // Weights (4 floats) - normalize to ensure they sum to 1.0
                    if (i * 4 + 3 < weights.size()) {
                        float w0 = weights[i * 4 + 0];
                        float w1 = weights[i * 4 + 1];
                        float w2 = weights[i * 4 + 2];
                        float w3 = weights[i * 4 + 3];
                        float totalWeight = w0 + w1 + w2 + w3;
                        // Normalize weights if they don't sum to 1.0 (or are all zero)
                        if (totalWeight > 0.0001f) {
                            w0 /= totalWeight;
                            w1 /= totalWeight;
                            w2 /= totalWeight;
                            w3 /= totalWeight;
                        } else {
                            // If all weights are zero, use first bone with weight 1.0
                            w0 = 1.0f;
                            w1 = 0.0f;
                            w2 = 0.0f;
                            w3 = 0.0f;
                        }
                        interleavedData.push_back(w0);
                        interleavedData.push_back(w1);
                        interleavedData.push_back(w2);
                        interleavedData.push_back(w3);
                    } else {
                        // No weights - use first bone with full weight
                        interleavedData.push_back(1.0f);
                        interleavedData.push_back(0.0f);
                        interleavedData.push_back(0.0f);
                        interleavedData.push_back(0.0f);
                    }
                }
            }
            
            // Create OpenGL resources
            glGenVertexArrays(1, &mesh.vao);
            glGenBuffers(1, &mesh.vbo);
            if (mesh.hasIndices) {
                glGenBuffers(1, &mesh.ebo);
            }
            if (hasSkinData) {
                glGenBuffers(1, &mesh.jointsVbo);
            }
            
            glBindVertexArray(mesh.vao);
            
            // Upload vertex data (positions, normals, weights)
            glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
            glBufferData(GL_ARRAY_BUFFER, 
                        interleavedData.size() * sizeof(float),
                        interleavedData.data(),
                        GL_STATIC_DRAW);
            
            // Set vertex attributes
            size_t offset = 0;
            // Position (location 0)
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, floatsPerVertex * sizeof(float), (void*)offset);
            glEnableVertexAttribArray(0);
            offset += 3 * sizeof(float);
            
            // Normal (location 1)
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, floatsPerVertex * sizeof(float), (void*)offset);
            glEnableVertexAttribArray(1);
            offset += 3 * sizeof(float);
            
            // Texture coordinates (location 4)
            glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, floatsPerVertex * sizeof(float), (void*)offset);
            glEnableVertexAttribArray(4);
            offset += 2 * sizeof(float);
            
            // Joints (location 2) - CRITICAL: Use glVertexAttribIPointer for integers!
            if (hasSkinData) {
                // Upload joints data as integers to separate VBO
                glBindBuffer(GL_ARRAY_BUFFER, mesh.jointsVbo);
                glBufferData(GL_ARRAY_BUFFER,
                            jointsData.size() * sizeof(unsigned int),
                            jointsData.data(),
                            GL_STATIC_DRAW);
                // Use glVertexAttribIPointer (with 'I') for integer attributes
                glVertexAttribIPointer(2, 4, GL_UNSIGNED_INT, 0, (void*)0);
                glEnableVertexAttribArray(2);
                
                // Weights (location 3) - back to main VBO
                glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
                glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, floatsPerVertex * sizeof(float), (void*)offset);
                glEnableVertexAttribArray(3);
            }
            
            // Upload index data if present
            if (mesh.hasIndices) {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                            indices.size() * sizeof(unsigned int),
                            indices.data(),
                            GL_STATIC_DRAW);
            }
            
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            if (mesh.hasIndices) {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            }
            
            model.meshes.push_back(mesh);
            std::cout << "    Created mesh: materialIndex=" << mesh.materialIndex 
                      << ", hasTexCoords=" << (hasTexCoords ? "yes" : "no")
                      << ", vertexCount=" << vertexCount << std::endl;
            std::cout << "Created mesh with " << vertexCount << " vertices, " 
                      << mesh.indexCount << " indices" << std::endl;
        }
    }
    
    return !model.meshes.empty();
}

// Helper function to get node transformation matrix
static glm::mat4 getNodeTransform(const tinygltf::Node& node) {
    glm::mat4 transform{1.0f};
    
    // Apply translation
    if (node.translation.size() == 3) {
        glm::mat4 translation = glm::mat4(1.0f);
        translation[3] = glm::vec4(node.translation[0], node.translation[1], node.translation[2], 1.0f);
        transform = transform * translation;
    }
    
    // Apply rotation (quaternion)
    if (node.rotation.size() == 4) {
        glm::quat rot(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
        transform = transform * glm::mat4_cast(rot);
    }
    
    // Apply scale
    if (node.scale.size() == 3) {
        glm::mat4 scale = glm::mat4(1.0f);
        scale[0][0] = node.scale[0];
        scale[1][1] = node.scale[1];
        scale[2][2] = node.scale[2];
        transform = transform * scale;
    }
    
    // Apply matrix (if present, overrides TRS)
    if (node.matrix.size() == 16) {
        glm::mat4 matrix;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                matrix[i][j] = static_cast<float>(node.matrix[i * 4 + j]);
            }
        }
        transform = matrix;
    }
    
    return transform;
}

// Process skeleton/skin data
static bool processSkin(const tinygltf::Model& gltfModel, GLBModel& model) {
    if (gltfModel.skins.empty()) {
        return false;
    }
    
    // For now, process the first skin (most models have only one)
    const auto& gltfSkin = gltfModel.skins[0];
    model.skin.name = gltfSkin.name;
    
    // Get skeleton root node if specified
    // In glTF, inverse bind matrices are relative to the skeleton root node
    model.skin.skeletonRootNodeIndex = gltfSkin.skeleton;
    if (model.skin.skeletonRootNodeIndex >= 0 && 
        model.skin.skeletonRootNodeIndex < gltfModel.nodes.size()) {
        const auto& skeletonRootNode = gltfModel.nodes[model.skin.skeletonRootNodeIndex];
        model.skin.skeletonRootTransform = getNodeTransform(skeletonRootNode);
        std::cout << "  Skeleton root node: " << model.skin.skeletonRootNodeIndex << std::endl;
    } else {
        model.skin.skeletonRootTransform = glm::mat4(1.0f);
    }
    
    // Get inverse bind matrices
    // glTF stores matrices in column-major order (16 floats per matrix)
    // Format: [m00, m10, m20, m30, m01, m11, m21, m31, m02, m12, m22, m32, m03, m13, m23, m33]
    // glm::mat4 uses column-major: mat[col][row] where col is column index, row is row index
    std::vector<glm::mat4> inverseBindMatrices;
    if (gltfSkin.inverseBindMatrices >= 0) {
        std::vector<float> matrixData;
        if (getAccessorData(gltfModel, gltfSkin.inverseBindMatrices, matrixData)) {
            size_t matrixCount = matrixData.size() / 16;
            inverseBindMatrices.resize(matrixCount);
            for (size_t i = 0; i < matrixCount; ++i) {
                // glTF stores matrices in column-major order: [m00, m10, m20, m30, m01, m11, ...]
                // glm::mat4 is also column-major, accessed as mat[col][row]
                // So matrixData[i*16 + col*4 + row] gives us element at column col, row row
                for (int col = 0; col < 4; ++col) {
                    for (int row = 0; row < 4; ++row) {
                        inverseBindMatrices[i][col][row] = matrixData[i * 16 + col * 4 + row];
                    }
                }
            }
            std::cout << "  Loaded " << inverseBindMatrices.size() << " inverse bind matrices" << std::endl;
        }
    }
    
    // Create joints
    model.skin.joints.clear();
    model.skin.joints.reserve(gltfSkin.joints.size());
    
    // Build joint map for quick lookup
    std::unordered_map<int, int> nodeIndexToJointIndex;
    
    // Store initial transforms for bind pose (needed when no animation is playing)
    std::vector<glm::mat4> initialTransforms;
    initialTransforms.reserve(gltfSkin.joints.size());
    
    for (size_t i = 0; i < gltfSkin.joints.size(); ++i) {
        int nodeIndex = gltfSkin.joints[i];
        if (nodeIndex < 0 || nodeIndex >= gltfModel.nodes.size()) {
            continue;
        }
        
        GLBJoint joint;
        joint.nodeIndex = nodeIndex;
        const auto& node = gltfModel.nodes[nodeIndex];
        joint.name = node.name;
        
        // Get and store initial transform from node
        glm::mat4 initialTransform = getNodeTransform(node);
        joint.localTransform = initialTransform;
        initialTransforms.push_back(initialTransform);
        
        // Pre-decompose bind pose transform (do this once at load time, not every frame)
        glm::vec3 bindT, bindS, bindSkew;
        glm::quat bindR;
        glm::vec4 bindPerspective;
        glm::decompose(initialTransform, bindS, bindR, bindT, bindSkew, bindPerspective);
        joint.bindTranslation = bindT;
        joint.bindRotation = bindR;
        joint.bindScale = bindS;
        
        // Set inverse bind matrix
        if (i < inverseBindMatrices.size()) {
            joint.inverseBindMatrix = inverseBindMatrices[i];
        } else {
            joint.inverseBindMatrix = glm::mat4(1.0f);
        }
        
        // Initialize global transform to local transform
        joint.globalTransform = joint.localTransform;
        
        model.skin.joints.push_back(joint);
        nodeIndexToJointIndex[nodeIndex] = static_cast<int>(i);
    }
    
    // Build parent-child relationships by traversing node hierarchy
    // Find root joint (joint that is not a child of any other joint in the skin)
    std::set<int> childNodes;
    for (const auto& node : gltfModel.nodes) {
        for (int childIndex : node.children) {
            childNodes.insert(childIndex);
        }
    }
    
    // Find root joint (a joint whose node is not a child of any other node in the skin)
    for (size_t i = 0; i < model.skin.joints.size(); ++i) {
        int nodeIndex = model.skin.joints[i].nodeIndex;
        if (childNodes.find(nodeIndex) == childNodes.end()) {
            // This node is not a child, it might be a root
            // But we need to check if its parent is also in the skin
            bool isRoot = true;
            for (const auto& node : gltfModel.nodes) {
                for (int childIdx : node.children) {
                    if (childIdx == nodeIndex) {
                        // Check if parent is in skin
                        int parentNodeIndex = -1;
                        for (size_t j = 0; j < gltfModel.nodes.size(); ++j) {
                            const auto& n = gltfModel.nodes[j];
                            for (int c : n.children) {
                                if (c == nodeIndex) {
                                    parentNodeIndex = static_cast<int>(j);
                                    break;
                                }
                            }
                            if (parentNodeIndex >= 0) break;
                        }
                        if (parentNodeIndex >= 0 && nodeIndexToJointIndex.find(parentNodeIndex) != nodeIndexToJointIndex.end()) {
                            isRoot = false;
                        }
                        break;
                    }
                }
                if (!isRoot) break;
            }
            if (isRoot) {
                model.skin.rootJointIndex = static_cast<int>(i);
                break;
            }
        }
    }
    
    // If no root found, use first joint
    if (model.skin.rootJointIndex < 0 && !model.skin.joints.empty()) {
        model.skin.rootJointIndex = 0;
    }
    
    // Build parent-child relationships
    // First, find parents by checking which nodes have this joint's node as a child
    for (size_t i = 0; i < model.skin.joints.size(); ++i) {
        int nodeIndex = model.skin.joints[i].nodeIndex;
        if (nodeIndex < 0 || nodeIndex >= gltfModel.nodes.size()) {
            continue;
        }
        
        // Find parent node
        int parentNodeIndex = -1;
        for (size_t j = 0; j < gltfModel.nodes.size(); ++j) {
            const auto& node = gltfModel.nodes[j];
            for (int childIdx : node.children) {
                if (childIdx == nodeIndex) {
                    parentNodeIndex = static_cast<int>(j);
                    break;
                }
            }
            if (parentNodeIndex >= 0) break;
        }
        
        // If parent is also in the skin, establish the relationship
        if (parentNodeIndex >= 0) {
            auto parentIt = nodeIndexToJointIndex.find(parentNodeIndex);
            if (parentIt != nodeIndexToJointIndex.end()) {
                int parentJointIndex = parentIt->second;
                // Avoid duplicate children
                bool alreadyChild = false;
                for (int child : model.skin.joints[parentJointIndex].children) {
                    if (child == static_cast<int>(i)) {
                        alreadyChild = true;
                        break;
                    }
                }
                if (!alreadyChild) {
                    model.skin.joints[parentJointIndex].children.push_back(static_cast<int>(i));
                }
                model.skin.joints[i].parentIndex = parentJointIndex;
            }
        }
        
        // Also add direct children from the node's children list
        const auto& node = gltfModel.nodes[nodeIndex];
        for (int childNodeIndex : node.children) {
            auto it = nodeIndexToJointIndex.find(childNodeIndex);
            if (it != nodeIndexToJointIndex.end()) {
                int childJointIndex = it->second;
                // Avoid duplicate children
                bool alreadyChild = false;
                for (int child : model.skin.joints[i].children) {
                    if (child == childJointIndex) {
                        alreadyChild = true;
                        break;
                    }
                }
                if (!alreadyChild) {
                    model.skin.joints[i].children.push_back(childJointIndex);
                }
                // Set parent if not already set
                if (model.skin.joints[childJointIndex].parentIndex < 0) {
                    model.skin.joints[childJointIndex].parentIndex = static_cast<int>(i);
                }
            }
        }
    }
    
    // Initialize bone matrices array
    model.skin.boneMatrices.resize(model.skin.joints.size(), glm::mat4(1.0f));
    
    // Store initial transforms for bind pose
    model.skin.initialTransforms = initialTransforms;
    
    // Cache node to joint mapping for performance (built once at load time)
    model.skin.nodeToJointMap.clear();
    model.skin.nodeToJointMap.reserve(model.skin.joints.size());
    for (size_t i = 0; i < model.skin.joints.size(); ++i) {
        model.skin.nodeToJointMap[model.skin.joints[i].nodeIndex] = static_cast<int>(i);
    }
    
    // Debug: print skeleton hierarchy (only first few joints to avoid spam)
    std::cout << "Skeleton structure:" << std::endl;
    std::cout << "  Root joint: " << model.skin.rootJointIndex << std::endl;
    std::cout << "  Total joints: " << model.skin.joints.size() << std::endl;
    
    // Print first 10 joints and their hierarchy
    // std::cout << "  Joint hierarchy (first 10):" << std::endl;
    // for (size_t i = 0; i < std::min(model.skin.joints.size(), size_t(10)); ++i) {
    //     const auto& joint = model.skin.joints[i];
    //     std::cout << "    Joint " << i << " (" << joint.name << "): parent=" << joint.parentIndex
    //               << ", children=[";
    //     for (size_t j = 0; j < std::min(joint.children.size(), size_t(5)); ++j) {
    //         std::cout << joint.children[j];
    //         if (j < joint.children.size() - 1 && j < 4) std::cout << ", ";
    //     }
    //     if (joint.children.size() > 5) std::cout << "...";
    //     std::cout << "]" << std::endl;
    // }
    
    // Count joints with no parent (should be 1 for root)
    int rootCount = 0;
    for (const auto& joint : model.skin.joints) {
        if (joint.parentIndex < 0) {
            rootCount++;
        }
    }
    std::cout << "  Joints with no parent: " << rootCount << " (should be 1)" << std::endl;
    
    return true;
}

// Process animations
static bool processAnimations(const tinygltf::Model& gltfModel, GLBModel& model) {
    model.animations.clear();
    model.animations.reserve(gltfModel.animations.size());
    
    for (const auto& gltfAnim : gltfModel.animations) {
        GLBAnimation animation;
        animation.name = gltfAnim.name;
        
        // Find maximum time to determine duration
        float maxTime = 0.0f;
        
        // Process each channel
        for (const auto& channel : gltfAnim.channels) {
            if (channel.sampler < 0 || channel.sampler >= gltfAnim.samplers.size()) {
                continue;
            }
            
            const auto& sampler = gltfAnim.samplers[channel.sampler];
            GLBAnimationChannel animChannel;
            animChannel.nodeIndex = channel.target_node;
            animChannel.path = channel.target_path;
            
            // Get interpolation type
            if (sampler.interpolation == "LINEAR") {
                animChannel.interpolation = 0;
            } else if (sampler.interpolation == "STEP") {
                animChannel.interpolation = 1;
            } else if (sampler.interpolation == "CUBICSPLINE") {
                animChannel.interpolation = 2;
            }
            
            // Get time keyframes
            if (sampler.input >= 0) {
                std::vector<float> timeData;
                if (getAccessorData(gltfModel, sampler.input, timeData)) {
                    animChannel.times = timeData;
                    if (!timeData.empty()) {
                        maxTime = std::max(maxTime, timeData.back());
                    }
                }
            }
            
            // Get value keyframes based on path type
            if (sampler.output >= 0) {
                std::vector<float> valueData;
                if (getAccessorData(gltfModel, sampler.output, valueData)) {
                    if (animChannel.path == "translation" || animChannel.path == "scale") {
                        // Translation and scale are vec3
                        size_t keyframeCount = valueData.size() / 3;
                        animChannel.translations.reserve(keyframeCount);
                        animChannel.scales.reserve(keyframeCount);
                        
                        for (size_t i = 0; i < keyframeCount; ++i) {
                            glm::vec3 vec(valueData[i * 3 + 0], valueData[i * 3 + 1], valueData[i * 3 + 2]);
                            if (animChannel.path == "translation") {
                                animChannel.translations.push_back(vec);
                            } else {
                                animChannel.scales.push_back(vec);
                            }
                        }
                    } else if (animChannel.path == "rotation") {
                        // Rotation is quaternion (xyzw)
                        size_t keyframeCount = valueData.size() / 4;
                        animChannel.rotations.reserve(keyframeCount);
                        
                        for (size_t i = 0; i < keyframeCount; ++i) {
                            glm::quat quat(valueData[i * 4 + 3],  // w
                                          valueData[i * 4 + 0],  // x
                                          valueData[i * 4 + 1],  // y
                                          valueData[i * 4 + 2]); // z
                            animChannel.rotations.push_back(quat);
                        }
                    }
                }
            }
            
            animation.channels.push_back(animChannel);
        }
        
        animation.duration = maxTime;
        model.animations.push_back(animation);
        
        // Only print animation info once, not every frame
        if (model.animations.size() == 1) {
            std::cout << "  Animation: " << animation.name 
                      << " (duration: " << animation.duration << "s, "
                      << "channels: " << animation.channels.size() << ")" << std::endl;
        }
    }
    
    return !model.animations.empty();
}

// Helper function for linear interpolation
static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

static glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t) {
    return a + (b - a) * t;
}

static glm::quat slerp(const glm::quat& a, const glm::quat& b, float t) {
    return glm::slerp(a, b, t);
}

// Interpolate animation channel at given time
static void interpolateChannel(const GLBAnimationChannel& channel, float time, 
                               glm::vec3& outTranslation, glm::quat& outRotation, glm::vec3& outScale) {
    if (channel.times.empty()) {
        return;
    }
    
    // Clamp time to animation range
    time = std::max(channel.times[0], std::min(time, channel.times.back()));
    
    // Find the two keyframes to interpolate between
    size_t keyframeIndex = 0;
    for (size_t i = 0; i < channel.times.size() - 1; ++i) {
        if (time <= channel.times[i + 1]) {
            keyframeIndex = i;
            break;
        }
    }
    
    if (keyframeIndex >= channel.times.size() - 1) {
        keyframeIndex = channel.times.size() - 2;
    }
    
    float t0 = channel.times[keyframeIndex];
    float t1 = channel.times[keyframeIndex + 1];
    float t = (t1 > t0) ? (time - t0) / (t1 - t0) : 0.0f;
    t = std::max(0.0f, std::min(1.0f, t));
    
    // Interpolate based on path type
    if (channel.path == "translation" && keyframeIndex < channel.translations.size() - 1) {
        outTranslation = lerp(channel.translations[keyframeIndex], 
                             channel.translations[keyframeIndex + 1], t);
    } else if (channel.path == "rotation" && keyframeIndex < channel.rotations.size() - 1) {
        outRotation = slerp(channel.rotations[keyframeIndex], 
                           channel.rotations[keyframeIndex + 1], t);
    } else if (channel.path == "scale" && keyframeIndex < channel.scales.size() - 1) {
        outScale = lerp(channel.scales[keyframeIndex], 
                       channel.scales[keyframeIndex + 1], t);
    }
}

// Update animation and compute bone matrices
bool GLBLoader::updateAnimation(GLBModel& model, float currentTime, int animationIndex, bool ignoreRootTranslation) {
    if (!model.hasSkin || model.skin.joints.empty()) {
        return false;
    }
    
    if (animationIndex < 0 || animationIndex >= static_cast<int>(model.animations.size())) {
        // No animation or invalid index, use bind pose (initial transforms from nodes)
        // Restore initial transforms
        for (size_t i = 0; i < model.skin.joints.size() && i < model.skin.initialTransforms.size(); ++i) {
            model.skin.joints[i].localTransform = model.skin.initialTransforms[i];
        }
        
        // Recompute global transforms from hierarchy
        std::function<void(int)> computeGlobalTransform = [&](int jointIndex) {
            auto& joint = model.skin.joints[jointIndex];
            
            if (joint.parentIndex >= 0 && joint.parentIndex < static_cast<int>(model.skin.joints.size())) {
                joint.globalTransform = model.skin.joints[joint.parentIndex].globalTransform * joint.localTransform;
            } else {
                joint.globalTransform = joint.localTransform;
            }
            
            for (int childIndex : joint.children) {
                if (childIndex >= 0 && childIndex < static_cast<int>(model.skin.joints.size())) {
                    computeGlobalTransform(childIndex);
                }
            }
        };
        
        if (model.skin.rootJointIndex >= 0 && model.skin.rootJointIndex < static_cast<int>(model.skin.joints.size())) {
            computeGlobalTransform(model.skin.rootJointIndex);
        } else if (!model.skin.joints.empty()) {
            computeGlobalTransform(0);
        }
    } else {
        const auto& animation = model.animations[animationIndex];
        
        // Loop animation time
        float animTime = std::fmod(currentTime, animation.duration);
        if (animTime < 0.0f) {
            animTime += animation.duration;
        }
        
        // Use cached node-to-joint map (built at load time, no per-frame overhead)
        const auto& nodeToJoint = model.skin.nodeToJointMap;
        
        // Reset all joints to bind pose (use pre-decomposed TRS from load time)
        for (auto& joint : model.skin.joints) {
            joint.localTransform = glm::translate(glm::mat4(1.0f), joint.bindTranslation) * 
                                  glm::mat4_cast(joint.bindRotation) * 
                                  glm::scale(glm::mat4(1.0f), joint.bindScale);
        }
        
        // Apply animation channels - only update joints that have animation
        for (const auto& channel : animation.channels) {
            auto it = nodeToJoint.find(channel.nodeIndex);
            if (it == nodeToJoint.end()) {
                continue; // Node not in skin
            }
            
            int jointIndex = it->second;
            auto& joint = model.skin.joints[jointIndex];
            
            // Interpolate channel values
            glm::vec3 translation(0.0f);
            glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
            glm::vec3 scale(1.0f);
            
            interpolateChannel(channel, animTime, translation, rotation, scale);
            
            // Get current TRS from joint's bind pose
            glm::vec3 t = joint.bindTranslation;
            glm::quat r = joint.bindRotation;
            glm::vec3 s = joint.bindScale;
            
            // Update based on channel path
            // if ignoreRootTranslation is true and this is root joint, skip translation
            bool isRootJoint = (jointIndex == model.skin.rootJointIndex) || 
                              (model.skin.rootJointIndex < 0 && jointIndex == 0);
            
            if (channel.path == "translation") {
                if (!ignoreRootTranslation || !isRootJoint) {
                t = translation;
                }
                // else: keep bind translation for root joint
            } else if (channel.path == "rotation") {
                r = rotation;
            } else if (channel.path == "scale") {
                s = scale;
            }
            
            // Rebuild local transform: T * R * S
            joint.localTransform = glm::translate(glm::mat4(1.0f), t) * 
                                  glm::mat4_cast(r) * 
                                  glm::scale(glm::mat4(1.0f), s);
        }
        
        // Compute global transforms recursively
        // Global transform = parent's global transform * local transform
        std::function<void(int)> computeGlobalTransform = [&](int jointIndex) {
            auto& joint = model.skin.joints[jointIndex];
            
            if (joint.parentIndex >= 0 && joint.parentIndex < static_cast<int>(model.skin.joints.size())) {
                // Global = parent global * local
                joint.globalTransform = model.skin.joints[joint.parentIndex].globalTransform * joint.localTransform;
            } else {
                // Root joint: global = local
                joint.globalTransform = joint.localTransform;
            }
            
            // Recursively compute children
            for (int childIndex : joint.children) {
                if (childIndex >= 0 && childIndex < static_cast<int>(model.skin.joints.size())) {
                    computeGlobalTransform(childIndex);
                }
            }
        };
        
        // Start from root
        if (model.skin.rootJointIndex >= 0 && model.skin.rootJointIndex < static_cast<int>(model.skin.joints.size())) {
            computeGlobalTransform(model.skin.rootJointIndex);
        } else if (!model.skin.joints.empty()) {
            // Fallback: compute from first joint if root not found
            computeGlobalTransform(0);
        }
    }
    
    // Compute final bone matrices for skinning
    // In glTF 2.0, the bone matrix transforms vertices from bind pose to current pose
    // According to glTF spec: boneMatrix = globalTransform * inverseBindMatrix
    // The inverseBindMatrix is relative to the skeleton root (if specified)
    // The globalTransform already includes the skeleton root transform if skeleton root is in the hierarchy
    // However, if skeleton root is NOT in the joint hierarchy, we need to include it separately
    for (size_t i = 0; i < model.skin.joints.size(); ++i) {
        const auto& globalTransform = model.skin.joints[i].globalTransform;
        const auto& inverseBind = model.skin.joints[i].inverseBindMatrix;
        
        // Check if skeleton root is in the joint hierarchy
        bool skeletonRootInHierarchy = false;
        if (model.skin.skeletonRootNodeIndex >= 0) {
            for (const auto& joint : model.skin.joints) {
                if (joint.nodeIndex == model.skin.skeletonRootNodeIndex) {
                    skeletonRootInHierarchy = true;
                    break;
                }
            }
        }
        
        // Standard glTF formula: boneMatrix = globalTransform * inverseBindMatrix
        // If skeleton root is NOT in hierarchy, we need to include its transform
        if (!skeletonRootInHierarchy && model.skin.skeletonRootNodeIndex >= 0) {
            // Skeleton root is outside the joint hierarchy, include its transform
            model.skin.boneMatrices[i] = model.skin.skeletonRootTransform * globalTransform * inverseBind;
        } else {
            // Standard formula (skeleton root is in hierarchy or not specified)
            model.skin.boneMatrices[i] = globalTransform * inverseBind;
        }
    }
    
    return true;
}

// Helper function to resolve texture path relative to GLB file
static std::string resolveTexturePath(const std::string& glbFilePath, const std::string& textureUri) {
    // If URI is absolute path, use it directly
    if (textureUri.find("/") == 0 || (textureUri.length() > 2 && textureUri[1] == ':')) {
        return textureUri;
    }
    
    // Get directory of GLB file
    std::string glbDir = glbFilePath.substr(0, glbFilePath.find_last_of("/\\"));
    if (glbDir.empty()) {
        glbDir = ".";
    }
    
    // Try relative to GLB file directory
    std::string texturePath = glbDir + "/" + textureUri;
    
    // Check if file exists (simple check - try to open)
    std::ifstream testFile(texturePath);
    if (testFile.good()) {
        testFile.close();
        return texturePath;
    }
    
    // Try relative to parent directory (for textures/ subdirectory case)
    std::string parentDir = glbDir.substr(0, glbDir.find_last_of("/\\"));
    if (!parentDir.empty() && parentDir != glbDir) {
        texturePath = parentDir + "/" + textureUri;
        std::ifstream testFile2(texturePath);
        if (testFile2.good()) {
            testFile2.close();
            return texturePath;
        }
    }
    
    // Try relative to current working directory
    texturePath = textureUri;
    std::ifstream testFile3(texturePath);
    if (testFile3.good()) {
        testFile3.close();
        return texturePath;
    }
    
    // Return original path (will fail to load, but at least we tried)
    return glbDir + "/" + textureUri;
}

// Process textures from GLB file
static bool processTextures(const tinygltf::Model& gltfModel, GLBModel& model, const std::string& glbFilePath) {
    model.textures.clear();
    model.textures.reserve(gltfModel.textures.size());
    
    for (size_t i = 0; i < gltfModel.textures.size(); ++i) {
        const auto& gltfTexture = gltfModel.textures[i];
        GLBTexture texture;
        texture.loaded = false;
        
        if (gltfTexture.source < 0 || gltfTexture.source >= gltfModel.images.size()) {
            std::cerr << "  Texture " << i << " has invalid image source" << std::endl;
            model.textures.push_back(texture);
            continue;
        }
        
        const auto& image = gltfModel.images[gltfTexture.source];
        texture.path = image.uri.empty() ? "embedded" : image.uri;
        
        // Get image data
        unsigned char* imageData = nullptr;
        unsigned char* externalImageData = nullptr;  // Track external data for cleanup
        int width = 0, height = 0, channels = 0;
        bool isExternalTexture = false;
        
        if (!image.image.empty()) {
            // Image data is already loaded by tinygltf
            width = image.width;
            height = image.height;
            channels = image.component;
            imageData = const_cast<unsigned char*>(image.image.data());
            
            std::cout << "  Texture " << i << " image data: " << width << "x" << height 
                      << ", component=" << channels << ", image.size()=" << image.image.size() << std::endl;
        } else if (!image.uri.empty()) {
            // External image file - try to load it
            std::string texturePath = resolveTexturePath(glbFilePath, image.uri);
            std::cout << "  Texture " << i << " external file: " << image.uri << " -> " << texturePath << std::endl;
            
            // Use stb_image to load external texture
            externalImageData = stbi_load(texturePath.c_str(), &width, &height, &channels, 0);
            
            if (externalImageData) {
                imageData = externalImageData;
                isExternalTexture = true;
                std::cout << "  Successfully loaded external texture: " << texturePath 
                          << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
            } else {
                std::cerr << "  Failed to load external texture: " << texturePath 
                          << " (stb_image error: " << stbi_failure_reason() << ")" << std::endl;
                model.textures.push_back(texture);
                continue;
            }
        } else {
            std::cerr << "  Texture " << i << " has no image data (image.image.empty()=" 
                      << image.image.empty() << ", uri.empty()=" << image.uri.empty() << ")" << std::endl;
            model.textures.push_back(texture);
            continue;
        }
        
        if (!imageData || width <= 0 || height <= 0) {
            std::cerr << "  Texture " << i << " has invalid image data: width=" << width 
                      << ", height=" << height << ", imageData=" << (void*)imageData << std::endl;
            if (isExternalTexture && externalImageData) {
                stbi_image_free(externalImageData);
            }
            model.textures.push_back(texture);
            continue;
        }
        
        // Validate image size matches expected size (only for embedded textures)
        if (!isExternalTexture) {
            size_t expectedSize = width * height * channels;
            if (image.image.size() < expectedSize) {
                std::cerr << "  Texture " << i << " image size mismatch: expected " << expectedSize 
                          << " bytes, got " << image.image.size() << " bytes" << std::endl;
                model.textures.push_back(texture);
                continue;
            }
        }
        
        // Create OpenGL texture
        GLuint textureId = 0;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        
        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Determine format and internal format
        // Use sized internal formats for better compatibility
        GLenum format = GL_RGB;
        GLenum internalFormat = GL_RGB8;
        if (channels == 1) {
            format = GL_RED;
            internalFormat = GL_R8;
        } else if (channels == 2) {
            format = GL_RG;
            internalFormat = GL_RG8;
        } else if (channels == 3) {
            format = GL_RGB;
            internalFormat = GL_RGB8;
        } else if (channels == 4) {
            format = GL_RGBA;
            internalFormat = GL_RGBA8;
        }
        
        // Upload texture data
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, imageData);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        glBindTexture(GL_TEXTURE_2D, 0);
        
        texture.textureId = textureId;
        texture.width = width;
        texture.height = height;
        texture.channels = channels;
        texture.loaded = true;
        
        // Free external texture data after uploading to OpenGL
        if (isExternalTexture && externalImageData) {
            stbi_image_free(externalImageData);
        }
        
        model.textures.push_back(texture);
        std::cout << "  Loaded texture " << i << ": " << width << "x" << height << " (" << channels << " channels), textureId=" << textureId << std::endl;
    }
    
    return true;
}

// Convert PBR material parameters to Phong material parameters
static void convertPBRToPhong(const GLBMaterial& pbrMaterial, GLBMaterial& phongMaterial) {
    // Base color becomes diffuse
    phongMaterial.diffuse = glm::vec3(pbrMaterial.baseColorFactor);
    
    // Ambient is typically a fraction of diffuse
    // Note: Emissive is handled separately in the shader, not added to ambient
    phongMaterial.ambient = glm::vec3(pbrMaterial.baseColorFactor) * 0.1f;
    
    // Specular and shininess from metallic and roughness
    // Metallic materials have higher specular
    // Rough materials have lower shininess
    float metallic = pbrMaterial.metallicFactor;
    float roughness = pbrMaterial.roughnessFactor;
    
    // Convert roughness to shininess (inverse relationship)
    // Roughness 0.0 = very smooth = high shininess
    // Roughness 1.0 = very rough = low shininess
    phongMaterial.shininess = (1.0f - roughness) * 128.0f + 1.0f;  // Range: 1 to 129
    
    // Specular color based on metallic factor
    // Metallic materials reflect more specular light
    float specularStrength = metallic * 0.8f + 0.2f;  // Range: 0.2 to 1.0
    phongMaterial.specular = glm::vec3(specularStrength);
    
    // Copy texture indices
    phongMaterial.baseColorTextureIndex = pbrMaterial.baseColorTextureIndex;
    phongMaterial.normalTextureIndex = pbrMaterial.normalTextureIndex;
    phongMaterial.emissiveTextureIndex = pbrMaterial.emissiveTextureIndex;
    phongMaterial.metallicRoughnessTextureIndex = pbrMaterial.metallicRoughnessTextureIndex;
    phongMaterial.hasBaseColorTexture = (pbrMaterial.baseColorTextureIndex >= 0);
}

// Process materials from GLB file
static bool processMaterials(const tinygltf::Model& gltfModel, GLBModel& model) {
    model.materials.clear();
    model.materials.reserve(gltfModel.materials.size());
    
    for (size_t i = 0; i < gltfModel.materials.size(); ++i) {
        const auto& gltfMaterial = gltfModel.materials[i];
        GLBMaterial material;
        material.name = gltfMaterial.name;
        
        // Get PBR metallic-roughness parameters
        if (gltfMaterial.pbrMetallicRoughness.baseColorFactor.size() >= 4) {
            material.baseColorFactor = glm::vec4(
                static_cast<float>(gltfMaterial.pbrMetallicRoughness.baseColorFactor[0]),
                static_cast<float>(gltfMaterial.pbrMetallicRoughness.baseColorFactor[1]),
                static_cast<float>(gltfMaterial.pbrMetallicRoughness.baseColorFactor[2]),
                static_cast<float>(gltfMaterial.pbrMetallicRoughness.baseColorFactor[3])
            );
        }
        
        material.metallicFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor);
        material.roughnessFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor);
        
        // Get emissive factor
        if (gltfMaterial.emissiveFactor.size() >= 3) {
            material.emissiveFactor = glm::vec3(
                static_cast<float>(gltfMaterial.emissiveFactor[0]),
                static_cast<float>(gltfMaterial.emissiveFactor[1]),
                static_cast<float>(gltfMaterial.emissiveFactor[2])
            );
        }
        
        // Debug: Print all texture references in the material
        std::cout << "    Material " << i << " texture references:" << std::endl;
        std::cout << "      baseColorTexture.index=" << gltfMaterial.pbrMetallicRoughness.baseColorTexture.index << std::endl;
        std::cout << "      normalTexture.index=" << gltfMaterial.normalTexture.index << std::endl;
        std::cout << "      emissiveTexture.index=" << gltfMaterial.emissiveTexture.index << std::endl;
        std::cout << "      metallicRoughnessTexture.index=" << gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index << std::endl;
        
        // Get texture indices
        // Note: gltfMaterial.pbrMetallicRoughness.baseColorTexture.index is the texture index in glTF textures array
        // This index corresponds directly to our textures array (we process textures in the same order)
        if (gltfMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0) {
            int gltfTextureIndex = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
            // The texture index in glTF corresponds to the index in our textures array
            // (we process textures in the same order as glTF)
            if (gltfTextureIndex >= 0 && gltfTextureIndex < static_cast<int>(model.textures.size())) {
                material.baseColorTextureIndex = gltfTextureIndex;
                material.hasBaseColorTexture = model.textures[gltfTextureIndex].loaded;
                std::cout << "    Material " << i << " has baseColorTexture: gltfIndex=" << gltfTextureIndex 
                          << ", ourIndex=" << material.baseColorTextureIndex
                          << ", loaded=" << material.hasBaseColorTexture 
                          << ", textureId=" << (material.hasBaseColorTexture ? model.textures[gltfTextureIndex].textureId : 0) << std::endl;
            } else {
                std::cerr << "    Material " << i << " has invalid texture index: " << gltfTextureIndex 
                          << " (textures.size()=" << model.textures.size() << ")" << std::endl;
            }
        } else {
            // Try to find a suitable texture as fallback
            // Since baseColorTexture is missing, try other textures
            // Strategy: Try textures in order, but skip normalTexture (it's a normal map, not a color map)
            // Priority: emissiveTexture > texture 0, 1, 2... (skip normalTexture)
            bool foundTexture = false;
            
            // Try all textures in order as fallback
            // Priority: Try texture 0, 1, 2... (skip normalTexture if possible, but use it if nothing else)
            if (!model.textures.empty()) {
                int normalTexIndex = gltfMaterial.normalTexture.index;
                int emissiveTexIndex = gltfMaterial.emissiveTexture.index;
                
                // First, try non-normal, non-emissive textures (these are most likely to be color textures)
                for (size_t texIdx = 0; texIdx < model.textures.size(); ++texIdx) {
                    // Skip normal texture if possible (it's not a color map)
                    if (normalTexIndex >= 0 && static_cast<int>(texIdx) == normalTexIndex) {
                        continue;
                    }
                    // Skip emissive texture if possible (try other textures first)
                    if (emissiveTexIndex >= 0 && static_cast<int>(texIdx) == emissiveTexIndex) {
                        continue;
                    }
                    
                    if (model.textures[texIdx].loaded) {
                        material.baseColorTextureIndex = static_cast<int>(texIdx);
                        material.hasBaseColorTexture = true;
                        std::cout << "    Material " << i << " using texture " << texIdx << " as baseColorTexture (fallback)" << std::endl;
                        foundTexture = true;
                        break;
                    }
                }
                
                // If no suitable texture found, try emissive texture
                if (!foundTexture && emissiveTexIndex >= 0) {
                    int gltfTextureIndex = emissiveTexIndex;
                    if (gltfTextureIndex >= 0 && gltfTextureIndex < static_cast<int>(model.textures.size()) &&
                        model.textures[gltfTextureIndex].loaded) {
                        material.baseColorTextureIndex = gltfTextureIndex;
                        material.hasBaseColorTexture = true;
                        std::cout << "    Material " << i << " using emissiveTexture as baseColorTexture: index=" << gltfTextureIndex << std::endl;
                        foundTexture = true;
                    }
                }
                
                // Last resort: try normal texture (better than nothing)
                if (!foundTexture && normalTexIndex >= 0) {
                    int gltfTextureIndex = normalTexIndex;
                    if (gltfTextureIndex >= 0 && gltfTextureIndex < static_cast<int>(model.textures.size()) &&
                        model.textures[gltfTextureIndex].loaded) {
                        material.baseColorTextureIndex = gltfTextureIndex;
                        material.hasBaseColorTexture = true;
                        std::cout << "    Material " << i << " using normalTexture as baseColorTexture (last resort): index=" << gltfTextureIndex << std::endl;
                        foundTexture = true;
                    }
                }
            }
            
            if (!foundTexture) {
                std::cout << "    Material " << i << " has no baseColorTexture (index=" 
                          << gltfMaterial.pbrMetallicRoughness.baseColorTexture.index << ")" << std::endl;
            }
        }
        
        if (gltfMaterial.normalTexture.index >= 0) {
            int texIndex = gltfMaterial.normalTexture.index;
            if (texIndex >= 0 && texIndex < static_cast<int>(model.textures.size())) {
                material.normalTextureIndex = texIndex;
            }
        }
        
        if (gltfMaterial.emissiveTexture.index >= 0) {
            int texIndex = gltfMaterial.emissiveTexture.index;
            if (texIndex >= 0 && texIndex < static_cast<int>(model.textures.size())) {
                material.emissiveTextureIndex = texIndex;
            }
        }
        
        // Convert PBR to Phong
        convertPBRToPhong(material, material);
        
        model.materials.push_back(material);
        std::cout << "  Material " << i << ": " << (material.name.empty() ? "(unnamed)" : material.name);
        std::cout << ", baseColorTextureIndex=" << material.baseColorTextureIndex;
        std::cout << ", hasBaseColorTexture=" << (material.hasBaseColorTexture ? "true" : "false");
        if (material.hasBaseColorTexture && material.baseColorTextureIndex >= 0 && 
            material.baseColorTextureIndex < static_cast<int>(model.textures.size())) {
            std::cout << ", textureId=" << model.textures[material.baseColorTextureIndex].textureId;
        }
        std::cout << std::endl;
    }
    
    return true;
}

void GLBLoader::cleanup(GLBModel& model) {
    // Clean up textures
    for (auto& texture : model.textures) {
        if (texture.textureId != 0) {
            glDeleteTextures(1, &texture.textureId);
            texture.textureId = 0;
        }
    }
    model.textures.clear();
    
    // Clean up all OpenGL resources
    for (auto& mesh : model.meshes) {
        if (mesh.ebo != 0) {
            glDeleteBuffers(1, &mesh.ebo);
            mesh.ebo = 0;
        }
        if (mesh.jointsVbo != 0) {
            glDeleteBuffers(1, &mesh.jointsVbo);
            mesh.jointsVbo = 0;
        }
        if (mesh.vbo != 0) {
            glDeleteBuffers(1, &mesh.vbo);
            mesh.vbo = 0;
        }
        if (mesh.vao != 0) {
            glDeleteVertexArrays(1, &mesh.vao);
            mesh.vao = 0;
        }
    }
    model.meshes.clear();
    model.loaded = false;
}

void GLBLoader::printModelInfo(const GLBModel& model) {
    std::cout << "GLB Model Info:" << std::endl;
    std::cout << "  Filepath: " << model.filepath << std::endl;
    std::cout << "  Loaded: " << (model.loaded ? "Yes" : "No") << std::endl;
    std::cout << "  Mesh count: " << model.meshes.size() << std::endl;
    
    for (size_t i = 0; i < model.meshes.size(); ++i) {
        const auto& mesh = model.meshes[i];
        std::cout << "  Mesh " << i << ":" << std::endl;
        std::cout << "    VAO: " << mesh.vao << std::endl;
        std::cout << "    Index count: " << mesh.indexCount << std::endl;
        std::cout << "    Material index: " << mesh.materialIndex << std::endl;
    }
}

