#pragma once

#include "PrimitiveTypes.h"

#include <glm/glm.hpp>
#include <vector>

struct Material {
	glm::vec3 Albedo{ 1.0f };
	f32 Roughness = 1.0f;
	f32 Metallic = 0.0f;

	glm::vec3 EmissionColor{ 0.0f };
	f32 EmissionPower = 0.0f;

	glm::vec3 GetEmission() const { return EmissionPower * EmissionColor; }
};

struct Sphere {
	glm::vec3 Position{ 0.0f };
	f32 Radius = 0.5f;

	i32 MaterialIndex = 0;
};

struct Scene {
	std::vector<Sphere> Spheres;
	std::vector<Material> Materials;
};