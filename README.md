# CS-330
SNHU CS 330 Comp Graphic and Visualization
# CS-330 3D Camping Scene

A 3D graphics project demonstrating computational graphics and visualization techniques using OpenGL and C++.

## Features

- **Interactive 3D Scene**: Camping environment with multiple objects
- **Realistic Lighting**: Dynamic campfire lighting with point lights and directional moonlight
- **Complex Objects**: 
  - Multi-layered campfire with animated flames
  - Detailed tent with stakes and guy lines
  - Realistic backpack with multiple components
  - Layered pine tree with foliage
  - Glowing moon with special rendering
- **Camera Controls**: First-person navigation with mouse and keyboard
- **Multiple Projections**: Perspective and orthographic viewing modes

## Controls

- **W/S**: Move forward/backward
- **A/D**: Strafe left/right
- **E/Q**: Move up/down
- **Mouse**: Look around
- **Scroll Wheel**: Zoom in/out
- **O/P**: Toggle orthographic/perspective projection

## Technical Implementation

- OpenGL rendering pipeline
- GLM mathematics library
- Custom shader management
- Texture mapping and material properties
- Advanced lighting models
- Modular object construction using helper functions

## Files

- `Source/SceneManager.cpp` - Main scene rendering and object management
- `Source/ViewManager.cpp` - Camera controls and input handling
- `shaders/` - Vertex and fragment shaders
- `textures/` - Image textures for materials
