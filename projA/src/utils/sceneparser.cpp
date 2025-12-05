#include "sceneparser.h"
#include "scenefilereader.h"
#include <glm/gtx/transform.hpp>

#include <chrono>
#include <iostream>

bool SceneParser::parse(std::string filepath, RenderData &renderData) {
    ScenefileReader fileReader = ScenefileReader(filepath);
    bool success = fileReader.readJSON();
    if (!success) {
        return false;
    }

    // TODO: Use your Lab 5 code here
    // Populate renderData with global data, and camera data;
    renderData.globalData = fileReader.getGlobalData();
    renderData.cameraData = fileReader.getCameraData();

    // Populate renderData's list of primitives and their transforms.
    // This will involve traversing the scene graph, and we recommend you create a helper function to do so!
    SceneNode* root = fileReader.getRootNode();
    renderData.shapes.clear();
    renderData.lights.clear();

    traverseNodes(root, glm::mat4(1.0f), renderData.shapes, renderData.lights);

    return true;
}

void SceneParser::traverseNodes(SceneNode* node, glm::mat4 parentCTM, std::vector<RenderShapeData>& shapes, std::vector<SceneLightData>& lights) {
    if(node == nullptr) return;

    // combine local transformations
    glm::mat4 local_transformation = glm::mat4(1.0f);
    for (auto& t : node->transformations) {
        if(t->type == TransformationType::TRANSFORMATION_TRANSLATE){
            local_transformation = local_transformation * glm::translate(t->translate);
        } else if(t->type == TransformationType::TRANSFORMATION_ROTATE){
            local_transformation = local_transformation * glm::rotate(t->angle, t->rotate);
        } else if(t->type == TransformationType::TRANSFORMATION_SCALE){
            local_transformation = local_transformation * glm::scale(t->scale);
        } else if(t->type == TransformationType::TRANSFORMATION_MATRIX){
            local_transformation = local_transformation * t->matrix;
        }
    }

    // multipy by the parent CTM
    glm::mat4 currentCTM = parentCTM * local_transformation;

    // store primitive
    for (auto& p : node->primitives) {
        RenderShapeData shape;
        shape.primitive = *p;
        shape.ctm = currentCTM;
        shapes.push_back(shape);
    }

    // store light
    for (auto& l : node->lights) {
        SceneLightData lightData;

        lightData.id = l->id;
        lightData.type = l->type;
        lightData.color = l->color;
        lightData.function = l->function;
        lightData.penumbra = l->penumbra;
        lightData.angle = l->angle;
        lightData.width = l->width;
        lightData.height = l->height;

        // lightData.pos = currentCTM * l->dir;
        // lightData.dir = glm::normalize(glm::vec4(l->dir.x, l->dir.y, l->dir.z, 0.0f) * currentCTM);
        glm::vec4 origin = currentCTM * glm::vec4(0.f, 0.f, 0.f, 1.f);
        if (l->type == LightType::LIGHT_DIRECTIONAL) {
            lightData.pos = glm::vec4(0.f, 0.f, 0.f, 0.f);
        } else {
            lightData.pos = origin;
        }

        glm::vec4 dir4 = glm::vec4(l->dir.x, l->dir.y, l->dir.z, 0.0f);
        glm::vec3 worldDir = glm::vec3(currentCTM * dir4);
        if (glm::length(worldDir) == 0.f) {
            worldDir = glm::vec3(0.f, -1.f, 0.f);
        } else {
            worldDir = glm::normalize(worldDir);
        }
        lightData.dir = glm::vec4(worldDir, 0.f);

        lights.push_back(lightData);
    }

    for (auto child : node->children) {
        traverseNodes(child, currentCTM, shapes, lights);
    }
}
