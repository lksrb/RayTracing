#include "Renderer.h"

#include "Walnut/Image.h"

#include <execution>

namespace Utils {

	static u32 ConvertToUInt(const glm::vec4& color)
	{
		u8 r = (u8)(color.r * 255.0f);
		u8 g = (u8)(color.g * 255.0f);
		u8 b = (u8)(color.b * 255.0f);
		u8 a = (u8)(color.a * 255.0f);
		return (a << 24) | (b << 16) | (g << 8) | r;
	}

	u32 PCG_Hash(u32 input)
	{
		u32 state = input * 747796405u + 2891336453u;
		u32 word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
		return (word >> 22u) ^ word;
	}

	static f32 RandomFloat(u32& seed)
	{
		seed = PCG_Hash(seed);

		constexpr f32 scale = 1.0f / std::numeric_limits<u32>::max();
		return (f32)seed * scale;
	}

	static glm::vec3 InUnitSphere(u32& seed)
	{
		return glm::normalize(glm::vec3(
			RandomFloat(seed) * 2.0f - 1.0f, 
			RandomFloat(seed) * 2.0f - 1.0f, 
			RandomFloat(seed) * 2.0f - 1.0f)
		);
	}
}

void Renderer::OnResize(u32 width, u32 height)
{
	if (m_FinalImage)
	{
		if (width == m_FinalImage->GetWidth() && height == m_FinalImage->GetHeight())
			return;

		m_FinalImage->Resize(width, height);
	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_ImageData;
	m_ImageData = new u32[width * height];

	delete[] m_AccumulationData;
	m_AccumulationData = new glm::vec4[width * height];

	m_ImageHorizontalIter.resize(width);
	m_ImageVerticalIter.resize(height);

	for (u32 i = 0; i < width; i++)
		m_ImageHorizontalIter[i] = i;

	for (u32 i = 0; i < height; i++)
		m_ImageVerticalIter[i] = i;
}

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
	delete[] m_ImageData;
	m_ImageData = nullptr;

	delete[] m_AccumulationData;
	m_AccumulationData = nullptr;
}

void Renderer::Render(const Scene& scene, const Camera& camera)
{
	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;

	if (m_FrameIndex == 1)
		memset(m_AccumulationData, 0, m_FinalImage->GetWidth() * m_FinalImage->GetHeight() * sizeof(glm::vec4));

	// ~2m -> 1920x1080
	std::for_each(std::execution::par, m_ImageVerticalIter.begin(), m_ImageVerticalIter.end(), [this](u32 y)
	{
		std::for_each(std::execution::par, m_ImageHorizontalIter.begin(), m_ImageHorizontalIter.end(), [this, y](u32 x)
		{
			glm::vec4 color = PerPixel(x, y);
			m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;

			glm::vec4 accumulatedColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
			accumulatedColor /= (f32)m_FrameIndex;

			accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToUInt(accumulatedColor);
		});
	});

	m_FinalImage->SetData(m_ImageData);

	if (m_Settings.Accumulate)
		++m_FrameIndex;
	else
		m_FrameIndex = 1;
}

glm::vec4 Renderer::PerPixel(u32 x, u32 y)
{
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];

	glm::vec3 light(0.0f);
	glm::vec3 contribution(1.0f);

	u32 seed = x + y * m_FinalImage->GetWidth();
	seed *= m_FrameIndex;

	constexpr u32 bounces = 10;
	for (u32 i = 0; i < bounces; ++i)
	{
		// Per bounce
		seed += i;

		Renderer::HitPayload payload = TraceRay(ray);
		if (payload.HitDistance < 0.0f)
		{
			glm::vec3 skyColor = glm::vec3(0.6f, 0.7f, 0.9f);
			//light += skyColor * contribution;
			break;
		}

		const Sphere& sphere = m_ActiveScene->Spheres[payload.ObjectIndex];
		const Material& material = m_ActiveScene->Materials[sphere.MaterialIndex];

		contribution *= material.Albedo;
		light += material.GetEmission();

		ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.0001f;
		//ray.Direction = glm::reflect(ray.Direction,
		//	payload.WorldNormal + material.Roughness * Walnut::Random::Vec3(-0.5f, 0.5f));
		ray.Direction = glm::normalize(payload.WorldNormal + Utils::InUnitSphere(seed));
	}

	return glm::vec4(light, 1.0f);

}

Renderer::HitPayload Renderer::TraceRay(const Ray& ray)
{
	i32 closestSphere = -1;
	f32 hitDistance = std::numeric_limits<f32>::max();

	for (size_t i = 0; i < m_ActiveScene->Spheres.size(); ++i)
	{
		const Sphere& sphere = m_ActiveScene->Spheres[i];
		glm::vec3 origin = ray.Origin - sphere.Position;

		f32 a = glm::dot(ray.Direction, ray.Direction);
		f32 b = 2.0f * glm::dot(origin, ray.Direction);
		f32 c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		// Discriminant
		f32 discriminant = b * b - 4.0f * a * c;

		// If no hits, skip
		if (discriminant < 0.0f)
			continue;

		// float t0 = (-b + glm::sqrt(discriminant)) / (2.0f * a); // Currently not used
		f32 closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);

		if (closestT > 0.0f && closestT < hitDistance)
		{
			hitDistance = closestT;
			closestSphere = (i32)i;
		}
	}

	if (closestSphere < 0)
		return Miss(ray);

	return ClosestHit(ray, hitDistance, closestSphere);
}

Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, f32 hitDistance, i32 objectIndex)
{
	Renderer::HitPayload payload;
	payload.HitDistance = hitDistance;
	payload.ObjectIndex = objectIndex;

	const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];

	glm::vec3 origin = ray.Origin - closestSphere.Position;
	payload.WorldPosition = origin + ray.Direction * hitDistance;
	payload.WorldNormal = glm::normalize(payload.WorldPosition);

	payload.WorldPosition += closestSphere.Position;
	return payload;
}

Renderer::HitPayload Renderer::Miss(const Ray& ray)
{
	Renderer::HitPayload payload;
	payload.HitDistance = -1.0f;
	return payload;
}
