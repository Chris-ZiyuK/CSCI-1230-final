#include "animation_director.h"
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <set>
#include <map>
#include <cmath>
#include <filesystem>

void AnimationDirector::initialize(const RenderData& renderData) {
    m_renderData = &renderData;
    m_pathAnimations.clear();
    m_glbAnimations.clear();
    m_meshfileToShapeIndex.clear();
    m_shapeIndexToMeshfile.clear();
    m_hiddenShapes.clear();
    m_titanIndex = SIZE_MAX;
    m_fishIndex = SIZE_MAX;
    m_titanMeshfile.clear();
    m_fishMeshfile.clear();
    
    // reset camera animation
    m_cameraFollowTarget = false;
    m_cameraPathEnabled = false;
    m_cameraAnimatedOffset = false;
    m_cameraWideShot = false;
    m_cameraOrbitMode = false;
    m_cameraUseLastPos = false;
    m_cameraSwitchActive = false;
    m_cameraSwitchStartTime = -1.f;
    m_cameraPullbackActive = false;
    m_cameraPullbackFinished = false;
    m_cameraPullbackStartTime = -1.f;
    m_cameraHoldAfterPullback = false;
    m_cameraHoldPos = glm::vec3(0.f);
    m_cameraHoldLook = glm::vec3(0.f, 0.f, -1.f);
    
    // build meshfile to shape index mapping
    for (size_t i = 0; i < renderData.shapes.size(); ++i) {
        const auto& shape = renderData.shapes[i];
        if (shape.primitive.type == PrimitiveType::PRIMITIVE_MESH && 
            !shape.primitive.meshfile.empty()) {
            m_meshfileToShapeIndex[shape.primitive.meshfile] = i;
            m_shapeIndexToMeshfile[i] = shape.primitive.meshfile;
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
                                       float speed,
                                       bool pingPong) {
    GLBAnimationControl control;
    control.startTime = startTime;
    control.duration = duration;
    control.animationIndex = animIndex;
    control.loop = loop;
    control.enabled = true;
    control.ignoreRootTranslation = ignoreRootTranslation;
    control.speed = speed;
    control.pingPong = pingPong;
    
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
        m_titanIndex = titanIndex;
        m_titanMeshfile = titanMeshfile;
        std::cout << "[Animation] Setting up titan animation at index " << titanIndex << std::endl;
        std::cout << "[Animation] Titan scale: " << getModelScale(titanMeshfile) << std::endl;
        m_titanPathEndTime = 10.0f; // original path end (for pullback trigger)
        std::vector<PathKeyframe> titanKeyframes = {
            {0.0f, glm::vec3(-18.0f, -1.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f)},
            {10.0f, glm::vec3(16.0f, -1.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f)},
            // extend with same speed to fly out of screen to the right
            {22.0f, glm::vec3(56.8f, -1.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f)},
        };
        // loop disabled: titan path ends after duration
        addPathAnimation(titanIndex, titanKeyframes, false);
        
        // titan glb animation: play continuously (only rotation/scale, no translation)
        // duration=0 means loop the full animation duration
        // speed=0.2 means play at 20% speed (slower)
        // pingPong=true means play forward then backward for smooth looping
        setGLBAnimation(titanMeshfile, 0.0f, 0.0f, 0, true, true, 0.2f, true);
        std::cout << "[Animation] Titan GLB animation: speed=0.2x, ping-pong mode enabled for smooth looping" << std::endl;
    } else {
        std::cout << "[Animation] WARNING: Titan not found!" << std::endl;
    }
    
    // setup fish path animation: stay at center, don't move
    // note: scale and rotation in keyframes are relative, model-specific adjustments applied separately
    if (fishIndex != SIZE_MAX) {
        m_fishIndex = fishIndex;
        m_fishMeshfile = fishMeshfile;
        std::cout << "[Animation] Setting up fish animation at index " << fishIndex << std::endl;
        std::cout << "[Animation] Fish scale: " << getModelScale(fishMeshfile) << std::endl;
        std::vector<PathKeyframe> fishKeyframes = {
            // swim from right to left across titan's path
            {0.0f, glm::vec3(12.0f, 0.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f)},
            {10.0f, glm::vec3(-12.0f, -1.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f)},
        };
        addPathAnimation(fishIndex, fishKeyframes, false);
        
        // fish glb animation: swimming animation (only rotation/scale, no translation)
        setGLBAnimation(fishMeshfile, 0.0f, 0.0f, 0, true, true);
    }

    // ensure everything is visible at start
    resetVisibility();
    
    // setup dramatic camera movement: orbit from front to side while following fish
    if (fishIndex != SIZE_MAX) {
        // fish animation duration is 10 seconds
        // camera orbits from front (0°) to side (90°) in 1/4 circle
        // simultaneously follows fish's translation
        setCameraOrbitTarget(fishIndex,
                           5.f,      // radius: 5 units from fish
                           0.f,      // start: front (0°)
                           90.f,     // end: right side (90°)
                           10.f,     // orbit duration: 10 seconds (full fish animation)
                           glm::vec3(0.f, 1.f, 0.f));  // vertical offset: slightly above
        
        std::cout << "[Animation] Camera setup: orbit from front to side while following fish" << std::endl;
    }
}

void AnimationDirector::update(float deltaSec) {
    if (m_playing) {
        m_currentTime += deltaSec;
        if (m_autoStopTime >= 0.f && m_currentTime >= m_autoStopTime) {
            m_currentTime = m_autoStopTime;
            m_playing = false;
        }
    }

    // collision check: titan vs fish
    if (m_titanIndex != SIZE_MAX && m_fishIndex != SIZE_MAX &&
        isShapeVisible(m_fishIndex)) {
        glm::mat4 titanMat = evaluatePathAnimation(m_titanIndex, m_currentTime);
        glm::mat4 fishMat = evaluatePathAnimation(m_fishIndex, m_currentTime);
        glm::vec3 titanPos = extractPosition(titanMat);
        glm::vec3 fishPos = extractPosition(fishMat);

        const float collisionDistance = 1.0f;
        if (glm::length(titanPos - fishPos) <= collisionDistance) {
            // remember last fish position for smooth camera handoff
            m_cameraLastTargetPos = fishPos;
            hideShape(m_fishIndex);
            std::cout << "[Animation] Titan touched fish -> fish hidden" << std::endl;
            // start smooth camera handoff toward titan
            m_cameraSwitchActive = true;
            m_cameraSwitchStartTime = m_currentTime;
        }
    }

    // finish camera handoff after blend duration
    if (m_cameraSwitchActive && m_cameraSwitchDuration > 0.f && m_cameraSwitchStartTime >= 0.f) {
        if ((m_currentTime - m_cameraSwitchStartTime) >= m_cameraSwitchDuration) {
            m_cameraSwitchActive = false;
        }
    }

    // trigger pullback after original titan path end
    if (!m_cameraPullbackActive && !m_cameraPullbackFinished &&
        m_currentTime >= m_titanPathEndTime) {
        m_cameraPullbackActive = true;
        m_cameraPullbackStartTime = m_currentTime;
    }
    if (m_cameraPullbackActive && m_cameraPullbackDuration > 0.f && m_cameraPullbackStartTime >= 0.f) {
        if ((m_currentTime - m_cameraPullbackStartTime) >= m_cameraPullbackDuration) {
            m_cameraPullbackActive = false;
            m_cameraPullbackFinished = true;
        }
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
    m_playing = true;
    resetVisibility();
    m_cameraSwitchActive = false;
    m_cameraSwitchStartTime = -1.f;
    m_cameraPullbackActive = false;
    m_cameraPullbackFinished = false;
    m_cameraPullbackStartTime = -1.f;
    m_cameraHoldAfterPullback = false;
}

bool AnimationDirector::isPlaying() const {
    return m_playing;
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
    const GLBAnimationControl* control = findGLBAnimationControl(meshfile);
    if (control) {
        float baseTime = m_currentTime - control->startTime;
        float localTime = baseTime * control->speed;  // apply speed multiplier
        
        if (localTime < 0.f) {
            return 0.f;  // animation not started yet
        }
        
        if (control->duration > 0.f) {
            if (control->loop) {
                return std::fmod(localTime, control->duration);
            } else {
                return std::min(localTime, control->duration);
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
    const GLBAnimationControl* control = findGLBAnimationControl(meshfile);
    return control ? control->animationIndex : 0;  // default to first animation
}

bool AnimationDirector::isGLBAnimationActive(const std::string& meshfile) const {
    const GLBAnimationControl* control = findGLBAnimationControl(meshfile);
    if (control) {
        float localTime = m_currentTime - control->startTime;
        if (control->duration > 0.f) {
            // fixed duration: check if within time range
            return localTime >= 0.f && (control->loop || localTime <= control->duration);
        } else {
            // duration 0 means loop full animation: always active once started
            return localTime >= 0.f;
        }
    }
    return false;
}

bool AnimationDirector::shouldIgnoreRootTranslation(const std::string& meshfile) const {
    const GLBAnimationControl* control = findGLBAnimationControl(meshfile);
    return control ? control->ignoreRootTranslation : false;
}

// ANIMATION: helper to find GLB animation control with filename fallback
// this handles path resolution differences (relative vs absolute paths)
const GLBAnimationControl* AnimationDirector::findGLBAnimationControl(const std::string& meshfile) const {
    // first try exact match
    auto it = m_glbAnimations.find(meshfile);
    if (it != m_glbAnimations.end() && it->second.enabled) {
        return &it->second;
    }

    // fallback: match by filename to handle resolved/relative path differences
    std::filesystem::path queryPath(meshfile);
    std::string queryName = queryPath.filename().string();
    std::transform(queryName.begin(), queryName.end(), queryName.begin(), ::tolower);

    for (const auto& kv : m_glbAnimations) {
        std::filesystem::path storedPath(kv.first);
        std::string storedName = storedPath.filename().string();
        std::transform(storedName.begin(), storedName.end(), storedName.begin(), ::tolower);

        if (storedName == queryName && kv.second.enabled) {
            return &kv.second;
        }
    }

    return nullptr;
}

bool AnimationDirector::isGLBAnimationPingPong(const std::string& meshfile) const {
    const GLBAnimationControl* control = findGLBAnimationControl(meshfile);
    return control ? control->pingPong : false;
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

float AnimationDirector::getMaxPathDuration() const {
    float maxDur = 0.f;
    for (const auto& [index, anim] : m_pathAnimations) {
        if (anim.duration > maxDur) {
            maxDur = anim.duration;
        }
    }
    return maxDur;
}

void AnimationDirector::setAutoStopTime(float timeSec) {
    m_autoStopTime = timeSec;
    if (m_autoStopTime < 0.f) {
        m_playing = true;
    }
}

bool AnimationDirector::isShapeVisible(size_t shapeIndex) const {
    return m_hiddenShapes.find(shapeIndex) == m_hiddenShapes.end();
}

glm::vec3 AnimationDirector::extractPosition(const glm::mat4& transform) const {
    // translation is stored in the 4th column of the matrix
    return glm::vec3(transform[3]);
}

void AnimationDirector::hideShape(size_t shapeIndex) {
    m_hiddenShapes.insert(shapeIndex);
    // disable path animation if present
    auto pathIt = m_pathAnimations.find(shapeIndex);
    if (pathIt != m_pathAnimations.end()) {
        pathIt->second.enabled = false;
    }

    // disable glb animation for this shape if we know its meshfile
    auto meshIt = m_shapeIndexToMeshfile.find(shapeIndex);
    if (meshIt != m_shapeIndexToMeshfile.end()) {
        auto glbIt = m_glbAnimations.find(meshIt->second);
        if (glbIt != m_glbAnimations.end()) {
            glbIt->second.enabled = false;
        }
    }
}

void AnimationDirector::resetVisibility() {
    m_hiddenShapes.clear();
    // re-enable animations for titan & fish
    if (m_fishIndex != SIZE_MAX) {
        auto fishAnim = m_pathAnimations.find(m_fishIndex);
        if (fishAnim != m_pathAnimations.end()) {
            fishAnim->second.enabled = true;
        }
        if (!m_fishMeshfile.empty()) {
            auto it = m_glbAnimations.find(m_fishMeshfile);
            if (it != m_glbAnimations.end()) {
                it->second.enabled = true;
            }
        }
    }
    if (m_titanIndex != SIZE_MAX) {
        auto titanAnim = m_pathAnimations.find(m_titanIndex);
        if (titanAnim != m_pathAnimations.end()) {
            titanAnim->second.enabled = true;
        }
        if (!m_titanMeshfile.empty()) {
            auto it = m_glbAnimations.find(m_titanMeshfile);
            if (it != m_glbAnimations.end()) {
                it->second.enabled = true;
            }
        }
    }
}

// camera animation implementation
void AnimationDirector::setCameraFollowTarget(size_t targetShapeIndex,
                                             const glm::vec3& offset,
                                             bool followPosition,
                                             bool lookAtTarget) {
    m_cameraFollowTarget = true;
    m_cameraTargetIndex = targetShapeIndex;
    m_cameraOffset = offset;
    m_cameraFollowPosition = followPosition;
    m_cameraLookAtTarget = lookAtTarget;
    m_cameraPathEnabled = false;
    m_cameraAnimatedOffset = false;
    std::cout << "[Animation] Camera set to follow target shape " << targetShapeIndex 
              << " with offset (" << offset.x << ", " << offset.y << ", " << offset.z << ")" << std::endl;
}

void AnimationDirector::setCameraFollowWithAnimatedOffset(size_t targetShapeIndex,
                                                         const glm::vec3& startOffset,
                                                         const glm::vec3& endOffset,
                                                         float transitionStartTime,
                                                         float transitionDuration) {
    m_cameraFollowTarget = true;
    m_cameraTargetIndex = targetShapeIndex;
    m_cameraAnimatedOffset = true;
    m_cameraStartOffset = startOffset;
    m_cameraEndOffset = endOffset;
    m_cameraOffsetStartTime = transitionStartTime;
    m_cameraOffsetDuration = transitionDuration;
    m_cameraFollowPosition = true;
    m_cameraLookAtTarget = true;
    m_cameraPathEnabled = false;
    m_cameraOrbitMode = false;
    std::cout << "[Animation] Camera set to follow target with animated offset: "
              << "start=(" << startOffset.x << ", " << startOffset.y << ", " << startOffset.z << "), "
              << "end=(" << endOffset.x << ", " << endOffset.y << ", " << endOffset.z << "), "
              << "duration=" << transitionDuration << "s" << std::endl;
}

void AnimationDirector::setCameraOrbitTarget(size_t targetShapeIndex,
                                            float radius,
                                            float startAngle,
                                            float endAngle,
                                            float orbitDuration,
                                            const glm::vec3& verticalOffset) {
    m_cameraFollowTarget = true;
    m_cameraTargetIndex = targetShapeIndex;
    m_cameraOrbitMode = true;
    m_cameraOrbitRadius = radius;
    m_cameraOrbitStartAngle = startAngle;
    m_cameraOrbitEndAngle = endAngle;
    m_cameraOrbitDuration = orbitDuration;
    m_cameraOrbitVerticalOffset = verticalOffset;
    m_cameraFollowPosition = true;
    m_cameraLookAtTarget = true;
    m_cameraPathEnabled = false;
    m_cameraAnimatedOffset = false;
    std::cout << "[Animation] Camera set to orbit target: radius=" << radius 
              << ", angle " << startAngle << "° to " << endAngle << "°"
              << ", duration=" << orbitDuration << "s" << std::endl;
}

void AnimationDirector::setCameraPath(const std::vector<PathKeyframe>& keyframes, bool loop) {
    m_cameraPath.keyframes = keyframes;
    m_cameraPath.loop = loop;
    m_cameraPath.enabled = true;
    m_cameraPathEnabled = true;
    m_cameraFollowTarget = false;
    
    if (!keyframes.empty()) {
        m_cameraPath.duration = keyframes.back().time;
    }
    std::cout << "[Animation] Camera path set with " << keyframes.size() 
              << " keyframes, duration=" << m_cameraPath.duration << "s" << std::endl;
}

std::pair<glm::vec3, glm::vec3> AnimationDirector::getCameraTransform() const {
    if (m_cameraPathEnabled && m_cameraPath.enabled && !m_cameraPath.keyframes.empty()) {
        // use camera path animation
        float animTime = getLocalTime(m_currentTime, m_cameraPath);
        
        // find keyframe interval (similar to evaluatePathAnimation)
        if (m_cameraPath.keyframes.size() == 1) {
            const PathKeyframe& kf = m_cameraPath.keyframes[0];
            glm::vec3 pos = kf.position;
            // rotation to look direction (simplified: use rotation as look direction offset)
            glm::vec3 lookDir = glm::normalize(glm::vec3(
                cos(glm::radians(kf.rotation.y)) * cos(glm::radians(kf.rotation.x)),
                sin(glm::radians(kf.rotation.x)),
                sin(glm::radians(kf.rotation.y)) * cos(glm::radians(kf.rotation.x))
            ));
            return {pos, lookDir};
        }
        
        size_t keyframeIndex = 0;
        bool foundInterval = false;
        for (size_t i = 0; i < m_cameraPath.keyframes.size() - 1; ++i) {
            if (animTime >= m_cameraPath.keyframes[i].time && 
                animTime <= m_cameraPath.keyframes[i + 1].time) {
                keyframeIndex = i;
                foundInterval = true;
                break;
            }
        }
        
        if (!foundInterval) {
            // use last keyframe
            const PathKeyframe& kf = m_cameraPath.keyframes.back();
            glm::vec3 pos = kf.position;
            glm::vec3 lookDir = glm::normalize(glm::vec3(
                cos(glm::radians(kf.rotation.y)) * cos(glm::radians(kf.rotation.x)),
                sin(glm::radians(kf.rotation.x)),
                sin(glm::radians(kf.rotation.y)) * cos(glm::radians(kf.rotation.x))
            ));
            return {pos, lookDir};
        }
        
        // interpolate
        const PathKeyframe& kf1 = m_cameraPath.keyframes[keyframeIndex];
        const PathKeyframe& kf2 = m_cameraPath.keyframes[keyframeIndex + 1];
        float t = 0.f;
        if (kf2.time > kf1.time) {
            t = (animTime - kf1.time) / (kf2.time - kf1.time);
            t = std::max(0.f, std::min(1.f, t));
        }
        
        glm::vec3 pos = glm::mix(kf1.position, kf2.position, t);
        glm::vec3 rot = glm::mix(kf1.rotation, kf2.rotation, t);
        glm::vec3 lookDir = glm::normalize(glm::vec3(
            cos(glm::radians(rot.y)) * cos(glm::radians(rot.x)),
            sin(glm::radians(rot.x)),
            sin(glm::radians(rot.y)) * cos(glm::radians(rot.x))
        ));
        return {pos, lookDir};
    } else if (m_cameraWideShot && m_currentTime >= m_cameraWideShotStartTime) {
        // wide shot: fixed position looking at scene
        return {m_cameraWideShotPos, m_cameraWideShotLook};
    } else if (m_cameraFollowTarget && m_cameraTargetIndex != SIZE_MAX) {
        // follow target object
        glm::vec3 targetPos;
        size_t actualTargetIndex = m_cameraTargetIndex;
        bool switchedToTitan = false;
        
        // check if target is still visible
        if (!isShapeVisible(m_cameraTargetIndex)) {
            // target is hidden, switch to titan if available
            if (m_titanIndex != SIZE_MAX && isShapeVisible(m_titanIndex)) {
                // switch to follow titan from the side
                actualTargetIndex = m_titanIndex;
                switchedToTitan = true;
                glm::mat4 titanTransform = evaluatePathAnimation(m_titanIndex, m_currentTime);
                glm::vec3 titanPos = extractPosition(titanTransform);

                float progress = 1.f;
                if (m_cameraSwitchActive && m_cameraSwitchDuration > 0.f && m_cameraSwitchStartTime >= 0.f) {
                    progress = std::clamp((m_currentTime - m_cameraSwitchStartTime) / m_cameraSwitchDuration, 0.f, 1.f);
                }
                // blend target position from last fish position to titan to avoid instant jump
                targetPos = glm::mix(m_cameraLastTargetPos, titanPos, progress);
            } else {
                // no alternative target, use last known position to avoid sudden jump
                if (m_cameraUseLastPos) {
                    targetPos = m_cameraLastTargetPos;
                } else {
                    // first time target is hidden, get last position before hiding
                    auto pathIt = m_pathAnimations.find(m_cameraTargetIndex);
                    if (pathIt != m_pathAnimations.end() && !pathIt->second.keyframes.empty()) {
                        targetPos = pathIt->second.keyframes.back().position;
                    } else {
                        targetPos = m_cameraLastTargetPos;
                    }
                    m_cameraLastTargetPos = targetPos;
                    m_cameraUseLastPos = true;
                }
                actualTargetIndex = m_cameraTargetIndex;  // keep original for offset calculation
            }
        } else {
            // target is visible, get current position
            glm::mat4 targetTransform = evaluatePathAnimation(m_cameraTargetIndex, m_currentTime);
            targetPos = extractPosition(targetTransform);
            m_cameraLastTargetPos = targetPos;  // cache current position
            m_cameraUseLastPos = false;
        }
        
        glm::vec3 offset = m_cameraOffset;
        
        if (m_cameraOrbitMode) {
            // orbit mode: 1/4 circle movement around target
            float currentAngle;
            
            if (switchedToTitan) {
                // switched to titan: smoothly blend from current orbit angle to side view (90°)
                float currentFishAngle = glm::mix(m_cameraOrbitStartAngle, m_cameraOrbitEndAngle,
                                                  m_cameraOrbitDuration > 0.f
                                                      ? std::clamp(m_currentTime / m_cameraOrbitDuration, 0.f, 1.f)
                                                      : 1.f);
                float progress = 1.f;
                if (m_cameraSwitchActive && m_cameraSwitchDuration > 0.f && m_cameraSwitchStartTime >= 0.f) {
                    progress = std::clamp((m_currentTime - m_cameraSwitchStartTime) / m_cameraSwitchDuration, 0.f, 1.f);
                }
                currentAngle = glm::mix(currentFishAngle, 90.f, progress);
            } else {
                // normal orbit for fish
                float t = 0.f;
                if (m_cameraOrbitDuration > 0.f) {
                    t = m_currentTime / m_cameraOrbitDuration;
                    t = std::max(0.f, std::min(1.f, t));  // clamp to [0, 1]
                }
                currentAngle = glm::mix(m_cameraOrbitStartAngle, m_cameraOrbitEndAngle, t);
            }
            
            float angleRad = glm::radians(currentAngle);
            
            // calculate camera position in a circle around target
            // assuming fish moves along X axis, so orbit in XZ plane
            // 0° = front (positive Z), 90° = right side (positive X)
            // for titan: 90° should be from the side (right side of titan)
            float x = m_cameraOrbitRadius * sin(angleRad);
            float z = m_cameraOrbitRadius * cos(angleRad);
            
            // if following titan, adjust offset to be from the side
            // titan is rotated 90° around Y axis in drawMeshPrimitive, so its front faces +X direction
            // for side view, camera should be at titan's right side
            // titan's right side in world space is +Z direction (when titan faces +X)
            if (switchedToTitan) {
                // camera at titan's right side: offset in +Z direction
                // use larger radius for titan to get better side view
                float titanSideRadius = m_cameraOrbitRadius * 1.5f;  // 50% further for better view
                float startRadius = glm::length(glm::vec3(x, 0.f, z));
                float progress = 1.f;
                if (m_cameraSwitchActive && m_cameraSwitchDuration > 0.f && m_cameraSwitchStartTime >= 0.f) {
                    progress = std::clamp((m_currentTime - m_cameraSwitchStartTime) / m_cameraSwitchDuration, 0.f, 1.f);
                }
                float blendedRadius = glm::mix(startRadius, titanSideRadius, progress);
                offset = glm::vec3(0.f, m_cameraOrbitVerticalOffset.y, blendedRadius);
            } else {
                offset = glm::vec3(x, m_cameraOrbitVerticalOffset.y, z) + m_cameraOrbitVerticalOffset;
            }
        } else if (m_cameraAnimatedOffset) {
            // interpolate offset over time
            float t = 0.f;
            if (m_cameraOffsetDuration > 0.f) {
                float elapsed = m_currentTime - m_cameraOffsetStartTime;
                t = elapsed / m_cameraOffsetDuration;
                t = std::max(0.f, std::min(1.f, t));  // clamp to [0, 1]
            }
            offset = glm::mix(m_cameraStartOffset, m_cameraEndOffset, t);
        }
        
        // apply pullback after titan main path ends: increase distance, then lock camera
        if (m_cameraPullbackActive || m_cameraPullbackFinished) {
            float pullT = 1.f;
            if (m_cameraPullbackActive && m_cameraPullbackDuration > 0.f && m_cameraPullbackStartTime >= 0.f) {
                pullT = std::clamp((m_currentTime - m_cameraPullbackStartTime) / m_cameraPullbackDuration, 0.f, 1.f);
            }
            offset += glm::vec3(0.f, 0.f, m_cameraPullbackExtraRadius * pullT);
        }

        glm::vec3 cameraPos = targetPos + offset;
        glm::vec3 lookDir = m_cameraLookAtTarget
            ? glm::normalize(targetPos - cameraPos)
            : glm::normalize(-offset);

        // once pullback finishes, freeze camera
        if (m_cameraPullbackFinished && !m_cameraHoldAfterPullback) {
            m_cameraHoldAfterPullback = true;
            m_cameraHoldPos = cameraPos;
            m_cameraHoldLook = lookDir;
        }
        if (m_cameraHoldAfterPullback) {
            cameraPos = m_cameraHoldPos;
            lookDir = m_cameraHoldLook;
        }
        
        // debug: print camera position when switched to titan
        if (switchedToTitan) {
            static bool printed = false;
            if (!printed) {
                std::cout << "[Animation] Camera switched to titan: pos=(" 
                          << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z 
                          << "), target=(" << targetPos.x << ", " << targetPos.y << ", " << targetPos.z
                          << "), offset=(" << offset.x << ", " << offset.y << ", " << offset.z << ")" << std::endl;
                printed = true;
            }
        }
        
        return {cameraPos, lookDir};
    }
    
    // no camera animation, return default
    return {glm::vec3(0.f, 0.f, 5.f), glm::vec3(0.f, 0.f, -1.f)};
}

bool AnimationDirector::isCameraAnimated() const {
    return m_cameraPathEnabled || m_cameraFollowTarget || m_cameraWideShot;
}
