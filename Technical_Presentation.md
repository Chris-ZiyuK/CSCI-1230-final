# Technical Presentation (Captions for Demo)

## 1. Kinematic Skeletons

Kinematic skeletons are hierarchical structures of connected joints that define character animation.

Each joint transforms relative to its parent, creating natural motion like rotating the shoulder moves the entire arm.

Instead of animating every vertex, we animate only the skeleton joints for computational efficiency.

We load skeletal data from GLB files and interpolate keyframes to produce smooth animation.

---

## 2. Linear Blend Skinning

Linear blend skinning deforms the mesh surface based on skeleton transformations.

Each vertex is influenced by up to four bones with weighted contributions.

The final vertex position is computed as a weighted average of bone transformations.

This computation happens in the vertex shader, enabling real-time smooth mesh deformation.

---

## 3. Camera Path

Camera path animation defines predefined trajectories for the virtual camera over time.

Each path consists of keyframes specifying position, look-at target, and orientation.

Between keyframes, we interpolate values to create smooth, continuous camera motion.

This enables complex cinematic movements like orbiting, tracking, and viewpoint transitions.

---

## 4. Normal Mapping

Normal mapping simulates fine surface detail without adding geometric complexity.

The normal map stores surface normal vectors as RGB texture values.

In the fragment shader, we sample and transform these normals for lighting calculations.

This creates detailed lighting effects efficiently, enhancing visual richness at minimal performance cost.

---

## 5. Post-Processing Pipeline

Post-processing applies visual effects to the rendered scene after initial rendering.

We render the scene to a framebuffer object instead of directly to the screen.

Multiple processing passes transform the rendered texture: bright-pass, blur, and combination.

This modular approach enables flexible visual effects without modifying core rendering code.

---

## 6. Screen Space Bloom

Bloom creates a glowing halo effect around bright areas of the scene.

A bright-pass extracts pixels exceeding a brightness threshold from the scene texture.

We apply Gaussian blur using ping-pong framebuffers for smooth light diffusion.

The blurred result is combined with the original scene using adjustable strength blending.

---

## 7. Scrolling Textures

Scrolling textures create animated movement in texture coordinates over time.

We offset UV coordinates using a time-based scroll value passed as a uniform.

The scroll offset is wrapped using modulo operations to create seamless looping.

This enables dynamic backgrounds and animated surface details without vertex animation.

---

## 8. Screen Space Motion Blur

Screen-space motion blur simulates motion trails based on camera and object movement.

We compute motion vectors using G-buffer depth and view-projection matrix differences.

The blur samples along the motion direction with depth-aware weighting to prevent bleeding.

This creates realistic motion blur effects entirely in screen space after rendering.

---

## 9. G-Buffers (Generic)

G-buffers store intermediate rendering data in separate texture attachments.

Our implementation uses color and depth textures stored in framebuffer objects.

The depth buffer enables reconstruction of world positions via inverse view-projection matrices.

G-buffers provide essential data for post-processing effects like motion blur and depth of field.
