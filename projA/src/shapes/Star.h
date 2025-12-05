#pragma once

#include <vector>
#include <glm/glm.hpp>

class Star
{
public:
    void updateParams(int param1, int param2);
    std::vector<float> generateShape() const { return m_vertexData; }

private:
    void insertVec3(std::vector<float> &data, const glm::vec3 &v);
    void addTriangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c);
    void tessellateFace(const glm::vec3 &apex,
                        const glm::vec3 &edgeA,
                        const glm::vec3 &edgeB);
    void setVertexData();

    std::vector<float> m_vertexData;
    int m_param1 = 1;
    int m_param2 = 1;
    float m_height = 1.f;
    float m_radius = 0.35f;
};


