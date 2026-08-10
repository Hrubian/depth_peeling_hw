#include <iostream>
#include <algorithm>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <array>

#include "ogl_resource.hpp"
#include "error_handling.hpp"
#include "window.hpp"
#include "shader.hpp"

#include "scene_definition.hpp"
#include "renderer.hpp"

#include "ogl_geometry_factory.hpp"
#include "ogl_material_factory.hpp"

#include "camera.hpp"
#include "simple_scene.hpp"
#include "spotlight.hpp"

struct Config {
	int numPeelingPasses = 4;
	bool rotateScene = false;
};

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

	try {
		auto window = Window();
		MouseTracking mouseTracking;
		Config config;
		
		// Camera setup - positioned to view the scene from above and slightly to the side
		Camera camera(window.aspectRatio());
		camera.setPosition(glm::vec3(20.0f, 20.0f, 30.0f));
		camera.lookAt(glm::vec3(0.0f, 0.5f, 0.0f));
		
		// Light setup - positioned to illuminate the scene
		SpotLight light;
		light.setPosition(glm::vec3(5.0f, 10.0f, 5.0f));
		light.lookAt(glm::vec3(0.0f, 0.0f, 0.0f));

		// Input handling for mouse orbit
		window.onCheckInput([&camera, &mouseTracking](GLFWwindow *aWin) {
				mouseTracking.update(aWin);
				if (glfwGetMouseButton(aWin, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
					camera.orbit(-0.4f * mouseTracking.offset(), glm::vec3(0.0f, 1.0f, 0.0f));
				}
			});

		// Mouse wheel scroll zoom
		window.setScrollCallback([&camera](GLFWwindow *aWin, double xoffset, double yoffset) {
				const glm::vec3 target(0.0f, 0.5f, 0.0f);
				glm::vec3 dir = camera.getPosition() - target;
				float distance = glm::length(dir);
				if (distance > 0.001f) {
					// Zoom in (scroll up, yoffset > 0) / Zoom out (scroll down, yoffset < 0)
					distance = std::clamp(distance - static_cast<float>(yoffset) * 0.5f, 1.0f, 60.0f);
					camera.setPosition(target + glm::normalize(dir) * distance);
					camera.lookAt(target);
				}
			});
		
		// Keyboard shortcuts
		window.setKeyCallback([&config, &camera](GLFWwindow *aWin, int key, int scancode, int action, int mods) {
				if (action == GLFW_PRESS) {
					switch (key) {
					case GLFW_KEY_ENTER:
						camera.setPosition(glm::vec3(20.0f, 20.0f, 30.0f));
						camera.lookAt(glm::vec3(0.0f, 0.5f, 0.0f));
						break;
					case GLFW_KEY_UP:
						config.numPeelingPasses = std::min(config.numPeelingPasses + 1, 8);
						std::cout << "Peeling passes: " << config.numPeelingPasses << std::endl;
						break;
					case GLFW_KEY_DOWN:
						config.numPeelingPasses = std::max(1, config.numPeelingPasses - 1);
						std::cout << "Peeling passes: " << config.numPeelingPasses << std::endl;
						break;
					case GLFW_KEY_R:
						config.rotateScene = !config.rotateScene;
						std::cout << "Scene rotation: " << (config.rotateScene ? "ON" : "OFF") << std::endl;
						break;
					}
				}
			});

		OGLMaterialFactory materialFactory;
		materialFactory.loadShadersFromDir("./depth_peeling_hw/shaders/");
		materialFactory.loadTexturesFromDir("./depth_peeling_hw/data/textures/");

		OGLGeometryFactory geometryFactory;

		SimpleScene cityScene = createCityScene(materialFactory, geometryFactory);
		Renderer renderer(materialFactory, config.numPeelingPasses);
		
		window.onResize([&camera, &window, &renderer](int width, int height) {
				camera.setAspectRatio(window.aspectRatio());
				renderer.initialize(width, height);
			});

		renderer.initialize(window.size()[0], window.size()[1]);
		
		window.runLoop([&] {
			if (config.rotateScene) {
				camera.orbit(glm::vec2(0.2f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			}
			
			// Enable depth test and clear background
			GL_CHECK(glEnable(GL_DEPTH_TEST));
			GL_CHECK(glClearColor(0.1f, 0.1f, 0.15f, 1.0f));
			GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

			// Set current number of peeling passes
			renderer.setNumPeelingPasses(config.numPeelingPasses);
			
			// Render the scene with depth peeling
			renderer.render(cityScene, camera, light, RenderOptions{"solid"});

		});
	} catch (ShaderCompilationError &exc) {
		std::cerr
			<< "Shader compilation error!\n"
			<< "Shader type: " << exc.shaderTypeName()
			<<"\nError: " << exc.what() << "\n";
		return -3;
	} catch (OpenGLError &exc) {
		std::cerr << "OpenGL error: " << exc.what() << "\n";
		return -2;
	} catch (std::exception &exc) {
		std::cerr << "Error: " << exc.what() << "\n";
		return -1;
	}
    glfwTerminate();
    return 0;
}
