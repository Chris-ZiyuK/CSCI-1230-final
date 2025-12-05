#pragma once

#include <vector> 
#include <glm/glm.hpp> 

/**
 * Utility class responsible for generating tessellated geometry for the
 * realtime project. Each function appends interleaved position/normal data
 * to the provided vertex buffer in the order [x, y, z, nx, ny, nz].
 */ 
class Tessellator { 
public: 
    static void generateCube(std::vector<float> &vertices, int param1); 
    static void generateCone(std::vector<float> &vertices, int param1, int param2); 
    static void generateSphere(std::vector<float> &vertices, int param1, int param2); 
    static void generateCylinder(std::vector<float> &vertices, int param1, int param2); 

private: 
    static void insertVec3(std::vector<float> &data, const glm::vec3 &v); 
}; 

