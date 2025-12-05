#include "tessellator.h" 

#include <algorithm> 
#include <cmath> 
#include <glm/gtc/constants.hpp> 

namespace { 
    constexpr float HALF_EXTENT = 0.5f; 
    constexpr float CYLINDER_RADIUS = 0.5f; 
    constexpr float CYLINDER_HALF_HEIGHT = 0.5f; 
    constexpr float CONE_RADIUS = 0.5f; 
    constexpr float CONE_HALF_HEIGHT = 0.5f; 
    constexpr float SPHERE_RADIUS = 0.5f; 

    inline glm::vec3 lerpVec3(const glm::vec3 &a, const glm::vec3 &b, float t) { 
        return a + t * (b - a); 
    } 

    inline glm::vec3 coneSurfaceNormal(const glm::vec3 &pt) { 
        float xNorm = 2.0f * pt.x; 
        float yNorm = -(0.25f) * (2.0f * pt.y - 1.0f); 
        float zNorm = 2.0f * pt.z; 
        return glm::normalize(glm::vec3{xNorm, yNorm, zNorm}); 
    } 
} 

void Tessellator::generateCube(std::vector<float> &vertices, int param1) { 
    vertices.clear(); 

    int divisions = std::max(1, param1); 

    auto addTriangle = [&](const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3) { 
        glm::vec3 u = p2 - p1; 
        glm::vec3 v = p3 - p1; 
        glm::vec3 normal = glm::normalize(glm::cross(u, v)); 

        insertVec3(vertices, p1); 
        insertVec3(vertices, normal); 
        insertVec3(vertices, p2); 
        insertVec3(vertices, normal); 
        insertVec3(vertices, p3); 
        insertVec3(vertices, normal); 
    }; 

    auto addFace = [&](glm::vec3 tl, glm::vec3 tr, glm::vec3 bl, glm::vec3 br) { 
        for (int i = 0; i < divisions; ++i) { 
            float t0 = static_cast<float>(i) / divisions; 
            float t1 = static_cast<float>(i + 1) / divisions; 

            glm::vec3 rowTL = lerpVec3(tl, bl, t0); 
            glm::vec3 rowTR = lerpVec3(tr, br, t0); 
            glm::vec3 rowBL = lerpVec3(tl, bl, t1); 
            glm::vec3 rowBR = lerpVec3(tr, br, t1); 

            for (int j = 0; j < divisions; ++j) { 
                float s0 = static_cast<float>(j) / divisions; 
                float s1 = static_cast<float>(j + 1) / divisions; 

                glm::vec3 quadTL = lerpVec3(rowTL, rowTR, s0); 
                glm::vec3 quadTR = lerpVec3(rowTL, rowTR, s1); 
                glm::vec3 quadBL = lerpVec3(rowBL, rowBR, s0); 
                glm::vec3 quadBR = lerpVec3(rowBL, rowBR, s1); 

                addTriangle(quadTL, quadBL, quadBR); 
                addTriangle(quadTL, quadBR, quadTR); 
            } 
        } 
    }; 

    // Front (+Z) 
    addFace(glm::vec3(-HALF_EXTENT,  HALF_EXTENT,  HALF_EXTENT), 
            glm::vec3( HALF_EXTENT,  HALF_EXTENT,  HALF_EXTENT), 
            glm::vec3(-HALF_EXTENT, -HALF_EXTENT,  HALF_EXTENT), 
            glm::vec3( HALF_EXTENT, -HALF_EXTENT,  HALF_EXTENT)); 

    // Back (-Z) 
    addFace(glm::vec3( HALF_EXTENT,  HALF_EXTENT, -HALF_EXTENT), 
            glm::vec3(-HALF_EXTENT,  HALF_EXTENT, -HALF_EXTENT), 
            glm::vec3( HALF_EXTENT, -HALF_EXTENT, -HALF_EXTENT), 
            glm::vec3(-HALF_EXTENT, -HALF_EXTENT, -HALF_EXTENT)); 

    // Left (-X) 
    addFace(glm::vec3(-HALF_EXTENT,  HALF_EXTENT, -HALF_EXTENT), 
            glm::vec3(-HALF_EXTENT,  HALF_EXTENT,  HALF_EXTENT), 
            glm::vec3(-HALF_EXTENT, -HALF_EXTENT, -HALF_EXTENT), 
            glm::vec3(-HALF_EXTENT, -HALF_EXTENT,  HALF_EXTENT)); 

    // Right (+X) 
    addFace(glm::vec3( HALF_EXTENT,  HALF_EXTENT,  HALF_EXTENT), 
            glm::vec3( HALF_EXTENT,  HALF_EXTENT, -HALF_EXTENT), 
            glm::vec3( HALF_EXTENT, -HALF_EXTENT,  HALF_EXTENT), 
            glm::vec3( HALF_EXTENT, -HALF_EXTENT, -HALF_EXTENT)); 

    // Top (+Y) 
    addFace(glm::vec3(-HALF_EXTENT,  HALF_EXTENT, -HALF_EXTENT), 
            glm::vec3( HALF_EXTENT,  HALF_EXTENT, -HALF_EXTENT), 
            glm::vec3(-HALF_EXTENT,  HALF_EXTENT,  HALF_EXTENT), 
            glm::vec3( HALF_EXTENT,  HALF_EXTENT,  HALF_EXTENT)); 

    // Bottom (-Y) 
    addFace(glm::vec3(-HALF_EXTENT, -HALF_EXTENT,  HALF_EXTENT), 
            glm::vec3( HALF_EXTENT, -HALF_EXTENT,  HALF_EXTENT), 
            glm::vec3(-HALF_EXTENT, -HALF_EXTENT, -HALF_EXTENT), 
            glm::vec3( HALF_EXTENT, -HALF_EXTENT, -HALF_EXTENT)); 
} 

void Tessellator::generateCone(std::vector<float> &vertices, int param1, int param2) { 
    vertices.clear(); 

    int verticalSegments = std::max(1, param1); 
    int radialSegments = std::max(3, param2); 

    float thetaStep = glm::two_pi<float>() / radialSegments; 

    auto addTriangle = [&](const glm::vec3 &p1, const glm::vec3 &n1, 
                           const glm::vec3 &p2, const glm::vec3 &n2, 
                           const glm::vec3 &p3, const glm::vec3 &n3) { 
        insertVec3(vertices, p1); 
        insertVec3(vertices, n1); 
        insertVec3(vertices, p2); 
        insertVec3(vertices, n2); 
        insertVec3(vertices, p3); 
        insertVec3(vertices, n3); 
    }; 

    glm::vec3 tip(0.0f, CONE_HALF_HEIGHT, 0.0f); 
    glm::vec3 baseCenter(0.0f, -CONE_HALF_HEIGHT, 0.0f); 

    auto basePoint = [&](float theta) -> glm::vec3 { 
        return glm::vec3( 
            CONE_RADIUS * std::cos(theta), 
            -CONE_HALF_HEIGHT, 
            CONE_RADIUS * std::sin(theta) 
        ); 
    }; 

    // Base cap 
    for (int i = 0; i < radialSegments; ++i) { 
        float thetaCurr = i * thetaStep; 
        float thetaNext = (i + 1) * thetaStep; 

        glm::vec3 curr = basePoint(thetaCurr); 
        glm::vec3 next = basePoint(thetaNext); 
        glm::vec3 normal(0.0f, -1.0f, 0.0f); 

        addTriangle(baseCenter, normal, curr, normal, next, normal); 
    } 

    // Slope surface 
    for (int i = 0; i < radialSegments; ++i) { 
        float thetaCurr = i * thetaStep; 
        float thetaNext = (i + 1) * thetaStep; 

        glm::vec3 baseCurr = basePoint(thetaCurr); 
        glm::vec3 baseNext = basePoint(thetaNext); 

        for (int j = 0; j < verticalSegments; ++j) { 
            float t0 = static_cast<float>(j) / verticalSegments; 
            float t1 = static_cast<float>(j + 1) / verticalSegments; 

            glm::vec3 upperLeft = lerpVec3(tip, baseCurr, t0); 
            glm::vec3 upperRight = lerpVec3(tip, baseNext, t0); 
            glm::vec3 lowerLeft = lerpVec3(tip, baseCurr, t1); 
            glm::vec3 lowerRight = lerpVec3(tip, baseNext, t1); 

            glm::vec3 normalUL, normalUR; 
            if (glm::length(upperLeft - tip) < 1e-5f) { 
                normalUL = glm::normalize(coneSurfaceNormal(lowerLeft) + coneSurfaceNormal(lowerRight)); 
                normalUR = normalUL; 
            } else { 
                normalUL = coneSurfaceNormal(upperLeft); 
                normalUR = coneSurfaceNormal(upperRight); 
            } 

            glm::vec3 normalLL = coneSurfaceNormal(lowerLeft); 
            glm::vec3 normalLR = coneSurfaceNormal(lowerRight); 

            addTriangle(upperLeft, normalUL, 
                        lowerRight, normalLR, 
                        lowerLeft, normalLL); 

            addTriangle(upperLeft, normalUL, 
                        upperRight, normalUR, 
                        lowerRight, normalLR); 
        } 
    } 
} 

void Tessellator::generateSphere(std::vector<float> &vertices, int param1, int param2) { 
    vertices.clear(); 

    int latSegments = std::max(2, param1); 
    int lonSegments = std::max(3, param2); 

    float phiStep = glm::pi<float>() / latSegments; 
    float thetaStep = glm::two_pi<float>() / lonSegments; 

    auto pointOnSphere = [&](float phi, float theta) -> glm::vec3 { 
        float x = SPHERE_RADIUS * std::sin(phi) * std::cos(theta); 
        float y = SPHERE_RADIUS * std::cos(phi); 
        float z = SPHERE_RADIUS * std::sin(phi) * std::sin(theta); 
        return glm::vec3(x, y, z); 
    }; 

    for (int i = 0; i < lonSegments; ++i) { 
        float thetaCurr = i * thetaStep; 
        float thetaNext = (i + 1) * thetaStep; 

        for (int j = 0; j < latSegments; ++j) { 
            float phiTop = j * phiStep; 
            float phiBottom = (j + 1) * phiStep; 

            glm::vec3 topLeft = pointOnSphere(phiTop, thetaCurr); 
            glm::vec3 topRight = pointOnSphere(phiTop, thetaNext); 
            glm::vec3 bottomLeft = pointOnSphere(phiBottom, thetaCurr); 
            glm::vec3 bottomRight = pointOnSphere(phiBottom, thetaNext); 

            glm::vec3 normalTL = glm::normalize(topLeft); 
            glm::vec3 normalTR = glm::normalize(topRight); 
            glm::vec3 normalBL = glm::normalize(bottomLeft); 
            glm::vec3 normalBR = glm::normalize(bottomRight); 

            insertVec3(vertices, topLeft); 
            insertVec3(vertices, normalTL); 
            insertVec3(vertices, bottomRight); 
            insertVec3(vertices, normalBR); 
            insertVec3(vertices, bottomLeft); 
            insertVec3(vertices, normalBL); 

            insertVec3(vertices, topLeft); 
            insertVec3(vertices, normalTL); 
            insertVec3(vertices, topRight); 
            insertVec3(vertices, normalTR); 
            insertVec3(vertices, bottomRight); 
            insertVec3(vertices, normalBR); 
        } 
    } 
} 

void Tessellator::generateCylinder(std::vector<float> &vertices, int param1, int param2) { 
    vertices.clear(); 

    int verticalSegments = std::max(1, param1); 
    int radialSegments = std::max(3, param2); 

    float thetaStep = glm::two_pi<float>() / radialSegments; 
    float heightStep = (2.0f * CYLINDER_HALF_HEIGHT) / verticalSegments; 

    auto addTriangle = [&](const glm::vec3 &p1, const glm::vec3 &n1, 
                           const glm::vec3 &p2, const glm::vec3 &n2, 
                           const glm::vec3 &p3, const glm::vec3 &n3) { 
        insertVec3(vertices, p1); 
        insertVec3(vertices, n1); 
        insertVec3(vertices, p2); 
        insertVec3(vertices, n2); 
        insertVec3(vertices, p3); 
        insertVec3(vertices, n3); 
    }; 

    auto pointOnCircle = [&](float theta, float y) -> glm::vec3 { 
        return glm::vec3( 
            CYLINDER_RADIUS * std::cos(theta), 
            y, 
            CYLINDER_RADIUS * std::sin(theta) 
        ); 
    }; 

    // ================== Barrel ================== 
    for (int i = 0; i < radialSegments; ++i) { 
        float thetaCurr = i * thetaStep; 
        float thetaNext = (i + 1) * thetaStep; 

        glm::vec3 normalCurr = glm::normalize(glm::vec3(std::cos(thetaCurr), 0.0f, std::sin(thetaCurr))); 
        glm::vec3 normalNext = glm::normalize(glm::vec3(std::cos(thetaNext), 0.0f, std::sin(thetaNext))); 

        for (int j = 0; j < verticalSegments; ++j) { 
            float yBottom = -CYLINDER_HALF_HEIGHT + j * heightStep; 
            float yTop = yBottom + heightStep; 

            glm::vec3 bottomLeft = pointOnCircle(thetaCurr, yBottom); 
            glm::vec3 bottomRight = pointOnCircle(thetaNext, yBottom); 
            glm::vec3 topLeft = pointOnCircle(thetaCurr, yTop); 
            glm::vec3 topRight = pointOnCircle(thetaNext, yTop); 

            // First triangle 
            addTriangle(topLeft, normalCurr, 
                        topRight, normalNext, 
                        bottomRight, normalNext); 

            // Second triangle 
            addTriangle(topLeft, normalCurr, 
                        bottomRight, normalNext, 
                        bottomLeft, normalCurr); 
        } 
    } 

    // ================== Caps ================== 
    int radialRings = std::max(1, param1); 
    glm::vec3 topCenter(0.0f, CYLINDER_HALF_HEIGHT, 0.0f); 
    glm::vec3 bottomCenter(0.0f, -CYLINDER_HALF_HEIGHT, 0.0f); 

    auto lerpCapPoint = [&](const glm::vec3 &center, float theta, float t) -> glm::vec3 { 
        glm::vec3 edge = pointOnCircle(theta, center.y); 
        return lerpVec3(center, edge, t); 
    }; 

    auto addCapTile = [&](bool top, 
                          const glm::vec3 &innerLeft, 
                          const glm::vec3 &innerRight, 
                          const glm::vec3 &outerLeft, 
                          const glm::vec3 &outerRight) { 
        glm::vec3 normal = top ? glm::vec3(0.0f, 1.0f, 0.0f) 
                               : glm::vec3(0.0f, -1.0f, 0.0f); 

        if (top) { 
            addTriangle(innerLeft, normal, 
                        outerRight, normal, 
                        outerLeft, normal); 
            addTriangle(innerLeft, normal, 
                        innerRight, normal, 
                        outerRight, normal); 
        } else { 
            addTriangle(innerLeft, normal, 
                        outerLeft, normal, 
                        outerRight, normal); 
            addTriangle(innerLeft, normal, 
                        outerRight, normal, 
                        innerRight, normal); 
        } 
    }; 

    for (int i = 0; i < radialSegments; ++i) { 
        float thetaCurr = i * thetaStep; 
        float thetaNext = (i + 1) * thetaStep; 

        for (int ring = 0; ring < radialRings; ++ring) { 
            float t0 = static_cast<float>(ring) / radialRings; 
            float t1 = static_cast<float>(ring + 1) / radialRings; 

            glm::vec3 topInnerLeft  = lerpCapPoint(topCenter, thetaCurr, t0); 
            glm::vec3 topInnerRight = lerpCapPoint(topCenter, thetaNext, t0); 
            glm::vec3 topOuterLeft  = lerpCapPoint(topCenter, thetaCurr, t1); 
            glm::vec3 topOuterRight = lerpCapPoint(topCenter, thetaNext, t1); 
            addCapTile(true, topInnerLeft, topInnerRight, topOuterLeft, topOuterRight); 

            glm::vec3 bottomInnerLeft  = lerpCapPoint(bottomCenter, thetaCurr, t0); 
            glm::vec3 bottomInnerRight = lerpCapPoint(bottomCenter, thetaNext, t0); 
            glm::vec3 bottomOuterLeft  = lerpCapPoint(bottomCenter, thetaCurr, t1); 
            glm::vec3 bottomOuterRight = lerpCapPoint(bottomCenter, thetaNext, t1); 
            addCapTile(false, bottomInnerLeft, bottomInnerRight, bottomOuterLeft, bottomOuterRight); 
        } 
    } 
} 

void Tessellator::insertVec3(std::vector<float> &data, const glm::vec3 &v) { 
    data.push_back(v.x); 
    data.push_back(v.y); 
    data.push_back(v.z); 
} 
