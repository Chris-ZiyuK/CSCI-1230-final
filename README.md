# CSCI1230: Final Project 

### Team Ritchie
- Betty Qianyun Gong (qgong9)
- Chris Ziyu Kong (zkong10)

### Introduction
We built a starfield packed with glowing stars against a nebula backdrop, pairing a massive Titan with a small fish and animating the Titan’s chase and flight after devouring its prey—rendering a scene that feels both sci‑fi and mysterious. Camera movement and dynamic motion trails amplify the sense of motion and elevate the visuals. Adjustable bloom intensity and starfield scroll speed add interactivity and a dreamlike quality, letting users sink into an immersive deep‑sea fantasy.


https://github.com/user-attachments/assets/d1ee3ea3-5375-4265-9a09-59c75bc075f1



### Features Inplemented
1. Kinematic Skeletons
2. Linear Blend Skinning
4. Normal Mapping 
5. Post-Processing Pipeline 
6. Screen Space Bloom 
7. Scrolling Textures
8. **Camera Path - [NEW FEATURE: 40 points]**
9. **G-Buffers (Generic) - [NEW FEATURE: 20 points]**
10. **Screen Space Motion Blur - [NEW FEATURE: 40 points]**

Details of features we inplemented can be found here: [Technical Features](Technical_Presentation.md)

### User Instructions
1. Build and launch the app (Qt UI opens with the render view on the right, controls on the left).


https://github.com/user-attachments/assets/f85a4623-dd4e-445b-8d1d-d49cc22fa855


2. Click “Upload Scene File” to load a custom scene; otherwise the default starfield + Titan + fish scene runs. 
3. **Scene Files Overview:** (Path: scenefiles/realtime/required/..)
   - **combined.json:** Titan and fish fly toward each other and play the “eat the fish” animation.
   

https://github.com/user-attachments/assets/427e23e2-df00-4d7c-a24c-7ec5ca284b22


   - **alien_fish.json:** A fish swims in place at the center of the starfield.
   

https://github.com/user-attachments/assets/7e0e091c-2dc0-4e42-967c-901d6ca81602


   - **desert_titan.json:** The Titan flies at the center of the starfield.
   

https://github.com/user-attachments/assets/06d69094-2a5e-4871-81f3-03c672677d8b


   - **test_glb.json:** A whale swims at the center of the starfield (test scene).


https://github.com/user-attachments/assets/a380b667-caa1-4d40-8e48-daebe0523b5b


4. **Core Controls** (left panel)
   - **Play Animation:** starts/stops the Titan–fish animation (Titan will eventually eat the fish, after which motion blur trails appear).
   - **Bloom Strength:** slider/spinbox to increase/decrease glow and highlights.
   - **Starfield Scroll Speed:** slider/spinbox to adjust background star/nebula drift.
### Third-party Libraries We Used
- TinyGLTF (GLB/GLTF loading), which internally bundles stb_image / stb_image_write and JSON parsing.

### Known Bugs
None.
