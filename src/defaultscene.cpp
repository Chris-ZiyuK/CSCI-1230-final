#include "DefaultScene.h"
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <algorithm>
#include <cmath>
#include <random>

namespace {

glm::vec3 randomStarHue(std::mt19937 &rng) {
    std::uniform_real_distribution<float> mix(0.f, 1.f);
    glm::vec3 warm = glm::vec3(1.4f, 1.2f, 0.8f);
    glm::vec3 cool = glm::vec3(0.9f, 1.0f, 1.3f);
    return glm::mix(warm, cool, mix(rng));
}

void addStarField(RenderData &renderData, int count,
                  float scaleMin, float scaleMax,
                  float emissiveMin, float emissiveMax) {
    std::mt19937 rng(20251125+ static_cast<int>((scaleMin + scaleMax) * 1000));

    // Sample stars uniformly on a spherical shell around the camera
    const glm::vec3 starCenter(0.f, 0.f, 5.f);
    const float twoPi = 6.28318530718f;
    std::uniform_real_distribution<float> radiusDist(12.f, 44.f); // keep inside sky sphere (r=50) and away from camera
    std::uniform_real_distribution<float> zDist(-1.f, 1.f);
    std::uniform_real_distribution<float> thetaDist(0.f, twoPi);
    std::uniform_real_distribution<float> scaleDist(scaleMin, scaleMax);
    std::uniform_real_distribution<float> emissiveDist(emissiveMin, emissiveMax);
    std::uniform_real_distribution<float> tilt(-0.5f, 0.5f);

    for (int i = 0; i < count; ++i) {
        RenderShapeData star;
        star.primitive.type = PrimitiveType::PRIMITIVE_STAR;

        // direction uniformly sampled on sphere
        float z = zDist(rng);
        float theta = thetaDist(rng);
        float xy = std::sqrt(std::max(0.f, 1.f - z * z));
        glm::vec3 dir(xy * std::cos(theta), xy * std::sin(theta), z);

        float radius = radiusDist(rng);
        glm::vec3 pos = starCenter + dir * radius;
        float scale = scaleDist(rng);

        glm::mat4 transform = glm::translate(pos);
        transform = transform * glm::rotate(tilt(rng), glm::vec3(0.f, 1.f, 0.f));
        transform = transform * glm::rotate(tilt(rng), glm::vec3(0.f, 0.f, 1.f));
        transform = transform * glm::scale(glm::vec3(scale));
        star.ctm = transform;

        glm::vec3 hue = randomStarHue(rng);
        float emissive = emissiveDist(rng);

        star.primitive.material.cDiffuse = glm::vec4(hue, 1.f);
        star.primitive.material.cAmbient = glm::vec4(hue * 0.4f, 1.f);
        star.primitive.material.cSpecular = glm::vec4(glm::vec3(1.2f), 1.f);
        star.primitive.material.shininess = 96.f;
        star.primitive.material.cEmissive = glm::vec4(hue * emissive, 1.f);

        renderData.shapes.push_back(star);
    }
}

} // namespace

void createDefaultScene(RenderData &renderData) {

    renderData = RenderData();

    // ===== Global =====
    renderData.globalData.ka = 0.1f;
    renderData.globalData.kd = 0.8f;
    renderData.globalData.ks = 0.2f;

    // ===== Camera =====
    renderData.cameraData.pos = glm::vec4(0, 0, 5, 1);
    renderData.cameraData.look = glm::vec4(0, 0, -1, 0);
    renderData.cameraData.up = glm::vec4(0, 1, 0, 0);
    renderData.cameraData.heightAngle = glm::radians(45.f);

    // ===== Sky Sphere =====
    RenderShapeData sky;
    sky.primitive.type = PrimitiveType::PRIMITIVE_SPHERE;
    sky.ctm = glm::scale(glm::vec3(50.f));

    sky.primitive.material.cDiffuse = glm::vec4(0, 0, 0, 1);
    sky.primitive.material.cAmbient = glm::vec4(0, 0, 0, 1);
    sky.primitive.material.blend = -1.f;

    renderData.shapes.push_back(sky);

    // ===== Bloom Planet =====
    // RenderShapeData planet;
    // planet.primitive.type = PrimitiveType::PRIMITIVE_SPHERE;
    // planet.ctm = glm::translate(glm::vec3(1.6, 1.8, -2)) *
    //              glm::scale(glm::vec3(0.8f)); // size

    // hightlight
    // planet.primitive.material.cDiffuse  = glm::vec4(2.4f, 2.2f, 1.1f, 1.f);   // warm yellow glow
    // planet.primitive.material.cAmbient  = glm::vec4(0.9f, 0.85f, 0.6f, 1.f);
    // planet.primitive.material.cSpecular = glm::vec4(1.4f, 1.25f, 0.8f, 1.f);
    // planet.primitive.material.cEmissive = glm::vec4(3.5f, 2.8f, 1.2f, 1.f);   // drives bloom around whole sphere

    // renderData.shapes.push_back(planet);

    // ===== Procedural star field =====
    addStarField(renderData, 1000, 0.035f, 0.13f, 1.2f, 3.5f);
    addStarField(renderData, 6000, 0.025f, 0.038f, 0.8f, 1.8f);
}
