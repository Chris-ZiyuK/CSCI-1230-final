#include "Star.h"

#include <algorithm>

void Star::updateParams(int param1, int param2)
{
    m_vertexData.clear();
    m_param1 = std::max(param1, 1);
    m_param2 = std::max(param2, 1);
    setVertexData();
}

void Star::setVertexData()
{
    const float halfHeight = 0.5f * m_height;
    const float r = m_radius;

    glm::vec3 top(0.f, halfHeight, 0.f);
    glm::vec3 bottom(0.f, -halfHeight, 0.f);

    std::vector<glm::vec3> rim = {
        {0.f, 0.f, r},
        {r, 0.f, 0.f},
        {0.f, 0.f, -r},
        {-r, 0.f, 0.f}
    };

    for (int i = 0; i < static_cast<int>(rim.size()); ++i) {
        glm::vec3 curr = rim[i];
        glm::vec3 next = rim[(i + 1) % rim.size()];

        tessellateFace(top, curr, next);
        tessellateFace(bottom, next, curr);
    }
}

static glm::vec3 lerp(const glm::vec3 &a, const glm::vec3 &b, float t)
{
    return (1.f - t) * a + t * b;
}

void Star::tessellateFace(const glm::vec3 &apex,
                          const glm::vec3 &edgeA,
                          const glm::vec3 &edgeB)
{
    int slices = std::max(1, m_param1);
    glm::vec3 tip = apex;
    glm::vec3 edgeStep = (edgeB - edgeA) / static_cast<float>(slices);

    for (int i = 0; i < slices; ++i) {
        glm::vec3 a0 = edgeA + edgeStep * static_cast<float>(i);
        glm::vec3 a1 = edgeA + edgeStep * static_cast<float>(i + 1);
        addTriangle(tip, a0, a1);
    }
}

void Star::addTriangle(const glm::vec3 &a,
                       const glm::vec3 &b,
                       const glm::vec3 &c)
{
    glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));

    insertVec3(m_vertexData, a);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, b);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, c);
    insertVec3(m_vertexData, normal);
}

void Star::insertVec3(std::vector<float> &data, const glm::vec3 &v)
{
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}


