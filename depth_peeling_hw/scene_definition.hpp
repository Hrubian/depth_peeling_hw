#pragma once

#include <memory>
#include <vector>
#include <ranges>
#include <cmath>

#include "scene_object.hpp"
#include "material_factory.hpp"
#include "geometry_factory.hpp"
#include "simple_scene.hpp"
#include "mesh_object.hpp"
#include "ogl_geometry_factory.hpp"
#include "ogl_material_factory.hpp"

inline SimpleScene createCityScene(
	OGLMaterialFactory &aMaterialFactory,
	OGLGeometryFactory &aGeometryFactory)
{
	SimpleScene scene;
	
	// FLOOR (Wood texture, opaque)
	{
		auto floor = std::make_shared<LoadedMeshObject>("./depth_peeling_hw/data/geometry/floor.obj");
		floor->setPosition(glm::vec3(0.0f, -0.1f, 0.0f));
		floor->setScale(glm::vec3(15.0f, 0.1f, 15.0f));
		
		MaterialParameterValues floorParams;
		floorParams["u_diffuseColor"] = glm::vec3(0.6f, 0.4f, 0.2f); // Wood color fallback
		floorParams["u_specularColor"] = glm::vec3(0.1f, 0.1f, 0.1f);
		floorParams["u_shininess"] = 5.0f;
		floorParams["u_alpha"] = 1.0f; // Fully opaque
		floorParams["u_useTexture"] = true;
		floorParams["u_diffuseTexture"] = TextureInfo("wood.png");
		floorParams["u_texScale"] = glm::vec2(15.0f, 15.0f);
		
		floor->addMaterial(
			"solid",
			MaterialParameters(
				"blinn_phong",
				RenderStyle::Solid,
				floorParams
			)
		);
		floor->prepareRenderData(aMaterialFactory, aGeometryFactory);
		scene.addObject(floor);
	}
	
	// CITY MODEL (translucent opacity 0.65 for depth peeling)
	// ========================================================================
	{
		auto city = std::make_shared<LoadedMeshObject>("./depth_peeling_hw/data/geometry/city.obj");
		city->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
		city->setScale(glm::vec3(0.001f, 0.001f, 0.001f)); // the city is a bit huge... :/
		
		MaterialParameterValues cityParams;
		cityParams["u_diffuseColor"] = glm::vec3(0.75f, 0.85f, 1.0f); // Light blue glass buildings
		cityParams["u_specularColor"] = glm::vec3(0.8f, 0.8f, 0.8f);
		cityParams["u_shininess"] = 40.0f;
		cityParams["u_alpha"] = 0.65f; // Translucent glass material
		cityParams["u_useTexture"] = false;
		
		city->addMaterial(
			"solid",
			MaterialParameters(
				"blinn_phong",
				RenderStyle::Solid,
				cityParams
			)
		);
		city->prepareRenderData(aMaterialFactory, aGeometryFactory);
		scene.addObject(city);
	}
	
	return scene;
}
