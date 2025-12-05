#include "Cone.h"

void Cone::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    m_param2 = param2;
    setVertexData();
}

// Task 8: create function(s) to make tiles which you can call later on
// Note: Consider your makeTile() functions from Sphere and Cube
void Cone::makeCapTile(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3) {
    glm::vec3 normal(0.f, -1.f, 0.f);
    insertVec3(m_vertexData, v1); insertVec3(m_vertexData, normal);
    insertVec3(m_vertexData, v2); insertVec3(m_vertexData, normal);
    insertVec3(m_vertexData, v3); insertVec3(m_vertexData, normal);
}

void Cone::makeCapSlice(float currentTheta, float nextTheta){
    // Task 8: create a slice of the cap face using your
    //         make tile function(s)
    // Note: think about how param 1 comes into play here!
    float y = -0.5f;
    float r = 0.5f;

    glm::vec3 center(0.f, y, 0.f);

    for (int i = 0; i < m_param1; i++) {
        float t1 = float(i) / m_param1;
        float t2 = float(i + 1) / m_param1;

        float r1 = r * t1;
        float r2 = r * t2;

        glm::vec3 v1(r1 * cos(currentTheta), y, r1 * sin(currentTheta));
        glm::vec3 v2(r1 * cos(nextTheta),    y, r1 * sin(nextTheta));
        glm::vec3 v3(r2 * cos(currentTheta), y, r2 * sin(currentTheta));
        glm::vec3 v4(r2 * cos(nextTheta),    y, r2 * sin(nextTheta));

        // CCW order viewed from outside
        makeCapTile(v1, v3, v4);
        makeCapTile(v1, v4, v2);
    }
}

glm::vec3 Cone::calcNorm(glm::vec3 &pt) {
    float y01 = pt.y + 0.5f;
    float xNorm = (2 * pt.x);
    float yNorm = -(1.f / 4.f) * (2.f * pt.y - 1.f);
    float zNorm = (2 * pt.z);

    return glm::normalize(glm::vec3{xNorm, yNorm, zNorm});
}

void Cone::makeSlopeTile(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3) {
    glm::vec3 n1 = calcNorm(v1);
    glm::vec3 n2 = calcNorm(v2);
    glm::vec3 n3 = calcNorm(v3);

    insertVec3(m_vertexData, v1); insertVec3(m_vertexData, n1);
    insertVec3(m_vertexData, v2); insertVec3(m_vertexData, n2);
    insertVec3(m_vertexData, v3); insertVec3(m_vertexData, n3);
}

void Cone::makeSlopeSlice(float currentTheta, float nextTheta){
    // Task 9: create a single sloped face using your make
    //         tile function(s)
    // Note: think about how param 1 comes into play here!
    float r = 0.5f;
    glm::vec3 tip(0.f, 0.5f, 0.f);

    for (int i = 0; i < m_param1; i++) {
        float t1 = (float)i / m_param1;
        float t2 = (float)(i + 1) / m_param1;

        float r1 = r * t1;
        float r2 = r * t2;

        float y1 = 0.5f - t1;
        float y2 = 0.5f - t2;

        glm::vec3 v1(r1 * cos(currentTheta), y1, r1 * sin(currentTheta));
        glm::vec3 v2(r1 * cos(nextTheta),    y1, r1 * sin(nextTheta));
        glm::vec3 v3(r2 * cos(currentTheta), y2, r2 * sin(currentTheta));
        glm::vec3 v4(r2 * cos(nextTheta),    y2, r2 * sin(nextTheta));

        makeSlopeTile(v1, v3, v4);
        makeSlopeTile(v1, v4, v2);

        if (i == m_param1 - 1) {
            glm::vec3 n3 = calcNorm(v3);
            glm::vec3 n4 = calcNorm(v4);
            glm::vec3 tipNormal = glm::normalize(n3 + n4);

            insertVec3(m_vertexData, v3);      insertVec3(m_vertexData, n3);
            insertVec3(m_vertexData, tip);     insertVec3(m_vertexData, tipNormal);
            insertVec3(m_vertexData, v4);      insertVec3(m_vertexData, n4);
        }
    }
}

void Cone::makeWedge(float currentTheta, float nextTheta) {
    // Task 10: create a single wedge of the Cone using the
    //          makeCapSlice() and makeSlopeSlice() functions you
    //          implemented in Task 5
    makeCapSlice(currentTheta, nextTheta);
    makeSlopeSlice(currentTheta, nextTheta);
}

void Cone::setVertexData() {
    // Task 10: create a full cone using the makeWedge() function you
    //          just implemented
    // Note: think about how param 2 comes into play here!
    m_vertexData.clear();

    float thetaStep = glm::radians(360.f / m_param2);

    for (int i = 0; i < m_param2; i++) {
        float currentTheta = i * thetaStep;
        float nextTheta = (i + 1) * thetaStep;
        makeWedge(currentTheta, nextTheta);
    }
}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Cone::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
