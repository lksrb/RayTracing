#pragma once

#include "PrimitiveTypes.h"
#include "Camera.h"
#include "Ray.h"
#include "Scene.h"

#include "Walnut/Image.h"

#include <memory>
#include <glm/glm.hpp>

class Renderer {
public:
	struct Settings {
		bool Accumulate = true;
	};

	Renderer();
	~Renderer();

	void Render(const Scene& scene, const Camera& camera);

	void OnResize(u32 width, u32 height);

	std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }

	void ResetFrameIndex() { m_FrameIndex = 1; }
	Settings& GetSettings() { return m_Settings; }
private:
	struct HitPayload {
		f32 HitDistance;
		glm::vec3 WorldPosition;
		glm::vec3 WorldNormal;

		i32 ObjectIndex;
	};

	glm::vec4 PerPixel(u32 x, u32 y); // RayGen

	HitPayload TraceRay(const Ray& ray);
	HitPayload ClosestHit(const Ray& ray, f32 hitDistance, i32 objectIndex);
	HitPayload Miss(const Ray& ray);
private:
	const Scene* m_ActiveScene = nullptr;
	const Camera* m_ActiveCamera = nullptr;

	std::vector<u32> m_ImageHorizontalIter, m_ImageVerticalIter;

	u32* m_ImageData = nullptr;
	glm::vec4* m_AccumulationData = nullptr;
	std::shared_ptr<Walnut::Image> m_FinalImage;

	Settings m_Settings;

	u32 m_FrameIndex = 1;
};