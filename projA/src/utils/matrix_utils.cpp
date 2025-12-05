#include "matrix_utils.h" 

#include <algorithm> 
#include <cmath> 
#include <limits> 
#include <glm/gtx/quaternion.hpp> 

glm::mat4 generatePerspectiveMatrix(float fovy_rad, float aspect, float nearPlane, float farPlane) { 
    // Clamp against degenerate input to avoid division by zero. 
    const float epsilon = std::numeric_limits<float>::epsilon(); 
    float clampedAspect = std::max(aspect, epsilon); 
    float clampedNear = std::max(nearPlane, epsilon); 
    float minSeparation = std::max(0.1f * clampedNear, 0.1f); 
    float clampedFar = std::max(farPlane, clampedNear + minSeparation); 

    float f = 1.0f / std::tan(fovy_rad * 0.5f); // cotangent of half the vertical FOV 
    glm::mat4 result(0.0f); 

    // Col-major assignment: result[col][row] 
    result[0][0] = f / clampedAspect; // Scale X by cot(fov/2) and aspect ratio 
    result[1][1] = f;                 // Scale Y by cot(fov/2) 

    // Depth range mapping from [near, far] to clip space [-1, 1] 
    result[2][2] = (clampedFar + clampedNear) / (clampedNear - clampedFar); 
    result[2][3] = -1.0f; 
    result[3][2] = (2.0f * clampedFar * clampedNear) / (clampedNear - clampedFar); 

    return result; 
} 

glm::mat4 generateViewMatrix(const glm::vec3& pos, const glm::vec3& look, const glm::vec3& up) { 
    glm::vec3 w = glm::normalize(-look);           // camera backward 
    glm::vec3 u = glm::normalize(glm::cross(up, w)); // camera right 
    glm::vec3 v = glm::cross(w, u);                // camera true up 

    glm::mat4 rotation(1.0f); 
    rotation[0][0] = u.x; rotation[1][0] = u.y; rotation[2][0] = u.z; 
    rotation[0][1] = v.x; rotation[1][1] = v.y; rotation[2][1] = v.z; 
    rotation[0][2] = w.x; rotation[1][2] = w.y; rotation[2][2] = w.z; 

    glm::mat4 translation(1.0f); 
    translation[3][0] = -pos.x; 
    translation[3][1] = -pos.y; 
    translation[3][2] = -pos.z; 

    return rotation * translation; 
} 

glm::mat4 generateTranslateMatrix(const glm::vec3& t) { 
    glm::mat4 result(1.0f); 
    result[3][0] = t.x; 
    result[3][1] = t.y; 
    result[3][2] = t.z; 
    return result; 
} 

glm::mat4 generateScaleMatrix(const glm::vec3& s) { 
    glm::mat4 result(1.0f); 
    result[0][0] = s.x; 
    result[1][1] = s.y; 
    result[2][2] = s.z; 
    return result; 
} 

glm::mat4 generateRotateMatrix(float angle_rad, const glm::vec3& axis) { 
    glm::vec3 normalizedAxis = glm::normalize(axis); 
    if (glm::length(normalizedAxis) < std::numeric_limits<float>::epsilon()) { 
        return glm::mat4(1.0f); 
    } 
    glm::quat rotationQuat = glm::angleAxis(angle_rad, normalizedAxis); 
    return glm::mat4_cast(rotationQuat); 
} 
