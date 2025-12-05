#include "Cylinder.h"

void Cylinder::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    m_param2 = param2;
    setVertexData();
}


void Cylinder::setVertexData() {
    m_vertexData.clear();

    int p1 = std::max(1, m_param1);
    int p2 = std::max(3, m_param2);

    float radius = 0.5f;
    float height = 1.0f;
    float y_top = 0.5f;
    float y_bottom = -0.5f;

    float dTheta = 2 * M_PI / p2;
    float dy = height / p1;

    auto pushVertex = [&](glm::vec3 pos, glm::vec3 normal) {
        insertVec3(m_vertexData, pos);
        insertVec3(m_vertexData, normal);
    };

    // Side Surface
    for (int i = 0; i < p2; i++) {
        float theta0 = i * dTheta;
        float theta1 = (i + 1) * dTheta;

        glm::vec3 n0 = glm::normalize(glm::vec3(cos(theta0), 0, sin(theta0)));
        glm::vec3 n1 = glm::normalize(glm::vec3(cos(theta1), 0, sin(theta1)));

        float x0 = radius * cos(theta0);
        float z0 = radius * sin(theta0);
        float x1 = radius * cos(theta1);
        float z1 = radius * sin(theta1);

        for (int j = 0; j < p1; j++) {
            float y0 = y_bottom + j * dy;
            float y1 = y_bottom + (j + 1) * dy;

            glm::vec3 p00(x0, y0, z0);
            glm::vec3 p01(x0, y1, z0);
            glm::vec3 p10(x1, y0, z1);
            glm::vec3 p11(x1, y1, z1);

            // Triangle 1
            pushVertex(p00, n0);
            pushVertex(p01, n0);
            pushVertex(p11, n1);

            // Triangle 2
            pushVertex(p00, n0);
            pushVertex(p11, n1);
            pushVertex(p10, n1);
        }
    }

    // Top Cap
    glm::vec3 topNormal(0, 1, 0);
    glm::vec3 topCenter(0, y_top, 0);

    for (int i = 0; i < p2; i++) {
        float theta0 = i * dTheta;
        float theta1 = (i + 1) * dTheta;

        glm::vec3 p0(radius * cos(theta0), y_top, radius * sin(theta0));
        glm::vec3 p1(radius * cos(theta1), y_top, radius * sin(theta1));

        pushVertex(topCenter, topNormal);
        pushVertex(p1, topNormal);
        pushVertex(p0, topNormal);
    }

    // Bottom Cap
    glm::vec3 bottomNormal(0, -1, 0);
    glm::vec3 bottomCenter(0, y_bottom, 0);

    for (int i = 0; i < p2; i++) {
        float theta0 = i * dTheta;
        float theta1 = (i + 1) * dTheta;

        glm::vec3 p0(radius * cos(theta0), y_bottom, radius * sin(theta0));
        glm::vec3 p1(radius * cos(theta1), y_bottom, radius * sin(theta1));

        pushVertex(bottomCenter, bottomNormal);
        pushVertex(p0, bottomNormal);
        pushVertex(p1, bottomNormal);
    }
}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Cylinder::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
