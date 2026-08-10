# Depth Peeling

Simple depth-peeling in OpenGL with max 8 passes.

The default scene contains a wooden floor and a city model on top of the floor.
The city is translucent.

### Usage

**NOTE: Set the working directory to the repo root before running.**

- LMB + mouse move - moving camera around the city
- scrolling - zooming in/out with camera
- R - toggle on/off camera rotation
- Key Up - increment the peeled layers depth (max 8)
- Key Down - decrement the peeled layers depth (min 1)
- Enter - reset camera position

The current depth is printed to the console on every change.



### The code

My code is the main.cpp, renderer.hpp, scene_definition.hpp and the shaders in the shaders/ folder.

All the utilities (utils/, stb/, glad/ folders as well as the CmakeLists.txt) are taken from the repository https://github.com/JanKolomaznik/gl_tutorials.
Minor adjustments were done to allow scrolling and reading of the city object file.
