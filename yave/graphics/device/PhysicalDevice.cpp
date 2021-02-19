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

#include "PhysicalDevice.h"

namespace yave {

PhysicalDevice::PhysicalDevice(VkPhysicalDevice device) : _device(device) {

    vkGetPhysicalDeviceMemoryProperties(_device, &_memory_properties);
    vkGetPhysicalDeviceFeatures(_device, &_supported_features);

    {
        _uniform_blocks_properties = vk_struct();

        VkPhysicalDeviceProperties2 properties = vk_struct();
        properties.pNext = &_uniform_blocks_properties;
        vkGetPhysicalDeviceProperties2(_device, &properties);
        _properties = properties.properties;
    }
}

VkPhysicalDevice PhysicalDevice::vk_physical_device() const {
    return _device;
}

const VkPhysicalDeviceProperties& PhysicalDevice::vk_properties() const {
    return _properties;
}

const VkPhysicalDeviceInlineUniformBlockPropertiesEXT& PhysicalDevice::vk_uniform_block_properties() const {
    return _uniform_blocks_properties;
}

const VkPhysicalDeviceMemoryProperties& PhysicalDevice::vk_memory_properties() const {
    return _memory_properties;
}

bool PhysicalDevice::is_discrete() const {
    return _properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

usize PhysicalDevice::total_device_memory() const {
    usize total = 0;
    for(u32 i = 0; i != _memory_properties.memoryHeapCount; ++i) {
        if(_memory_properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            total += _memory_properties.memoryHeaps[i].size;
        }
    }
    return total;
}

bool PhysicalDevice::support_features(const VkPhysicalDeviceFeatures& features) const {
    const auto supported = reinterpret_cast<const VkBool32*>(&_supported_features);
    const auto required = reinterpret_cast<const VkBool32*>(&features);
    for(usize i = 0; i != sizeof(features) / sizeof(VkBool32); ++i) {
        if(required[i] && !supported[i]) {
            return false;
        }
    }
    return true;
}

}

