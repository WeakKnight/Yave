/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include "Scene.h"

#include <yave/material/Material.h>

#include <y/core/Chrono.h>
#include <y/io/File.h>

namespace yave {

void Scene::serialized(io::WriterRef writer) const {
	writer->write_one(fs::magic_number);
	writer->write_one(fs::scene_file_type);
	u32 version = 2;
	writer->write_one(version);

	writer->write_one(u32(_statics.size()));
	writer->write_one(u32(_lights.size()));

	for(const auto& mesh : _statics) {
		AssetId id = mesh->mesh().id();
		if(id == assets::invalid_id) {
			log_msg("Asset with invalid id, skipping.", Log::Warning);
			continue;
		}
		writer->write_one(id);
		writer->write_one(mesh->transform());
	}

	for(const auto& light : _lights) {
		writer->write_one(*light);
	}

	for(const auto& renderable : _renderables) {
		log_msg("Can not serialize renderable of type \"" + type_name(*renderable) + "\"", Log::Warning);
	}

}

Scene Scene::deserialized(io::ReaderRef reader, AssetLoader<StaticMesh>& mesh_loader) {
	core::DebugTimer _("Scene::from_file()");

	struct Header {
		u32 magic;
		u32 type;
		u32 version;

		u32 statics;
		u32 lights;

		bool is_valid() const {
			return magic == fs::magic_number &&
				   type == fs::scene_file_type &&
				   version == 2;
		}
	};

	auto header = reader->read_one<Header>();
	if(!header.is_valid()) {
		y_throw("Invalid header");
	}

	DevicePtr dptr = mesh_loader.device();
	auto material = make_asset<Material>(dptr, MaterialData()
			.set_frag_data(SpirVData::deserialized(io::File::open("basic.frag.spv").expected("Unable to load spirv file.")))
			.set_vert_data(SpirVData::deserialized(io::File::open("basic.vert.spv").expected("Unable to load spirv file.")))
		);

	Scene scene;
	scene.static_meshes().set_min_capacity(header.statics);
	scene.lights().set_min_capacity(header.lights);

	{
		for(u32 i = 0; i != header.statics; ++i) {
			auto id = reader->read_one<AssetId>();
			auto transform = reader->read_one<math::Transform<>>();

			if(id == assets::invalid_id) {
				log_msg("Skipping asset with invalid id.", Log::Warning);
				continue;
			}

			try {
				auto mesh = mesh_loader.load(id);
				auto inst = std::make_unique<StaticMeshInstance>(mesh, material);
				inst->transform() = transform;
				scene.static_meshes().emplace_back(std::move(inst));
			} catch(std::exception& e) {
				log_msg("Unable to load asset: "_s + e.what() + " Skipping.", Log::Warning);
				continue;
			}
		}
	}

	{
		for(u32 i = 0; i != header.lights; ++i) {
			// load as point, then read on top. Maybe change this ?
			auto light = std::make_unique<Light>(Light::Point);
			reader->read_one(*light);
			scene.lights().emplace_back(std::move(light));
		}
	}

	return scene;
}

}
