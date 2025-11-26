#pragma once

#include <vector>
#include <glm/glm.hpp>

class Cone
{
public:
    void updateParams(int param1, int param2);
    std::vector<float> generateShape() { return m_vertexData; }

private:
    void insertVec3(std::vector<float> &data, glm::vec3 v);
    void setVertexData();
    void makeCapSlice(float currentTheta, float nextTheta);
    void makeSlopeSlice(float currentTheta, float nextTheta);
    void makeWedge(float currentTheta, float nextTheta);
    void makeCapTile(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3);
    void makeSlopeTile(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3);
    glm::vec3 calcNorm(glm::vec3 &pt);

    std::vector<float> m_vertexData;
    int m_param1;
    int m_param2;
    float m_radius = 0.5;
};
