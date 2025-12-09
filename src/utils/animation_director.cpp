#include "animation_director.h"
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <set>
#include <map>

void AnimationDirector::initialize(const RenderData& renderData) {
    m_renderData = &renderData;
    m_pathAnimations.clear();
    m_glbAnimations.clear();
    m_meshfileToShapeIndex.clear();
    
    // build meshfile to shape index mapping
    for (size_t i = 0; i < renderData.shapes.size(); ++i) {
        const auto& shape = renderData.shapes[i];
        if (shape.primitive.type == PrimitiveType::PRIMITIVE_MESH && 
            !shape.primitive.meshfile.empty()) {
            m_meshfileToShapeIndex[shape.primitive.meshfile] = i;
        }
    }
}

void AnimationDirector::addPathAnimation(const std::string& meshfile, 
                                        const std::vector<PathKeyframe>& keyframes, 
                                        bool loop) {
    auto it = m_meshfileToShapeIndex.find(meshfile);
    if (it != m_meshfileToShapeIndex.end()) {
        addPathAnimation(it->second, keyframes, loop);
    }
}

void AnimationDirector::addPathAnimation(size_t shapeIndex,
                                        const std::vector<PathKeyframe>& keyframes,
                                        bool loop) {
    if (!m_renderData || shapeIndex >= m_renderData->shapes.size()) {
        std::cout << "[Animation] WARNING: Cannot add path animation - invalid shapeIndex " << shapeIndex << std::endl;
        return;
    }
    
    PathAnimation anim;
    anim.keyframes = keyframes;
    anim.loop = loop;
    anim.enabled = true;
    anim.baseTransform = m_renderData->shapes[shapeIndex].ctm;
    
    if (!keyframes.empty()) {
        anim.duration = keyframes.back().time;
    }
    
    m_pathAnimations[shapeIndex] = anim;
    std::cout << "[Animation] Added path animation for shape " << shapeIndex 
              << " with " << keyframes.size() << " keyframes, duration=" << anim.duration 
              << "s, loop=" << (loop ? "yes" : "no") << std::endl;
}

void AnimationDirector::setGLBAnimation(const std::string& meshfile,
                                       float startTime,
                                       float duration,
                                       int animIndex,
                                       bool loop,
                                       bool ignoreRootTranslation,
                                       float speed) {
    GLBAnimationControl control;
    control.startTime = startTime;
    control.duration = duration;
    control.animationIndex = animIndex;
    control.loop = loop;
    control.enabled = true;
    control.ignoreRootTranslation = ignoreRootTranslation;
    control.speed = speed;
    
    m_glbAnimations[meshfile] = control;
}

void AnimationDirector::setupTitanFishAnimation() {
    std::cout << "[Animation] setupTitanFishAnimation() called" << std::endl;
    if (!m_renderData) {
        std::cout << "[Animation] ERROR: m_renderData is null!" << std::endl;
        return;
    }
    
    std::cout << "[Animation] Total shapes: " << m_renderData->shapes.size() << std::endl;
    
    // find titan and fish shape indices
    size_t titanIndex = SIZE_MAX;
    size_t fishIndex = SIZE_MAX;
    std::string titanMeshfile;
    std::string fishMeshfile;
    
    for (size_t i = 0; i < m_renderData->shapes.size(); ++i) {
        const auto& shape = m_renderData->shapes[i];
        if (shape.primitive.type == PrimitiveType::PRIMITIVE_MESH) {
            std::string meshfile = shape.primitive.meshfile;
            std::cout << "[Animation] Shape " << i << ": meshfile = " << meshfile << std::endl;
            
            // check for titan (case insensitive)
            std::string meshfileLower = meshfile;
            std::transform(meshfileLower.begin(), meshfileLower.end(), meshfileLower.begin(), ::tolower);
            if (meshfileLower.find("titan") != std::string::npos) {
                titanIndex = i;
                titanMeshfile = meshfile;
                std::cout << "[Animation] Found titan at index " << i << ", meshfile: " << meshfile << std::endl;
            } else if (meshfile.find("alien_fish") != std::string::npos) {
                fishIndex = i;
                fishMeshfile = meshfile;
                std::cout << "[Animation] Found fish at index " << i << ", meshfile: " << meshfile << std::endl;
            }
        }
    }
    
    // setup model scales for unified sizing
    // titan: keep current visibility scale
    // fish: shrink to 50%
    // whale: shrink to 10%
    // these scales are relative multipliers applied in drawMeshPrimitive
    setModelScale("titan", 0.02f);        // make titan more visible
    setModelScale("alien_fish", 0.2f);    // fish at 50% of original
    setModelScale("glow_whale", 0.1f);    // whale at 10% of original
    
    // setup titan path animation: move from left to right
    // note: scale and rotation in keyframes are relative, model-specific adjustments applied separately
    if (titanIndex != SIZE_MAX) {
        std::cout << "[Animation] Setting up titan animation at index " << titanIndex << std::endl;
        std::cout << "[Animation] Titan scale: " << getModelScale(titanMeshfile) << std::endl;
        std::vector<PathKeyframe> titanKeyframes = {
            {0.0f, glm::vec3(-8.0f, -1.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f)},
            {5.0f, glm::vec3(16.0f, -1.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f)},
        };
        addPathAnimation(titanIndex, titanKeyframes, false);
        
        // titan glb animation: play continuously (only rotation/scale, no translation)
        // duration=0 means loop the full animation duration
        // speed=0.5 means play at half speed (slower)
        setGLBAnimation(titanMeshfile, 0.0f, 0.0f, 0, true, true, 0.2f);
        std::cout << "[Animation] Titan GLB animation speed set to 0.5x (half speed)" << std::endl;
    } else {
        std::cout << "[Animation] WARNING: Titan not found!" << std::endl;
    }
    
    // setup fish path animation: stay at center, don't move
    // note: scale and rotation in keyframes are relative, model-specific adjustments applied separately
    if (fishIndex != SIZE_MAX) {
        std::cout << "[Animation] Setting up fish animation at index " << fishIndex << std::endl;
        std::cout << "[Animation] Fish scale: " << getModelScale(fishMeshfile) << std::endl;
        std::vector<PathKeyframe> fishKeyframes = {
            {0.0f, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f)},
        };
        addPathAnimation(fishIndex, fishKeyframes, false);
        
        // fish glb animation: swimming animation (only rotation/scale, no translation)
        setGLBAnimation(fishMeshfile, 0.0f, 0.0f, 0, true, true);
    }
}

void AnimationDirector::update(float deltaSec) {
    if (m_playing) {
        m_currentTime += deltaSec;
    }
}

void AnimationDirector::setTime(float time) {
    m_currentTime = time;
}

void AnimationDirector::play() {
    m_playing = true;
}

void AnimationDirector::pause() {
    m_playing = false;
}

void AnimationDirector::reset() {
    m_currentTime = 0.f;
}

float AnimationDirector::getCurrentTime() const {
    return m_currentTime;
}

glm::mat4 AnimationDirector::getTransform(size_t shapeIndex) const {
    return evaluatePathAnimation(shapeIndex, m_currentTime);
}

glm::mat4 AnimationDirector::getTransform(const std::string& meshfile) const {
    auto it = m_meshfileToShapeIndex.find(meshfile);
    if (it != m_meshfileToShapeIndex.end()) {
        return getTransform(it->second);
    }
    return glm::mat4(1.0f);
}

float AnimationDirector::getGLBAnimationTime(const std::string& meshfile) const {
    auto it = m_glbAnimations.find(meshfile);
    if (it != m_glbAnimations.end() && it->second.enabled) {
        const auto& control = it->second;
        float baseTime = m_currentTime - control.startTime;
        float localTime = baseTime * control.speed;  // apply speed multiplier
        
        if (localTime < 0.f) {
            return 0.f;  // animation not started yet
        }
        
        if (control.duration > 0.f) {
            if (control.loop) {
                return std::fmod(localTime, control.duration);
            } else {
                return std::min(localTime, control.duration);
            }
        } else {
            // duration 0 means loop the full animation duration
            // the caller (GLBLoader) will handle the looping based on animation.duration
            // return localTime (already multiplied by speed), GLBLoader will mod by animation.duration
            return localTime;
        }
    }
    return 0.f;
}

int AnimationDirector::getGLBAnimationIndex(const std::string& meshfile) const {
    auto it = m_glbAnimations.find(meshfile);
    if (it != m_glbAnimations.end() && it->second.enabled) {
        return it->second.animationIndex;
    }
    return 0;  // default to first animation
}

bool AnimationDirector::isGLBAnimationActive(const std::string& meshfile) const {
    auto it = m_glbAnimations.find(meshfile);
    if (it != m_glbAnimations.end() && it->second.enabled) {
        const auto& control = it->second;
        float localTime = m_currentTime - control.startTime;
        if (control.duration > 0.f) {
            // fixed duration: check if within time range
            return localTime >= 0.f && (control.loop || localTime <= control.duration);
        } else {
            // duration 0 means loop full animation: always active once started
            return localTime >= 0.f;
        }
    }
    return false;
}

bool AnimationDirector::shouldIgnoreRootTranslation(const std::string& meshfile) const {
    auto it = m_glbAnimations.find(meshfile);
    if (it != m_glbAnimations.end() && it->second.enabled) {
        return it->second.ignoreRootTranslation;
    }
    return false;
}

void AnimationDirector::printAnimationInfo() const {
    std::cout << "=== Animation Director Info ===" << std::endl;
    std::cout << "Current time: " << m_currentTime << "s" << std::endl;
    std::cout << "Playing: " << (m_playing ? "yes" : "no") << std::endl;
    std::cout << "Path animations: " << m_pathAnimations.size() << std::endl;
    std::cout << "GLB animations: " << m_glbAnimations.size() << std::endl;
    
    for (const auto& [index, anim] : m_pathAnimations) {
        std::cout << "  Shape " << index << ": " << anim.keyframes.size() 
                  << " keyframes, duration=" << anim.duration << "s" << std::endl;
    }
    
    for (const auto& [meshfile, control] : m_glbAnimations) {
        std::cout << "  GLB " << meshfile << ": start=" << control.startTime 
                  << "s, duration=" << control.duration << "s, index=" 
                  << control.animationIndex << std::endl;
    }
}

std::vector<PathKeyframe> AnimationDirector::getPathKeyframes(size_t shapeIndex) const {
    auto it = m_pathAnimations.find(shapeIndex);
    if (it != m_pathAnimations.end()) {
        return it->second.keyframes;
    }
    return {};
}

glm::mat4 AnimationDirector::evaluatePathAnimation(size_t shapeIndex, float time) const {
    auto it = m_pathAnimations.find(shapeIndex);
    if (it == m_pathAnimations.end() || !it->second.enabled) {
        // no animation path, return original ctm
        if (m_renderData && shapeIndex < m_renderData->shapes.size()) {
            return m_renderData->shapes[shapeIndex].ctm;
        }
        return glm::mat4(1.0f);
    }
    
    const PathAnimation& anim = it->second;
    if (anim.keyframes.empty()) {
        return anim.baseTransform;
    }
    
    // special case: only one keyframe, use it directly
    if (anim.keyframes.size() == 1) {
        const PathKeyframe& kf = anim.keyframes[0];
        glm::mat4 transform = glm::translate(kf.position);
        transform = glm::rotate(transform, glm::radians(kf.rotation.z), glm::vec3(0, 0, 1));
        transform = glm::rotate(transform, glm::radians(kf.rotation.y), glm::vec3(0, 1, 0));
        transform = glm::rotate(transform, glm::radians(kf.rotation.x), glm::vec3(1, 0, 0));
        transform = glm::scale(transform, kf.scale);
        return anim.baseTransform * transform;
    }
    
    // handle loop and time range
    float animTime = getLocalTime(time, anim);
    
    // find current keyframe interval
    size_t keyframeIndex = 0;
    bool foundInterval = false;
    for (size_t i = 0; i < anim.keyframes.size() - 1; ++i) {
        if (animTime >= anim.keyframes[i].time && 
            animTime <= anim.keyframes[i + 1].time) {
            keyframeIndex = i;
            foundInterval = true;
            break;
        }
    }
    
    // if time exceeds last keyframe, use last keyframe directly
    if (!foundInterval && animTime > anim.keyframes.back().time) {
        const PathKeyframe& lastKf = anim.keyframes.back();
        glm::mat4 transform = glm::translate(lastKf.position);
        transform = glm::rotate(transform, glm::radians(lastKf.rotation.z), glm::vec3(0, 0, 1));
        transform = glm::rotate(transform, glm::radians(lastKf.rotation.y), glm::vec3(0, 1, 0));
        transform = glm::rotate(transform, glm::radians(lastKf.rotation.x), glm::vec3(1, 0, 0));
        transform = glm::scale(transform, lastKf.scale);
        return anim.baseTransform * transform;
    }
    
    // if time is before first keyframe, use first keyframe
    if (!foundInterval && animTime < anim.keyframes[0].time) {
        const PathKeyframe& firstKf = anim.keyframes[0];
        glm::mat4 transform = glm::translate(firstKf.position);
        transform = glm::rotate(transform, glm::radians(firstKf.rotation.z), glm::vec3(0, 0, 1));
        transform = glm::rotate(transform, glm::radians(firstKf.rotation.y), glm::vec3(0, 1, 0));
        transform = glm::rotate(transform, glm::radians(firstKf.rotation.x), glm::vec3(1, 0, 0));
        transform = glm::scale(transform, firstKf.scale);
        return anim.baseTransform * transform;
    }
    
    // if no interval found (should not happen with valid keyframes), use last keyframe as fallback
    if (!foundInterval) {
        const PathKeyframe& lastKf = anim.keyframes.back();
        glm::mat4 transform = glm::translate(lastKf.position);
        transform = glm::rotate(transform, glm::radians(lastKf.rotation.z), glm::vec3(0, 0, 1));
        transform = glm::rotate(transform, glm::radians(lastKf.rotation.y), glm::vec3(0, 1, 0));
        transform = glm::rotate(transform, glm::radians(lastKf.rotation.x), glm::vec3(1, 0, 0));
        transform = glm::scale(transform, lastKf.scale);
        return anim.baseTransform * transform;
    }
    
    // linear interpolation between keyframes
    const PathKeyframe& kf1 = anim.keyframes[keyframeIndex];
    const PathKeyframe& kf2 = anim.keyframes[keyframeIndex + 1];
    
    float t = 0.f;
    if (kf2.time > kf1.time) {
        t = (animTime - kf1.time) / (kf2.time - kf1.time);
        t = std::max(0.f, std::min(1.f, t));
    }
    
    // interpolate
    glm::vec3 pos = glm::mix(kf1.position, kf2.position, t);
    glm::vec3 rot = glm::mix(kf1.rotation, kf2.rotation, t);
    glm::vec3 scale = glm::mix(kf1.scale, kf2.scale, t);
    
    // build transform matrix
    glm::mat4 transform = glm::translate(pos);
    transform = glm::rotate(transform, glm::radians(rot.z), glm::vec3(0, 0, 1));
    transform = glm::rotate(transform, glm::radians(rot.y), glm::vec3(0, 1, 0));
    transform = glm::rotate(transform, glm::radians(rot.x), glm::vec3(1, 0, 0));
    transform = glm::scale(transform, scale);
    
    // apply base transform
    return anim.baseTransform * transform;
}

float AnimationDirector::getLocalTime(float globalTime, const PathAnimation& anim) const {
    if (anim.loop && anim.duration > 0.f) {
        return std::fmod(globalTime, anim.duration);
    } else {
        // if duration is 0, return 0 (for single keyframe animations)
        if (anim.duration <= 0.f) {
            return 0.f;
        }
        return std::min(globalTime, anim.duration);
    }
}

float AnimationDirector::getModelScale(const std::string& meshfile) const {
    // check for exact match first
    auto it = m_modelScales.find(meshfile);
    if (it != m_modelScales.end()) {
        return it->second;
    }
    
    // check for partial match (case insensitive)
    std::string meshfileLower = meshfile;
    std::transform(meshfileLower.begin(), meshfileLower.end(), meshfileLower.begin(), ::tolower);
    
    for (const auto& [key, scale] : m_modelScales) {
        std::string keyLower = key;
        std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), ::tolower);
        if (meshfileLower.find(keyLower) != std::string::npos || 
            keyLower.find(meshfileLower) != std::string::npos) {
            return scale;
        }
    }
    
    // default scale if not found
    return 1.0f;
}

void AnimationDirector::setModelScale(const std::string& meshfile, float scale) {
    m_modelScales[meshfile] = scale;
}
