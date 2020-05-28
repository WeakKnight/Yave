/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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
#ifndef YAVE_ASSETS_ASSETLOADER_INL
#define YAVE_ASSETS_ASSETLOADER_INL

#ifndef YAVE_ASSETS_ASSETLOADER_H
#error this file should not be included directly

// Just to help the IDE
#include "AssetLoader.h"
#endif

#include <y/serde3/archives.h>

#include <y/core/Chrono.h>
#include <y/utils/log.h>

namespace yave {

template<typename T>
void AssetPtr<T>::post_deserialize(AssetLoadingContext& context)  {
	const AssetId id = _id;
	if(id == AssetId::invalid_id()) {
		return;
	}

	*this = context.load_async<T>(id);
	flush_reload();
}

template<typename T>
void AssetPtr<T>::reload() {
	y_debug_assert(!_data || _data->id == _id);
	if(has_loader()) {
		flush_reload();
		loader()->reload(*this);
		flush_reload();
	}
}

template<typename T>
void AssetPtr<T>::wait_until_loaded() const {
	if(has_loader() && !is_loaded()) {
		loader()->wait_until_loaded(*this);
		y_debug_assert(is_loaded());
	}
}

template<typename T>
core::Result<core::String> AssetPtr<T>::name() const {
	if(has_loader()) {
		AssetStore& store = loader()->store();
		if(auto r = store.name(_id)) {
			return core::Ok(std::move(r.unwrap()));
		}
	}
	return core::Err();
}








// --------------------------- Loader ---------------------------

template<typename T>
AssetLoader::Loader<T>::Loader(AssetLoader* parent) : LoaderBase(parent) {
	y_always_assert(parent, "Parent should not be null.");
}

template<typename T>
AssetLoader::Loader<T>::~Loader() {
	y_profile();
	const auto lock = y_profile_unique_lock(_lock);
	for(auto&& [id, ptr] : _loaded) {
		if(!ptr.expired()) {
			y_fatal("Asset is still live.");
		}
	}
}

template<typename T>
bool AssetLoader::Loader<T>::find_ptr(AssetPtr<T>& ptr) {
	const AssetId id = ptr.id();
	if(id == AssetId::invalid_id()) {
		return true;
	}

	const auto lock = y_profile_unique_lock(_lock);
	auto& weak = _loaded[id];
	ptr = weak.lock();
	if(ptr._data) {
		return true;
	}
	weak = (ptr = std::make_shared<Data>(id, parent()))._data;
	return false;
}


template<typename T>
AssetPtr<T> AssetLoader::Loader<T>::load(AssetId id) {
	y_profile();
	auto ptr = load_async(id);
	parent()->wait_until_loaded(ptr);
	y_debug_assert(!ptr.is_loading());
	return ptr;

}

template<typename T>
AssetPtr<T> AssetLoader::Loader<T>::load_async(AssetId id) {
	y_profile();
	AssetPtr<T> ptr(id);
	if(!find_ptr(ptr)) {
		parent()->_thread_pool.add_loading_job(create_loading_job(ptr));
	}
	return ptr;
}

template<typename T>
AssetPtr<T> AssetLoader::Loader<T>::reload(const AssetPtr<T>& ptr) {
	y_profile();

	const AssetId id = ptr.id();
	if(id == AssetId::invalid_id()) {
		return AssetPtr<T>();
	}

	y_always_assert(!ptr.is_empty(), "Can not reload empty asset");

	AssetPtr<T> reloaded(id, parent());
	{
		parent()->_thread_pool.add_loading_job(create_loading_job(reloaded));
		parent()->wait_until_loaded(reloaded);
		y_debug_assert(!reloaded.is_loading());
	}

	{
		const auto lock = y_profile_unique_lock(_lock);
		auto& weak = _loaded[id];
		if(auto orig = weak.lock()) {
			orig->set_reloaded(reloaded._data);
		}
		weak = reloaded._data;
	}
	return reloaded;
}

template<typename T>
std::unique_ptr<AssetLoader::LoadingJob> AssetLoader::Loader<T>::create_loading_job(AssetPtr<T> ptr) {
	class Job : public LoadingJob {
		public:
			Job(AssetLoader* loader, std::shared_ptr<Data> data) : LoadingJob(loader), _data(std::move(data)) {
				y_always_assert(_data, "Invalid asset");
			}

			core::Result<void> read() override {
				y_profile_zone("loading");

				const AssetId id = _data->id;

				y_always_assert(_data->is_loading(), "Asset is not in a loading state");
				y_always_assert(_data->loader() == parent(), "Mismatched AssetLoaders");
				y_always_assert(id != AssetId::invalid_id(), "Invalid asset ID");

				if(auto reader = parent()->store().data(id)) {
					const serde3::Result res = serde3::ReadableArchive(*reader.unwrap()).deserialize(_load_from, loading_context());
					if(res.is_error()/* || res.unwrap() == serde3::Success::Partial*/) {
						_data->set_failed(ErrorType::InvalidData);
						log_msg(fmt("Unable to load %: invalid data", asset_name()), Log::Error);
						return core::Err();
					}
					return core::Ok();
				}
				_data->set_failed(ErrorType::InvalidID);
				y_debug_assert(!_data->is_loading());
				log_msg(fmt("Unable to load asset: invalid ID"), Log::Error);
				return core::Err();
			}

			void finalize(DevicePtr dptr) {
				if(_data->is_failed()) {
					return;
				}

				y_profile_zone("finalizing");
				y_debug_assert(_data->is_loading());
				_data->finalize_loading(T(dptr, std::move(_load_from)));
			}

			void set_dependencies_failed() {
				if(_data->is_failed()) {
					return;
				}

				_data->set_failed(AssetLoadingErrorType::FailedDependency);
				log_msg(fmt("Unable to load %: failed to load dependency", asset_name()), Log::Error);
			}

		private:
			std::shared_ptr<Data> _data;
			LoadFrom _load_from;

			core::String asset_name() const {
				return AssetPtr<T>(_data).name().unwrap_or("asset");
			}
	};
	return std::make_unique<Job>(parent(), std::move(ptr._data));
}








// --------------------------- AssetLoader ---------------------------

template<typename T>
AssetLoader::Result<T> AssetLoader::load_res(AssetId id) {
	auto ptr = load<T>(id);
	if(ptr.is_failed()) {
		return core::Err(ptr.error());
	}
	return core::Ok(std::move(ptr));
}

template<typename T>
AssetLoader::Result<T> AssetLoader::load_res(std::string_view name) {
	return load<T>(store().id(name));
}


template<typename T>
AssetPtr<T> AssetLoader::load(AssetId id) {
	return loader_for_type<T>().load(id);
}

template<typename T>
AssetPtr<T> AssetLoader::load_async(AssetId id) {
	return loader_for_type<T>().load_async(id);
}


template<typename T>
AssetPtr<T> AssetLoader::reload(const AssetPtr<T>& ptr) {
	return loader_for_type<T>().reload(ptr);
}

template<typename T>
AssetLoader::Result<T> AssetLoader::import(std::string_view name, std::string_view import_from) {
	return load<T>(load_or_import(name, import_from, AssetTraits<T>::type));
}

template<typename T, typename E>
AssetLoader::Result<T> AssetLoader::load(core::Result<AssetId, E> id) {
	if(id) {
		return load_res<T>(id.unwrap());
	}
	return core::Err(ErrorType::UnknownID);
}

template<typename T>
AssetLoader::Loader<T>& AssetLoader::loader_for_type() {
	const auto lock = y_profile_unique_lock(_lock);
	auto& loader = _loaders[typeid(T)];
	if(!loader) {
		loader = std::make_unique<Loader<T>>(this);
	}
	return *dynamic_cast<Loader<T>*>(loader.get());
}








// --------------------------- AssetLoadingContext ---------------------------

template<typename T>
AssetPtr<T> AssetLoadingContext::load(AssetId id) {
	auto ptr = _parent->load<T>(id);
	_dependencies.add_dependency(ptr);
	return ptr;
}

template<typename T>
AssetPtr<T> AssetLoadingContext::load_async(AssetId id) {
	auto ptr = _parent->load_async<T>(id);
	_dependencies.add_dependency(ptr);
	return ptr;
}


}

#endif // YAVE_ASSETS_ASSETLOADER_INL
