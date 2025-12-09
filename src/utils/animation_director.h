#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
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
    bool pingPong = false;     // ping-pong mode: play forward then backward (smooth looping)
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
                       float speed = 1.0f,  // animation playback speed
                       bool pingPong = false);  // ping-pong mode for smooth looping
    
    // preset scene animations
    // setup default titan and fish animations
    void setupTitanFishAnimation();
    
    // time control
    void update(float deltaSec);
    void setTime(float time);
    void play();
    void pause();
    void reset();
    bool isPlaying() const;
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
    bool isGLBAnimationPingPong(const std::string& meshfile) const;
    
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
    bool isShapeVisible(size_t shapeIndex) const;
    
    // camera animation (cinematic camera movement)
    // set camera to follow a target object with dynamic offset
    void setCameraFollowTarget(size_t targetShapeIndex, 
                               const glm::vec3& offset = glm::vec3(0.f, 0.f, 5.f),
                               bool followPosition = true,
                               bool lookAtTarget = true);
    // set camera to follow target with animated offset (for dramatic pull-back effect)
    void setCameraFollowWithAnimatedOffset(size_t targetShapeIndex,
                                           const glm::vec3& startOffset,
                                           const glm::vec3& endOffset,
                                           float transitionStartTime,
                                           float transitionDuration);
    // set camera to orbit around target (1/4 circle movement + follow translation)
    void setCameraOrbitTarget(size_t targetShapeIndex,
                             float radius,
                             float startAngle,  // start angle in degrees (0 = front, 90 = side)
                             float endAngle,    // end angle in degrees
                             float orbitDuration,
                             const glm::vec3& verticalOffset = glm::vec3(0.f, 0.f, 0.f));
    // set camera path animation
    void setCameraPath(const std::vector<PathKeyframe>& keyframes, bool loop = false);
    // get camera transform (position and look direction)
    // returns: pair of (position, lookDirection)
    std::pair<glm::vec3, glm::vec3> getCameraTransform() const;
    bool isCameraAnimated() const;
    
private:
    // internal implementation
    glm::mat4 evaluatePathAnimation(size_t shapeIndex, float time) const;
    float getLocalTime(float globalTime, const PathAnimation& anim) const;
    glm::vec3 extractPosition(const glm::mat4& transform) const;
    void hideShape(size_t shapeIndex);
    void resetVisibility();
    
    // data storage
    std::unordered_map<size_t, PathAnimation> m_pathAnimations;
    std::unordered_map<std::string, GLBAnimationControl> m_glbAnimations;
    std::unordered_map<std::string, size_t> m_meshfileToShapeIndex;  // mapping table
    std::unordered_map<size_t, std::string> m_shapeIndexToMeshfile;
    std::unordered_map<std::string, float> m_modelScales;  // model scale multipliers
    std::unordered_set<size_t> m_hiddenShapes;  // shapes to skip rendering

    // cached indices for titan & fish
    size_t m_titanIndex = SIZE_MAX;
    size_t m_fishIndex = SIZE_MAX;
    std::string m_titanMeshfile;
    std::string m_fishMeshfile;
    
    float m_currentTime = 0.f;
    bool m_playing = true;
    float m_autoStopTime = -1.f;  // >=0 means stop when time reaches this
    const RenderData* m_renderData = nullptr;
    
    // camera animation
    bool m_cameraFollowTarget = false;
    size_t m_cameraTargetIndex = SIZE_MAX;
    glm::vec3 m_cameraOffset = glm::vec3(0.f, 0.f, 5.f);
    bool m_cameraFollowPosition = true;
    bool m_cameraLookAtTarget = true;
    PathAnimation m_cameraPath;
    bool m_cameraPathEnabled = false;
    
    // animated offset for dramatic camera movement
    bool m_cameraAnimatedOffset = false;
    glm::vec3 m_cameraStartOffset = glm::vec3(0.f, 0.f, 5.f);
    glm::vec3 m_cameraEndOffset = glm::vec3(0.f, 0.f, 5.f);
    float m_cameraOffsetStartTime = 0.f;
    float m_cameraOffsetDuration = 0.f;
    
    // camera cut to wide shot
    bool m_cameraWideShot = false;
    glm::vec3 m_cameraWideShotPos = glm::vec3(0.f, 0.f, 5.f);
    glm::vec3 m_cameraWideShotLook = glm::vec3(0.f, 0.f, -1.f);
    float m_cameraWideShotStartTime = 0.f;
    
    // camera orbit mode
    bool m_cameraOrbitMode = false;
    float m_cameraOrbitRadius = 5.f;
    float m_cameraOrbitStartAngle = 0.f;
    float m_cameraOrbitEndAngle = 90.f;
    float m_cameraOrbitDuration = 10.f;
    glm::vec3 m_cameraOrbitVerticalOffset = glm::vec3(0.f, 0.f, 0.f);
    
    // cached last position when target is hidden (mutable for caching in const method)
    mutable glm::vec3 m_cameraLastTargetPos = glm::vec3(0.f, 0.f, 0.f);
    mutable bool m_cameraUseLastPos = false;
};
