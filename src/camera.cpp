#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

void Camera::setCameraData(glm::vec4 pos, glm::vec4 look, glm::vec4 up, float heightAngle) {
    m_pos = glm::vec3(pos);
    m_look = glm::normalize(glm::vec3(look));
    m_up   = glm::normalize(glm::vec3(up));
    m_heightAngle = heightAngle;
}

void Camera::setAspectRatio(float aspect) {
    m_aspect = aspect;
}

void Camera::setNearFar(float nearPlane, float farPlane) {
    m_near = nearPlane;
    m_far  = farPlane;
}

glm::mat4 Camera::getViewMatrix() const {
    glm::vec3 w = glm::normalize(-m_look);
    glm::vec3 v = glm::normalize(m_up - glm::dot(m_up, w) * w);
    glm::vec3 u = glm::cross(v, w);

    glm::mat4 rotate = glm::mat4(
        glm::vec4(u.x, v.x, w.x, 0),
        glm::vec4(u.y, v.y, w.y, 0),
        glm::vec4(u.z, v.z, w.z, 0),
        glm::vec4(0,   0,   0,   1)
        );

    glm::mat4 translate = glm::mat4(
        glm::vec4(1, 0, 0, 0),
        glm::vec4(0, 1, 0, 0),
        glm::vec4(0, 0, 1, 0),
        glm::vec4(-m_pos.x, -m_pos.y, -m_pos.z, 1)
        );

    return rotate * translate;
}

glm::mat4 Camera::getProjMatrix() const {
    return glm::perspective(m_heightAngle, m_aspect, m_near, m_far);
}

glm::mat4 Camera::getViewProjMatrix() const {
    return getProjMatrix() * getViewMatrix();
}

glm::vec3 Camera::getPosition() const {
    return m_pos;
}

glm::vec3 Camera::getForward() const {
    return glm::normalize(m_look);
}

glm::vec3 Camera::getUpVector() const {
    return glm::normalize(m_up);
}

glm::vec3 Camera::getRightVector() const {
    return glm::normalize(glm::cross(m_look, m_up));
}


//============== Not needed ================

// void Camera::translate(const glm::vec3 &delta) {
//     m_pos += delta;
// }

// static glm::vec3 rotateVectorAroundAxis(const glm::vec3 &v, const glm::vec3 &axis, float radians) {
//     glm::vec3 normAxis = glm::normalize(axis);
//     float cosTheta = cos(radians);
//     float sinTheta = sin(radians);
//     return v * cosTheta + glm::cross(normAxis, v) * sinTheta + normAxis * glm::dot(normAxis, v) * (1.f - cosTheta);
// }

// void Camera::rotateAroundWorldUp(float radians) {
//     glm::vec3 axis(0.f, 1.f, 0.f);
//     m_look = rotateVectorAroundAxis(m_look, axis, radians);
//     m_up   = rotateVectorAroundAxis(m_up, axis, radians);
//     m_look = glm::normalize(m_look);
//     m_up   = glm::normalize(m_up);
// }

// void Camera::rotateAroundRight(float radians) {
//     glm::vec3 right = getRightVector();
//     m_look = rotateVectorAroundAxis(m_look, right, radians);
//     m_up   = rotateVectorAroundAxis(m_up, right, radians);
//     m_look = glm::normalize(m_look);
//     m_up   = glm::normalize(m_up);
// }
