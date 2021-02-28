/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include "CmdBufferPool.h"
#include "CmdBuffer.h"

#include <yave/graphics/utils.h>
#include <yave/graphics/device/Queue.h>
#include <yave/graphics/device/LifetimeManager.h>

#include <y/core/Chrono.h>
#include <y/concurrent/concurrent.h>


namespace yave {

static VkCommandPool create_pool(DevicePtr dptr) {
    VkCommandPoolCreateInfo create_info = vk_struct();
    {
        create_info.queueFamilyIndex = graphic_queue(dptr).family_index();
        create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    }

    VkCommandPool pool = {};
    vk_check(vkCreateCommandPool(vk_device(dptr), &create_info, vk_allocation_callbacks(dptr), &pool));
    return pool;
}


CmdBufferPool::CmdBufferPool(DevicePtr dptr) :
        DeviceLinked(dptr),
        _pool(create_pool(dptr)),
        _thread_id(concurrent::thread_id()) {
}

CmdBufferPool::~CmdBufferPool() {
    join_all();

    y_debug_assert(_pending.is_empty()); // We probably need to wait for those too....
    y_debug_assert(_cmd_buffers.size() == _recycled.size());

    _cmd_buffers.clear();
    _recycled.clear();

    for(const VkFence fence : _fences) {
        destroy(fence);
    }
    destroy(_pool);
}

VkCommandPool CmdBufferPool::vk_pool() const {
    return _pool;
}

void CmdBufferPool::join_all() {
    const auto lock = y_profile_unique_lock(_pool_lock);

    if(_fences.is_empty()) {
        return;
    }

    vk_check(vkWaitForFences(vk_device(device()), _fences.size(), _fences.data(), true, u64(-1)));
}

void CmdBufferPool::release(CmdBufferData* data) {
    y_debug_assert(data->pool() == this);

    {
        const auto lock = y_profile_unique_lock(_pending_lock);
        _pending << data;
    }

    lifetime_manager(device()).queue_for_recycling(data);
}


void CmdBufferPool::prepare_for_recycling(CmdBufferData* data) {
    data->set_signaled();
    data->release_resources();
}

void CmdBufferPool::recycle(CmdBufferData* data) {
    y_profile();

    y_debug_assert(data->poll_fence());
    y_debug_assert(data->pool() == this);

    {
        const auto lock = y_profile_unique_lock(_pending_lock);
        const auto it = std::find(_pending.begin(), _pending.end(), data);

        // Already recycled
        if(it == _pending.end()) {
            return;
        }

        _pending.erase_unordered(it);
    }

    prepare_for_recycling(data);

    {
        const auto lock = y_profile_unique_lock(_recycle_lock);
        _recycled << data;
    }
}

CmdBufferData* CmdBufferPool::alloc() {
    y_profile();

    y_debug_assert(concurrent::thread_id() == _thread_id);

    CmdBufferData* data = nullptr;
    {
        const auto lock = y_profile_unique_lock(_recycle_lock);
        if(!_recycled.is_empty()) {
            data = _recycled.pop();
        }
    }

    if(!data) {
        auto lock = y_profile_unique_lock(_pending_lock);
        const auto it = std::find_if(_pending.begin(), _pending.end(), [](CmdBufferData* data) { return data->poll_fence(); });

        if(it != _pending.end()) {
            data = *it;
            _pending.erase_unordered(it);
            lifetime_manager(device()).set_recycled(data);

            lock.unlock(); // Remove this?  =|
            prepare_for_recycling(data);
        }
    }

    if(data) {
        data->reset();
        return data;
    }

    return create_data();
}

CmdBufferData* CmdBufferPool::create_data() {
    const auto lock = y_profile_unique_lock(_pool_lock);

    const VkFenceCreateInfo fence_create_info = vk_struct();
    VkCommandBufferAllocateInfo allocate_info = vk_struct();
    {
        allocate_info.commandBufferCount = 1;
        allocate_info.commandPool = _pool;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    }

    VkCommandBuffer buffer = {};
    VkFence fence = {};
    vk_check(vkAllocateCommandBuffers(vk_device(device()), &allocate_info, &buffer));
    vk_check(vkCreateFence(vk_device(device()), &fence_create_info, vk_allocation_callbacks(device()), &fence));

    _fences << fence;
    return _cmd_buffers.emplace_back(std::make_unique<CmdBufferData>(buffer, fence, this)).get();
}


CmdBuffer CmdBufferPool::create_buffer() {
    return CmdBuffer(alloc());
}

}

