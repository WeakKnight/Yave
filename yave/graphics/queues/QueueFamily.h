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
#ifndef YAVE_GRAPHICS_QUEUES_QUEUEFAMILY_H
#define YAVE_GRAPHICS_QUEUES_QUEUEFAMILY_H

#include <yave/device/PhysicalDevice.h>
#include <y/core/Result.h>

#include "Queue.h"

namespace yave {

class QueueFamily {

    public:
        static constexpr auto Graphics = VK_QUEUE_GRAPHICS_BIT;

        static core::Result<QueueFamily> create(const PhysicalDevice& dev, u32 index);
        static core::Vector<QueueFamily> all(const PhysicalDevice& dev);

        u32 index() const;
        u32 count() const;

        VkQueueFlags flags() const;

        core::Vector<Queue> queues(DevicePtr dptr) const;

    private:
        QueueFamily(u32 index, const VkQueueFamilyProperties& props);

        u32 _index;
        u32 _queue_count;
        VkQueueFlags _flags;


};

}

#endif // YAVE_GRAPHICS_QUEUES_QUEUEFAMILY_H

