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

#include "DescriptorSetAllocator.h"

namespace yave {

DescriptorSetData::DescriptorSetData(DescriptorSetPool* pool, u32 id) : _pool(pool), _index(id) {
}

DevicePtr DescriptorSetData::device() const {
    return _pool ? _pool->device() : nullptr;
}

bool DescriptorSetData::is_null() const {
    return !device();
}

void DescriptorSetData::recycle() {
    if(_pool) {
        _pool->recycle(_index);
    }
}

VkDescriptorSetLayout DescriptorSetData::vk_descriptor_set_layout() const {
    return _pool->vk_descriptor_set_layout();
}

VkDescriptorSet DescriptorSetData::vk_descriptor_set() const {
    return _pool->vk_descriptor_set(_index);
}

}

