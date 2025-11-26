#include "Sphere.h"

void Sphere::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    m_param2 = param2;
    setVertexData();
}

void Sphere::makeTile(glm::vec3 topLeft,
                      glm::vec3 topRight,
                      glm::vec3 bottomLeft,
                      glm::vec3 bottomRight) {
    // Task 5: Implement the makeTile() function for a Sphere
    // Note: this function is very similar to the makeTile() function for Cube,
    //       but the normals are calculated in a different way!
    // Triangle 1 - left
    glm::vec3 n1 = glm::normalize(topLeft);
    glm::vec3 n2 = glm::normalize(bottomLeft);
    glm::vec3 n3 = glm::normalize(bottomRight);

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, n1);

    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, n2);

    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, n3);


    // Triangle 2 - right
    glm::vec3 n4 = glm::normalize(topRight);

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, n1);

    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, n3);

    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, n4);
}

glm::vec3 sphericalToCartesian(float r, float phi, float theta) {
    return glm::vec3(
        r * sin(phi) * cos(theta),
        r * cos(phi),
        -r * sin(phi) * sin(theta)
        );
}

void Sphere::makeWedge(float currentTheta, float nextTheta) {
    // Task 6: create a single wedge of the sphere using the
    //         makeTile() function you implemented in Task 5
    // Note: think about how param 1 comes into play here!
    float r = 0.5f;

    for (int i = 0; i < m_param1; i++) {
        float phi1 = ((float)i / m_param1) * M_PI;
        float phi2 = ((float)(i + 1) / m_param1) * M_PI;
        glm::vec3 topLeft     = sphericalToCartesian(r, phi1, currentTheta);
        glm::vec3 topRight    = sphericalToCartesian(r, phi1, nextTheta);
        glm::vec3 bottomLeft  = sphericalToCartesian(r, phi2, currentTheta);
        glm::vec3 bottomRight = sphericalToCartesian(r, phi2, nextTheta);

        makeTile(topLeft, topRight, bottomLeft, bottomRight);
    }
}

void Sphere::makeSphere() {
    // Task 7: create a full sphere using the makeWedge() function you
    //         implemented in Task 6
    // Note: think about how param 2 comes into play here!
    float thetaStep = glm::radians(360.f / m_param2);

    for (int i = 0; i < m_param2; i++) {
        float currentTheta = i * thetaStep;
        float nextTheta = (i + 1) * thetaStep;
        makeWedge(currentTheta, nextTheta);
    }
}

void Sphere::setVertexData() {
    // Uncomment these lines to make a wedge for Task 6, then comment them out for Task 7:

    // float thetaStep = glm::radians(360.f / m_param2);
    // float currentTheta = 0 * thetaStep;
    // float nextTheta = 1 * thetaStep;
    // makeWedge(currentTheta, nextTheta);

    // Uncomment these lines to make sphere for Task 7:

    makeSphere();
}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Sphere::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
