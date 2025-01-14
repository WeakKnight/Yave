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

#include "Swapchain.h"

#include <yave/window/Window.h>

#include <yave/graphics/commands/CmdQueue.h>
#include <yave/graphics/memory/DeviceMemoryHeapBase.h>
#include <yave/graphics/barriers/Barrier.h>

#include <yave/graphics/device/extensions/DebugUtils.h>
#include <yave/utils/gpuprofile.h>

#include <y/core/FixedArray.h>
#include <y/utils/log.h>

namespace yave {

static VkSurfaceCapabilitiesKHR compute_capabilities(VkSurfaceKHR surface) {
    VkSurfaceCapabilitiesKHR capabilities = {};
    if(is_error(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device(), surface, &capabilities))) {
        return {};
    }
    return capabilities;
}

static VkSurfaceFormatKHR surface_format(VkSurfaceKHR surface) {
    Y_TODO(Find best format instead of always returning first)
    u32 format_count = 1;
    VkSurfaceFormatKHR format = {};
    vk_check_or_incomplete(vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device(), surface, &format_count, &format));
    y_always_assert(format_count, "No swapchain format supported");
    return format;
}

static VkPresentModeKHR present_mode(VkSurfaceKHR surface) {
    std::array<VkPresentModeKHR, 16> modes = {};
    u32 mode_count = u32(modes.size());
    vk_check(vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device(), surface, &mode_count, modes.data()));
    y_always_assert(mode_count, "No presentation mode supported");
    for(u32 i = 0; i != mode_count; ++i) {
        if(modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

static u32 compute_image_count(VkSurfaceCapabilitiesKHR capabilities) {
    const u32 ideal = 3;
    if(capabilities.maxImageCount < ideal && capabilities.maxImageCount > capabilities.minImageCount) {
        return capabilities.maxImageCount;
    }
    if(capabilities.minImageCount > ideal) {
        return capabilities.minImageCount;
    }
    return ideal;
}

static VkImageView create_image_view(VkImage image, VkFormat format) {
    const VkComponentMapping mapping = {
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY
    };

    VkImageSubresourceRange subrange = {};
    {
        subrange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subrange.layerCount = 1;
        subrange.levelCount = 1;
    }

    VkImageViewCreateInfo create_info = vk_struct();
    {
        create_info.image = image;
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = format;
        create_info.components = mapping;
        create_info.subresourceRange = subrange;
    }

    VkImageView view = {};
    vk_check(vkCreateImageView(vk_device(), &create_info, vk_allocation_callbacks(), &view));
    return view;
}

static bool has_wsi_support(VkSurfaceKHR surface) {
    const u32 index =  command_queue().family_index();
    VkBool32 supported = false;
    vk_check(vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device(), index, surface, &supported));
    return supported;
}

#ifdef Y_OS_WIN
static VkSurfaceKHR create_surface(HINSTANCE_ instance, HWND_ handle) {
    VkWin32SurfaceCreateInfoKHR create_info = vk_struct();
    {
        create_info.hinstance = instance;
        create_info.hwnd = handle;
    }

    VkSurfaceKHR surface = {};
    vk_check(vkCreateWin32SurfaceKHR(vk_device_instance(), &create_info, vk_allocation_callbacks(), &surface));

    if(!has_wsi_support(surface)) {
        y_fatal("No WSI support.");
    }
    log_msg("Vulkan WSI supported!");

    return surface;
}
#endif

#ifdef Y_OS_LINUX
static VkSurfaceKHR create_surface(xcb_connection_t* connection, u32 window) {
    VkXcbSurfaceCreateInfoKHR create_info = vk_struct();
    {
        create_info.connection = connection;
        create_info.window = window;
    }

    VkSurfaceKHR surface = {};
    vk_check(vkCreateXcbSurfaceKHR(vk_device_instance(), &create_info, vk_allocation_callbacks(), &surface));

    if(!has_wsi_support(surface)) {
        y_fatal("No WSI support.");
    }
    log_msg("Vulkan WSI supported!");

    return surface;
}
#endif

static VkSurfaceKHR create_surface(Window* window) {
    y_profile();

#if defined(Y_OS_WIN) || defined(Y_OS_LINUX)
    return create_surface(window->instance(), window->handle());
#endif

    unused(window);
    return vk_null();
}


Swapchain::Swapchain(Window* window) : Swapchain(create_surface(window)) {
}

Swapchain::Swapchain(VkSurfaceKHR surface) : _surface(surface) {
    build_swapchain();
    build_semaphores();
}

bool Swapchain::reset() {
    wait_all_queues();

    _images.clear();

    const VkSwapchainKHR old = _swapchain;

    if(build_swapchain()) {
        destroy_graphic_resource(old);
        return true;
    }

    return false;
}

Swapchain::~Swapchain() {
    _images.clear();

    destroy_semaphores();

    destroy_graphic_resource(_swapchain);
    destroy_graphic_resource(_surface);
}

bool Swapchain::build_swapchain() {
    y_debug_assert(_images.is_empty());

    const VkSurfaceCapabilitiesKHR capabilities = compute_capabilities(_surface);

    _size = {capabilities.currentExtent.width, capabilities.currentExtent.height};

    if(!_size.x() || !_size.y()) {
        return false;
    }

    const VkSurfaceFormatKHR format = surface_format(_surface);
    _color_format = VkFormat(format.format);

    const VkImageUsageFlagBits image_usage_flags = VkImageUsageFlagBits(SwapchainImageUsage & ~ImageUsage::SwapchainBit);
    if((capabilities.supportedUsageFlags & image_usage_flags) != image_usage_flags) {
        y_fatal("Swapchain does not support required usage flags.");
    }

    {
        y_profile_zone("create swapchain");
        VkSwapchainCreateInfoKHR create_info = vk_struct();
        {
            create_info.imageUsage = image_usage_flags;
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            create_info.imageArrayLayers = 1;
            create_info.clipped = true;
            create_info.surface = _surface;
            create_info.preTransform = capabilities.currentTransform;
            create_info.imageFormat = format.format;
            create_info.imageColorSpace = format.colorSpace;
            create_info.imageExtent = capabilities.currentExtent;
            create_info.minImageCount = compute_image_count(capabilities);
            create_info.presentMode = present_mode(_surface);
            create_info.oldSwapchain = _swapchain;
        }
        vk_check(vkCreateSwapchainKHR(vk_device(), &create_info, vk_allocation_callbacks(), &_swapchain.get()));
    }

    y_profile_zone("image setup");

    u32 image_count = 0;
    vk_check(vkGetSwapchainImagesKHR(vk_device(), _swapchain, &image_count, nullptr));

    core::FixedArray<VkImage> images(image_count);
    vk_check(vkGetSwapchainImagesKHR(vk_device(), _swapchain, &image_count, images.data()));

    for(auto image : images) {
        const VkImageView view = create_image_view(image, _color_format.vk_format());

        struct SwapchainImageMemory : DeviceMemory {
            SwapchainImageMemory() : DeviceMemory(vk_null(), 0, 0) {
            }
        };

        auto swapchain_image = SwapchainImage();

        swapchain_image._memory = SwapchainImageMemory();

        swapchain_image._size = math::Vec3ui(_size, 1);
        swapchain_image._format = _color_format;
        swapchain_image._usage = SwapchainImageUsage;

        // prevent the images to delete their handles: the swapchain already does that.
        swapchain_image._image = image;
        swapchain_image._view = view;

#ifdef Y_DEBUG
        if(const auto* debug = debug_utils()) {
            debug->set_resource_name(image, "Swapchain Image");
            debug->set_resource_name(view, "Swapchain Image View");
        }
#endif

        _images << std::move(swapchain_image);
    }

    CmdBufferRecorder recorder(create_disposable_cmd_buffer());
    for(auto& i : _images) {
        recorder.barriers({ImageBarrier::transition_barrier(i, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)});
    }
    command_queue().submit(std::move(recorder)).wait();

    return true;
}

void Swapchain::build_semaphores() {
    const VkFenceCreateInfo fence_create_info = vk_struct();
    const VkSemaphoreCreateInfo semaphore_create_info = vk_struct();

    for(usize i = 0; i != image_count(); ++i) {
        auto& semaphores = _semaphores.emplace_back();
        vk_check(vkCreateSemaphore(vk_device(), &semaphore_create_info, vk_allocation_callbacks(), &semaphores.render_complete));
        vk_check(vkCreateSemaphore(vk_device(), &semaphore_create_info, vk_allocation_callbacks(), &semaphores.image_aquired));
        vk_check(vkCreateFence(vk_device(), &fence_create_info, vk_allocation_callbacks(), &semaphores.fence));
    }

    y_debug_assert(image_count() == _semaphores.size());
}

void Swapchain::destroy_semaphores() {
    for(const auto& semaphores : _semaphores) {
        destroy_graphic_resource(semaphores.render_complete);
        destroy_graphic_resource(semaphores.image_aquired);
        destroy_graphic_resource(semaphores.fence);
    }
    _semaphores.clear();
}

core::Result<FrameToken> Swapchain::next_frame() {
    y_profile();

    if(_images.is_empty()) {
        if(!reset()) {
            return core::Err();
        }
    }

    y_debug_assert(_semaphores.size());

    const usize semaphore_index = ++_frame_id % image_count();
    const Semaphores& frame_semaphores = _semaphores[semaphore_index];

    u32 image_index = u32(-1);
    {
        y_profile_zone("aquire");
        while(vk_swapchain_out_of_date(vkAcquireNextImageKHR(vk_device(), _swapchain, u64(-1), frame_semaphores.image_aquired, frame_semaphores.fence, &image_index))) {
            if(!reset()) {
                return core::Err();
            }
        }
    }

    y_debug_assert(image_index < _images.size());

    {
        y_profile_zone("image fence");
        vk_check(vkWaitForFences(vk_device(), 1, &frame_semaphores.fence, true, u64(-1)));
        vk_check(vkResetFences(vk_device(), 1, &frame_semaphores.fence));
    }

    return core::Ok(FrameToken {
        _frame_id,
        image_index,
        u32(_images.size()),
        SwapchainImageView(_images[image_index]),
    });
}

void Swapchain::present(const FrameToken& token, CmdBufferRecorder&& recorder, const CmdQueue& queue) {
    y_profile();

    const usize semaphore_index = token.id % image_count();
    const Semaphores& frame_semaphores = _semaphores[semaphore_index];

    {
        queue.submit(std::move(recorder), frame_semaphores.image_aquired, frame_semaphores.render_complete);

        const auto lock = y_profile_unique_lock(queue._lock);

        y_profile_zone("present");

        VkPresentInfoKHR present_info = vk_struct();
        {
            present_info.swapchainCount = 1;
            present_info.pSwapchains = &_swapchain.get();
            present_info.pImageIndices = &token.image_index;
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores = &frame_semaphores.render_complete;
        }

        if(vk_swapchain_out_of_date(vkQueuePresentKHR(queue.vk_queue(), &present_info))) {
            // Nothing ?
        }
    }

    y_profile_frame_end();
}

VkSwapchainKHR Swapchain::vk_swapchain() const {
    return _swapchain;
}

const math::Vec2ui& Swapchain::size() const {
    return _size;
}

usize Swapchain::image_count() const {
    return _images.size();
}

ImageFormat Swapchain::color_format() const {
    return _color_format;
}

}
