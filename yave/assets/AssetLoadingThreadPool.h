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
#ifndef YAVE_ASSETS_ASSETLOADINGTHREADPOOL_H
#define YAVE_ASSETS_ASSETLOADINGTHREADPOOL_H

#include "AssetLoadingContext.h"

#include <y/core/Vector.h>
#include <y/core/Functor.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <list>

namespace yave {

class AssetLoadingThreadPool : NonMovable {
	public:
		using CreateFunc = core::Function<void()>;
		using ReadFunc = core::Function<CreateFunc(AssetLoadingContext&)>;

		class LoadingJob : NonMovable {
			public:
				virtual ~LoadingJob();

				virtual core::Result<void> read() = 0;
				virtual void finalize(DevicePtr dptr) = 0;
				virtual void set_dependencies_failed() = 0;

				const AssetDependencies& dependencies() const;
				AssetLoader* parent() const;

			protected:
				LoadingJob(AssetLoader* loader);

				AssetLoadingContext& loading_context();

			private:
				AssetLoadingContext _ctx;
		};


		AssetLoadingThreadPool(AssetLoader* parent, usize concurency = 1);
		~AssetLoadingThreadPool();

		DevicePtr device() const;


		void wait_until_loaded(const GenericAssetPtr& ptr);

		void add_loading_job(std::unique_ptr<LoadingJob> job);

	private:
		void process_one(std::unique_lock<std::mutex> lock);
		void worker();

		std::deque<std::unique_ptr<LoadingJob>> _loading_jobs;
		std::list<std::unique_ptr<LoadingJob>> _finalize_jobs;

		std::mutex _lock;
		std::condition_variable _condition;

		core::Vector<std::thread> _threads;
		std::atomic<bool> _run = true;

		AssetLoader* _parent = nullptr;
};

}

#endif // YAVE_ASSETS_ASSETLOADINGTHREADPOOL_H
