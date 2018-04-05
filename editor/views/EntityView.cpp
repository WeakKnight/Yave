/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#include "EntityView.h"

#include <editor/EditorContext.h>

#include <yave/objects/Light.h>

#include <imgui/imgui.h>

namespace editor {

static const char* light_type_name(Light::Type type) {
	switch(type) {
		case Light::Directional:
			return "Directional";

		case Light::Point:
			return "Point";

		default:
		break;
	}
	return y_fatal("Unsupported light type.");
}


EntityView::EntityView(ContextPtr cptr) : Dock("Entities"), ContextLinked(cptr) {
}

void EntityView::add_light() {
	Scene::Ptr<Light> light = std::make_unique<Light>(Light::Point);
	context()->selected = light.get();

	light->radius() = 100.0f;
	light->color() *= 10000.0;
	context()->scene()->lights() << std::move(light);
}

void EntityView::paint_ui(CmdBufferRecorder<>&, const FrameToken&) {
	char buffer[256];

	if(ImGui::Button("+", math::Vec2(24))) {
		ImGui::OpenPopup("Add entity");
	}

	if(ImGui::BeginPopup("Add entity")) {
		if(ImGui::MenuItem("Add light")) {
			add_light();
		}
		ImGui::MenuItem("Add renderable");
		ImGui::EndPopup();
	}

	if(ImGui::TreeNode("Renderables")) {
		for(const auto& r : context()->scene()->renderables()) {
			std::sprintf(buffer, "%s##%p", type_name(*r).data(), static_cast<void*>(r.get()));
			bool selected = r.get() == context()->selected;
			ImGui::Selectable(buffer, &selected);
			context()->selected = selected ? r.get() : context()->selected;
		}
		ImGui::TreePop();
	}

	if(ImGui::TreeNode("Lights")) {
		for(const auto& l : context()->scene()->lights()) {
			std::sprintf(buffer, "%s##%p", light_type_name(l->type()), static_cast<void*>(l.get()));
			bool selected = l.get() == context()->selected;
			ImGui::Selectable(buffer, &selected);
			context()->selected = selected ? l.get() : context()->selected;
		}
		ImGui::TreePop();
	}

}

}
