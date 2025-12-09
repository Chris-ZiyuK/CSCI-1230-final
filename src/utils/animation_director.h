#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>
#include "sceneparser.h"

// path animation keyframe
struct PathKeyframe {
    float time;              // time point in seconds
    glm::vec3 position;      // position
    glm::vec3 rotation;      // rotation (euler angles in degrees)
    glm::vec3 scale;         // scale
};

// path animation data
struct PathAnimation {
    std::vector<PathKeyframe> keyframes;
    bool loop = false;
    float duration = 0.f;
    bool enabled = true;
    glm::mat4 baseTransform = glm::mat4(1.0f);  // original ctm from scene
};

// glb skeletal animation control
struct GLBAnimationControl {
    float startTime = 0.f;      // animation start time
    float duration = 0.f;       // animation duration (0 means loop until end)
    int animationIndex = 0;     // animation index in glb file
    bool loop = true;           // whether to loop
    bool enabled = true;
    bool ignoreRootTranslation = false;  // ignore translation on root joint (for path animation)
    float speed = 1.0f;         // animation playback speed (1.0 = normal, 0.5 = half speed, 2.0 = double speed)
};

// animation director: manages all animations (path and skeletal)
class AnimationDirector {
public:
    // initialization
    void initialize(const RenderData& renderData);
    
    // path animation management
    // add path animation by meshfile identifier
    void addPathAnimation(const std::string& meshfile, 
                         const std::vector<PathKeyframe>& keyframes, 
                         bool loop = false);
    // add path animation by shape index
    void addPathAnimation(size_t shapeIndex,
                         const std::vector<PathKeyframe>& keyframes,
                         bool loop = false);
    
    // glb skeletal animation management
    // set animation timeline for glb model
    void setGLBAnimation(const std::string& meshfile,
                       float startTime,
                       float duration = 0.f,  // 0 means loop
                       int animIndex = 0,
                       bool loop = true,
                       bool ignoreRootTranslation = false,  // ignore root joint translation
                       float speed = 1.0f);  // animation playback speed
    
    // preset scene animations
    // setup default titan and fish animations
    void setupTitanFishAnimation();
    
    // time control
    void update(float deltaSec);
    void setTime(float time);
    void play();
    void pause();
    void reset();
    float getCurrentTime() const;
    
    // query interface
    // get transform matrix for object at current time
    glm::mat4 getTransform(size_t shapeIndex) const;
    glm::mat4 getTransform(const std::string& meshfile) const;
    
    // get glb animation time for updateAnimation call
    float getGLBAnimationTime(const std::string& meshfile) const;
    int getGLBAnimationIndex(const std::string& meshfile) const;
    bool isGLBAnimationActive(const std::string& meshfile) const;
    bool shouldIgnoreRootTranslation(const std::string& meshfile) const;
    
    // debug interface
    void printAnimationInfo() const;
    std::vector<PathKeyframe> getPathKeyframes(size_t shapeIndex) const;
    
    // model scale management
    // get model scale multiplier (for unified scaling)
    float getModelScale(const std::string& meshfile) const;
    void setModelScale(const std::string& meshfile, float scale);
    
    // playback helpers
    float getMaxPathDuration() const;
    void setAutoStopTime(float timeSec);
    
private:
    // internal implementation
    glm::mat4 evaluatePathAnimation(size_t shapeIndex, float time) const;
    float getLocalTime(float globalTime, const PathAnimation& anim) const;
    
    // data storage
    std::unordered_map<size_t, PathAnimation> m_pathAnimations;
    std::unordered_map<std::string, GLBAnimationControl> m_glbAnimations;
    std::unordered_map<std::string, size_t> m_meshfileToShapeIndex;  // mapping table
    std::unordered_map<std::string, float> m_modelScales;  // model scale multipliers
    
    float m_currentTime = 0.f;
    bool m_playing = true;
    float m_autoStopTime = -1.f;  // >=0 means stop when time reaches this
    const RenderData* m_renderData = nullptr;
};
