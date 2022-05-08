/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

#include "EditorWorld.h"

#include <editor/components/EditorComponent.h>

#include <yave/ecs/EntityScene.h>
#include <yave/assets/AssetLoader.h>
#include <yave/utils/FileSystemModel.h>

#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SkyLightComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/TransformableComponent.h>

#include <yave/systems/AssetLoaderSystem.h>
#include <yave/systems/OctreeSystem.h>
#include <yave/systems/ASUpdateSystem.h>

#include <y/utils/format.h>

#include <external/imgui/IconsFontAwesome5.h>

#include <algorithm>

namespace editor {

editor_action("Remove all entities", [] { current_world().clear(); })

EditorWorld::EditorWorld(AssetLoader& loader) {
    add_required_component<EditorComponent>();
    add_system<AssetLoaderSystem>(loader);
    add_system<OctreeSystem>();
    // add_system<ASUpdateSystem>();
}

void EditorWorld::clear() {
    core::Vector<ecs::EntityId> all_entities;
    for(const ecs::EntityId id : ids()) {
        all_entities << id;
    }
    for(const ecs::EntityId id : all_entities) {
        remove_entity(id);
    }
}

void EditorWorld::flush_reload() {
    AssetLoaderSystem* system = find_system<AssetLoaderSystem>();
    system->reset(*this);
}

bool EditorWorld::set_entity_name(ecs::EntityId id, std::string_view name) {
    if(EditorComponent* comp = component<EditorComponent>(id)) {
        comp->set_name(name);
        return true;
    }
    return false;
}


std::string_view EditorWorld::entity_name(ecs::EntityId id) const {
    if(const EditorComponent* comp = component<EditorComponent>(id)) {
        return comp->name();
    }

    return "";
}

std::string_view EditorWorld::entity_icon(ecs::EntityId id) const {
    if(has<StaticMeshComponent>(id)) {
        return ICON_FA_CUBE;
    }

    if(has<PointLightComponent>(id)) {
        return ICON_FA_LIGHTBULB;
    }

    if(has<SpotLightComponent>(id)) {
        return ICON_FA_VIDEO;
    }

    if(has<DirectionalLightComponent>(id)) {
        return ICON_FA_SUN;
    }

    if(has<SkyLightComponent>(id)) {
        return ICON_FA_CLOUD;
    }

    if(const EditorComponent* comp = component<EditorComponent>(id)) {
        if(comp->is_collection()) {
            return ICON_FA_BOX_OPEN;
        }
    }

    return ICON_FA_DATABASE;
}


ecs::EntityId EditorWorld::create_collection_entity(std::string_view name) {
    const ecs::EntityId id = create_entity();
    EditorComponent* comp = component<EditorComponent>(id);
    y_always_assert(comp, "Unable to create entity name");
    comp->set_name(name);
    comp->_is_collection = true;
    return id;
}

ecs::EntityId EditorWorld::add_prefab(std::string_view name) {
    if(const auto id = asset_store().id(name)) {
        return add_prefab(id.unwrap());
    }
    return ecs::EntityId();
}

ecs::EntityId EditorWorld::add_prefab(AssetId asset) {
    y_profile();

    if(const auto prefab = asset_loader().load_res<ecs::EntityPrefab>(asset)) {
        const ecs::EntityId id = create_entity(*prefab.unwrap());

        if(EditorComponent* comp = component<EditorComponent>(id)) {
            comp->set_parent_prefab(asset);
        }
        if(const auto name = asset_store().name(asset)) {
            set_entity_name(id, asset_store().filesystem()->filename(name.unwrap()));
        }
        return id;
    }
    return ecs::EntityId();
}

void EditorWorld::add_scene(std::string_view name, ecs::EntityId parent) {
    if(const auto id = asset_store().id(name)) {
        add_scene(id.unwrap(), parent);
    }
}

void EditorWorld::add_scene(AssetId asset, ecs::EntityId parent) {
    y_profile();

    if(const auto scene = asset_loader().load_res<ecs::EntityScene>(asset)) {
        for(const auto& prefab : scene.unwrap()->prefabs()) {
            set_parent(create_entity(prefab), parent);
        }
    }
}

void EditorWorld::set_parent(ecs::EntityId id, ecs::EntityId parent) {
    y_profile();

    if(EditorComponent* comp = component<EditorComponent>(id)) {
        if(comp->_parent == parent) {
            return;
        }

        if(comp->has_parent()) {
            if(EditorComponent* current_parent = component<EditorComponent>(comp->_parent)) {
                if(current_parent->_is_collection) {
                    const auto it = std::find(current_parent->_children.begin(), current_parent->_children.end(), comp->_parent);
                    if(it != current_parent->_children.end()) {
                        current_parent->_children.erase_unordered(it);
                    }
                }
                comp->_parent = ecs::EntityId();
            }
        }

        if(EditorComponent* new_parent = component<EditorComponent>(parent)) {
            if(new_parent->_is_collection) {
                new_parent->_children.push_back(id);
                comp->_parent = parent;
            }
        }
    }
}

core::Span<std::pair<core::String, ecs::ComponentRuntimeInfo>> EditorWorld::component_types() {
    static core::Vector<std::pair<core::String, ecs::ComponentRuntimeInfo>> types;
    static bool init = false;

    if(!init) {
        for(const auto* poly_base = ecs::ComponentContainerBase::_y_serde3_poly_base.first; poly_base; poly_base = poly_base->next) {
            if(const auto container = poly_base->create()) {
                const ecs::ComponentRuntimeInfo info = container->runtime_info();
                types.emplace_back(info.clean_component_name(), info);
            }
        }
        init = true;
    }

    return types;
}



}

