#pragma once

#include <glm/glm.hpp> 

/**
 * Hand-written matrix helpers that replace the forbidden GLM convenience
 * functions such as glm::perspective / glm::lookAt.
 */ 
glm::mat4 generatePerspectiveMatrix(float fovy_rad, float aspect, float nearPlane, float farPlane); 
glm::mat4 generateViewMatrix(const glm::vec3& pos, const glm::vec3& look, const glm::vec3& up); 
glm::mat4 generateTranslateMatrix(const glm::vec3& t); 
glm::mat4 generateScaleMatrix(const glm::vec3& s); 
glm::mat4 generateRotateMatrix(float angle_rad, const glm::vec3& axis); 
