#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Timer.h"

#include "Renderer.h"
#include "Camera.h"

#include <glm/gtc/type_ptr.hpp>

class RayTracingLayer : public Walnut::Layer {
public:
	RayTracingLayer()
		: m_Camera(45.0f, 0.1f, 100.0f)
	{
		// Materials
		Material& pinkMaterial = m_Scene.Materials.emplace_back();
		pinkMaterial.Albedo = { 1.0f,0.0f, 1.0f };
		pinkMaterial.Roughness = 0.0f;

		Material& blueMaterial = m_Scene.Materials.emplace_back();
		blueMaterial.Albedo = { 0.2f, 0.3f, 1.0f };
		blueMaterial.Roughness = 0.2f;

		Material& orangeMaterial = m_Scene.Materials.emplace_back();
		orangeMaterial.Albedo = { 0.8f, 0.5f, 0.2f };
		orangeMaterial.Roughness = 0.1f;
		orangeMaterial.EmissionColor = orangeMaterial.Albedo;
		orangeMaterial.EmissionPower = 3.65f;

		// Scene objects
		{
			Sphere& sphere = m_Scene.Spheres.emplace_back();
			sphere.Position = { 0.0f, 0.0f, 0.0f };
			sphere.Radius = 1.0f;
			sphere.MaterialIndex = 0;
		}

		{
			Sphere& sphere = m_Scene.Spheres.emplace_back();
			sphere.Position = { 0.0f, -101.0f, 0.0f };
			sphere.Radius = 100.0f;
			sphere.MaterialIndex = 1;
		}

		{
			Sphere& sphere = m_Scene.Spheres.emplace_back();
			sphere.Position = { 9.2f, -1.2f, -9.6f };
			sphere.Radius = 9.2f;
			sphere.MaterialIndex = 2;
		}
	}

	virtual void OnUpdate(f32 ts) override
	{
		// If camera move, stop and reset accumulation
		if (m_Camera.OnUpdate(ts))
		{
			m_Renderer.ResetFrameIndex();
		}

	}
	virtual void OnUIRender() override
	{
		// Scene
		ImGui::Begin("Scene");
		for (size_t i = 0; i < m_Scene.Spheres.size(); ++i)
		{
			ImGui::PushID((int)i);

			Sphere& sphere = m_Scene.Spheres[i];
			ImGui::DragFloat3("Position", glm::value_ptr(sphere.Position), 0.1f);
			ImGui::DragFloat("Radius", &sphere.Radius, 0.1f);
			ImGui::DragInt("Material", &sphere.MaterialIndex, 1.0f, 0, (i32)(m_Scene.Spheres.size() - 1));
			ImGui::Separator();
			ImGui::PopID();
		}

		for (size_t i = 0; i < m_Scene.Materials.size(); ++i)
		{
			ImGui::PushID((int)i);

			Material& material = m_Scene.Materials[i];
			ImGui::ColorEdit3("Albedo", glm::value_ptr(material.Albedo));
			ImGui::DragFloat("Roughness", &material.Roughness, 0.05f, 0.0f, 1.0f);
			ImGui::DragFloat("Metallic", &material.Metallic, 0.05f, 0.0f, 1.0f);

			ImGui::ColorEdit3("Emission Color", glm::value_ptr(material.EmissionColor));
			ImGui::DragFloat("Emission Power", &material.EmissionPower, 0.05f, 0.0f, FLT_MAX);
			ImGui::Separator();

			ImGui::PopID();
		}

		ImGui::End();

		// Settings
		ImGui::Begin("Settings");
		ImGui::Text("Last render: %.3fms", m_LastRenderTime);
		if (ImGui::Button("Render"))
		{
			Render();
		}

		ImGui::Checkbox("Accumulate", &m_Renderer.GetSettings().Accumulate);
		if (ImGui::Button("Reset frame index"))
			m_Renderer.ResetFrameIndex();

		ImGui::End();

		// Viewport
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");
		m_ViewportWidth = (u32)ImGui::GetContentRegionAvail().x;
		m_ViewportHeight = (u32)ImGui::GetContentRegionAvail().y;

		if (auto image = m_Renderer.GetFinalImage())
		{
			ImGui::Image(image->GetDescriptorSet(), { (f32)image->GetWidth(), (f32)image->GetHeight() }, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		}

		ImGui::End();
		ImGui::PopStyleVar();

		Render();
	}
private:
	void Render()
	{
		Walnut::Timer timer;

		m_Renderer.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Camera.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Renderer.Render(m_Scene, m_Camera);

		m_LastRenderTime = timer.ElapsedMillis();
	}
private:
	Camera m_Camera;
	Scene m_Scene;
	Renderer m_Renderer;

	u32 m_ViewportWidth = 0, m_ViewportHeight = 0;
	f32 m_LastRenderTime = 0.0f;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "RayTracing";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<RayTracingLayer>();
	app->SetMenubarCallback([app]()
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit"))
				{
					app->Close();
				}
				ImGui::EndMenu();
			}
		});
	return app;
}