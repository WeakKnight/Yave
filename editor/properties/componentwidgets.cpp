/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/

#include "componentwidgets.h"

#include <editor/context/EditorContext.h>

#include <editor/components/EditorComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/SkyLightComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/TransformableComponent.h>

#include <yave/utils/color.h>
#include <editor/utils/ui.h>
#include <editor/widgets/AssetSelector.h>

#include <imgui/yave_imgui.h>

#include <mutex>

namespace editor {
namespace detail {

static ComponentWidgetData* first_component = nullptr;

void register_component_widget(ComponentWidgetData* data) {
	data->next = first_component;
	first_component = data;
}

}

void draw_component_widgets(ContextPtr ctx, ecs::EntityId id) {
	usize index = 0;
	for(detail::ComponentWidgetData* data = detail::first_component; data; data = data->next) {
		y_debug_assert(data->func);

		ImGui::PushID(fmt_c_str("wid %", index++));
		data->func(ctx, id);
		ImGui::PopID();
	}
}





/**************************************************************************
*								Lights
**************************************************************************/

template<typename T>
static void light_widget(T* light) {
	const int color_flags = ImGuiColorEditFlags_NoSidePreview |
					  // ImGuiColorEditFlags_NoSmallPreview |
					  ImGuiColorEditFlags_NoAlpha |
					  ImGuiColorEditFlags_Float |
					  ImGuiColorEditFlags_InputRGB;

	const math::Vec4 color(light->color(), 1.0f);


	ImGui::Text("Light color");
	ImGui::NextColumn();
	if(ImGui::ColorButton("Color", color, color_flags)) {
		ImGui::OpenPopup("Color");
	}

	if(ImGui::BeginPopup("Color")) {
		ImGui::ColorPicker3("##lightpicker", light->color().begin(), color_flags);

		float kelvin = std::clamp(rgb_to_k(light->color()), 1000.0f, 12000.0f);
		if(ImGui::SliderFloat("##temperature", &kelvin, 1000.0f, 12000.0f, "%.0f°K")) {
			light->color() = k_to_rbg(kelvin);
		}

		ImGui::EndPopup();
	}


	ImGui::NextColumn();
	ImGui::Text("Intensity");
	ImGui::NextColumn();
	ImGui::DragFloat("##intensity", &light->intensity(), 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.2f");
}

editor_widget_draw_func(ContextPtr ctx, ecs::EntityId id) {
	PointLightComponent* light = ctx->world().component<PointLightComponent>(id);
	if(!light) {
		return;
	}

	if(!ImGui::CollapsingHeader("Point light", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}


	ImGui::Columns(2);
	{
		light_widget(light);

		ImGui::NextColumn();
		ImGui::Text("Radius");
		ImGui::NextColumn();
		ImGui::DragFloat("##radius", &light->radius(), 1.0f, 0.0f, std::numeric_limits<float>::max(), "%.2f");

		ImGui::NextColumn();
		ImGui::Text("Falloff");
		ImGui::NextColumn();
		ImGui::DragFloat("##falloff", &light->falloff(), 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.2f", 2.0f);
	}
	ImGui::Columns(1);
}

editor_widget_draw_func(ContextPtr ctx, ecs::EntityId id) {
	SpotLightComponent* light = ctx->world().component<SpotLightComponent>(id);
	if(!light) {
		return;
	}

	if(!ImGui::CollapsingHeader("Spot light", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}


	ImGui::Columns(2);
	{
		light_widget(light);

		ImGui::NextColumn();
		ImGui::Text("Radius");
		ImGui::NextColumn();
		ImGui::DragFloat("##radius", &light->radius(), 1.0f, 0.0f, std::numeric_limits<float>::max(), "%.2f");

		ImGui::NextColumn();
		ImGui::Text("Falloff");
		ImGui::NextColumn();
		ImGui::DragFloat("##falloff", &light->falloff(), 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.2f", 2.0f);

		ImGui::NextColumn();
		ImGui::Text("Angle");
		ImGui::NextColumn();
		float angle = math::to_deg(light->half_angle() * 2.0f);
		if(ImGui::DragFloat("##angle", &angle, 0.1f, 0.0f, 360.0f, "%.2f°")) {
			light->half_angle() = math::to_rad(angle * 0.5f);
		}

		ImGui::NextColumn();
		ImGui::Text("Exponent");
		ImGui::NextColumn();
		ImGui::DragFloat("##exponent", &light->angle_exponent(), 0.1f, 0.0f, 10.0f, "%.2f");


		ImGui::NextColumn();
		ImGui::Text("Cast shadows");
		ImGui::NextColumn();
		ImGui::Checkbox("##shadows", &light->cast_shadow());


		ImGui::NextColumn();
		ImGui::Text("Shadow LoD");
		ImGui::NextColumn();

		int lod = light->shadow_lod();
		if(ImGui::DragInt("LoD", &lod, 1.0f / 8.0f, 0, 8)) {
			light->shadow_lod() = lod;
		}

		/*if(light->cast_shadow()) {
			ImGui::NextColumn();
			ImGui::Text("Shadow bias");
			ImGui::NextColumn();
			ImGui::InputFloat("C##bias", &light->depth_bias().x());
			ImGui::InputFloat("S##bias", &light->depth_bias().y());
		}*/
	}
	ImGui::Columns(1);
}

editor_widget_draw_func(ContextPtr ctx, ecs::EntityId id) {
	DirectionalLightComponent* light = ctx->world().component<DirectionalLightComponent>(id);
	if(!light) {
		return;
	}

	if(!ImGui::CollapsingHeader("Directional light", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}


	ImGui::Columns(2);
	{
		light_widget(light);

		ImGui::NextColumn();
		ImGui::Text("Direction");
		ImGui::NextColumn();
		ImGui::InputFloat3("##direction", light->direction().data(), "%.2f");
	}
	ImGui::Columns(1);
}


editor_widget_draw_func(ContextPtr ctx, ecs::EntityId id) {
	SkyLightComponent* sky = ctx->world().component<SkyLightComponent>(id);
	if(!sky) {
		return;
	}

	if(!ImGui::CollapsingHeader("Sky", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	ImGui::Columns(2);
	{
		ImGui::Text("Envmap");
		ImGui::NextColumn();
		bool clear = false;
		if(imgui::asset_selector(ctx, sky->probe().id(), AssetType::Image, "Envmap", &clear)) {
			ctx->ui().add<AssetSelector>(AssetType::Image)->set_selected_callback(
				[=](AssetId asset) {
					if(const auto probe = ctx->loader().load_res<IBLProbe>(asset)) {
						if(SkyLightComponent* sky = ctx->world().component<SkyLightComponent>(id)) {
							sky->probe() = probe.unwrap();
						}
					}
					return true;
				});
		} else if(clear) {
			sky->probe() = AssetPtr<IBLProbe>();
		}
	}
	ImGui::Columns(1);
}


/**************************************************************************
*								Meshes
**************************************************************************/

editor_widget_draw_func(ContextPtr ctx, ecs::EntityId id) {
	StaticMeshComponent* static_mesh = ctx->world().component<StaticMeshComponent>(id);
	if(!static_mesh) {
		return;
	}
	if(!ImGui::CollapsingHeader("Static mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	ImGui::Columns(2);
	{

		for(usize i = 0; i != static_mesh->sub_meshes().size(); ++i) {
			StaticMeshComponent::SubMesh& sub_mesh = static_mesh->sub_meshes()[i];

			auto find_sub_mesh = [ctx, id, i, sub = sub_mesh]() -> StaticMeshComponent::SubMesh* {
				static_assert(!std::is_reference_v<decltype(sub)>);
				if(StaticMeshComponent* comp = ctx->world().component<StaticMeshComponent>(id)) {
					if(comp->sub_meshes().size() > i && comp->sub_meshes()[i] == sub) {
						return &comp->sub_meshes()[i];
					}
				}
				return nullptr;
			};

			{
				ImGui::Text("Material");
				ImGui::NextColumn();

				bool clear = false;
				if(imgui::asset_selector(ctx, sub_mesh.material.id(), AssetType::Material, "Material", &clear)) {
					ctx->ui().add<AssetSelector>(AssetType::Material)->set_selected_callback(
						[=](AssetId asset) {
							if(const auto material = ctx->loader().load_res<Material>(asset)) {
								if(StaticMeshComponent::SubMesh* sub = find_sub_mesh()) {
									sub->material = material.unwrap();
								}
							}
							return true;
						});
				} else if(clear) {
					sub_mesh.material = AssetPtr<Material>();
				}
			}

			{
				ImGui::NextColumn();
				ImGui::Text("Mesh");
				ImGui::NextColumn();

				bool clear = false;
				if(imgui::asset_selector(ctx, sub_mesh.mesh.id(), AssetType::Mesh, "Mesh", &clear)) {
					ctx->ui().add<AssetSelector>(AssetType::Mesh)->set_selected_callback(
						[=](AssetId asset) {
							if(const auto mesh = ctx->loader().load_res<StaticMesh>(asset)) {
								if(StaticMeshComponent::SubMesh* sub = find_sub_mesh()) {
									sub->mesh = mesh.unwrap();
								}
							}
							return true;
						});
				} else if(clear) {
					sub_mesh.mesh = AssetPtr<StaticMesh>();
				}
			}

			ImGui::NextColumn();
			ImGui::Separator();
		}

		ImGui::Columns(1);

		if(ImGui::Button("Add sub mesh")) {
			static_mesh->sub_meshes().emplace_back();
		}
	}
}



/**************************************************************************
*								Transformable
**************************************************************************/

editor_widget_draw_func(ContextPtr ctx, ecs::EntityId id) {
	TransformableComponent* component = ctx->world().component<TransformableComponent>(id);
	if(!component) {
		return;
	}

	if(!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	ImGui::Columns(2);
	{
		auto [pos, rot, scale] = component->transform().decompose();

		// position
		{
			ImGui::Text("Position");
			ImGui::NextColumn();
			ImGui::InputFloat("X", &pos.x(), 0.0f, 0.0f, "%.2f");
			ImGui::InputFloat("Y", &pos.y(), 0.0f, 0.0f, "%.2f");
			ImGui::InputFloat("Z", &pos.z(), 0.0f, 0.0f, "%.2f");
		}

		ImGui::Separator();

		// rotation
		{
			ImGui::NextColumn();
			ImGui::Text("Rotation");
			ImGui::NextColumn();

			auto is_same_angle = [&](math::Vec3 a, math::Vec3 b) {
				const auto qa = math::Quaternion<>::from_euler(a);
				const auto qb = math::Quaternion<>::from_euler(b);
				for(usize i = 0; i != 3; ++i) {
					math::Vec3 v;
					v[i] = 1.0f;
					if((qa(v) - qb(v)).length2() > 0.001f) {
						return false;
					}
				}
				return true;
			};


			math::Vec3 actual_euler = rot.to_euler();
			auto& euler = ctx->world().component<EditorComponent>(id)->euler();

			if(!is_same_angle(actual_euler, euler)) {
				euler = actual_euler;
			}

			const std::array indexes = {math::Quaternion<>::YawIndex, math::Quaternion<>::PitchIndex, math::Quaternion<>::RollIndex};
			const std::array names = {"Yaw", "Pitch", "Roll"};

			for(usize i = 0; i != 3; ++i) {
				float angle = math::to_deg(euler[indexes[i]]);
				if(ImGui::DragFloat(names[i], &angle, 1.0, -180.0f, 180.0f, "%.2f")) {
					euler[indexes[i]] = math::to_rad(angle);
					rot = math::Quaternion<>::from_euler(euler);
				}
			}
		}

		component->transform() = math::Transform<>(pos, rot, scale);
	}
	ImGui::Columns(1);
}



/**************************************************************************
*								Editor
**************************************************************************/

editor_widget_draw_func(ContextPtr ctx, ecs::EntityId id) {
	EditorComponent* component = ctx->world().component<EditorComponent>(id);
	if(!component) {
		return;
	}

	if(!ImGui::CollapsingHeader("Entity")) {
		return;
	}

	const core::String& name = component->name();

	std::array<char, 1024> name_buffer;
	std::fill(name_buffer.begin(), name_buffer.end(), 0);
	std::copy(name.begin(), name.end(), name_buffer.begin());

	if(ImGui::InputText("Name", name_buffer.data(), name_buffer.size())) {
		component->set_name(name_buffer.data());
	}
}

}
