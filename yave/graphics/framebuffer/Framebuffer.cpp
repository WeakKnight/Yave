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
#include "Framebuffer.h"
#include "RenderPass.h"
#include <yave/graphics/utils.h>

namespace yave {

static math::Vec2ui compute_size(const Framebuffer::DepthAttachment& depth, core::Span<Framebuffer::ColorAttachment> colors) {
    math::Vec2ui ref;
    if(depth.view.device()) {
        ref = depth.view.size();
    } else if(!colors.is_empty()) {
        ref = colors[0].view.size();
    }

    for(const auto& c : colors) {
        y_always_assert(c.view.size() == ref, "Invalid attachment size");
    }
    return ref;
}

static core::Vector<Framebuffer::ColorAttachment> color_attachments(core::Span<ColorAttachmentView> color_views, Framebuffer::LoadOp load_op) {
    auto colors = core::vector_with_capacity<Framebuffer::ColorAttachment>(color_views.size());
    std::transform(color_views.begin(), color_views.end(), std::back_inserter(colors), [=](const auto& c) { return Framebuffer::ColorAttachment(c, load_op); });
    return colors;
}

static std::unique_ptr<RenderPass> create_render_pass(DevicePtr dptr, const Framebuffer::DepthAttachment& depth, core::Span<Framebuffer::ColorAttachment> colors) {
    auto color_vec = core::vector_with_capacity<RenderPass::AttachmentData>(colors.size());
    std::transform(colors.begin(), colors.end(), std::back_inserter(color_vec), [](const auto& c) { return RenderPass::AttachmentData(c.view, c.load_op); });
    return depth.view.device()
        ? std::make_unique<RenderPass>(dptr, RenderPass::AttachmentData(depth.view, depth.load_op), color_vec)
        : std::make_unique<RenderPass>(dptr, color_vec);
}

Framebuffer::Framebuffer(DevicePtr dptr, const DepthAttachment& depth, core::Span<ColorAttachment> colors) :
        DeviceLinked(dptr),
        _size(compute_size(depth, colors)),
        _attachment_count(colors.size()),
        _render_pass(create_render_pass(dptr, depth, colors)) {

    auto views = core::vector_with_capacity<VkImageView>(colors.size() + 1);
    std::transform(colors.begin(), colors.end(), std::back_inserter(views), [](const auto& v) { return v.view.vk_view(); });

    if(depth.view.device()) {
        views << depth.view.vk_view();
    }

    VkFramebufferCreateInfo create_info = vk_struct();
    {
        create_info.renderPass = _render_pass->vk_render_pass();
        create_info.attachmentCount = u32(views.size());
        create_info.pAttachments = views.data();
        create_info.width = _size.x();
        create_info.height = _size.y();
        create_info.layers = 1;
    }

    vk_check(vkCreateFramebuffer(vk_device(device()), &create_info, vk_allocation_callbacks(device()), &_framebuffer.get()));
}

Framebuffer::Framebuffer(DevicePtr dptr, core::Span<ColorAttachmentView> colors, LoadOp load_op) :
        Framebuffer(dptr, DepthAttachment(), color_attachments(colors, load_op)) {
}

Framebuffer::Framebuffer(DevicePtr dptr, const DepthAttachmentView& depth, core::Span<ColorAttachmentView> colors, LoadOp load_op) :
        Framebuffer(dptr, DepthAttachment(depth, load_op), color_attachments(colors, load_op)) {
}

Framebuffer::~Framebuffer() {
    destroy(_framebuffer);
}

const math::Vec2ui& Framebuffer::size() const {
    return _size;
}

VkFramebuffer Framebuffer::vk_framebuffer() const {
    y_debug_assert(device());
    return _framebuffer;
}

const RenderPass& Framebuffer::render_pass() const {
    return *_render_pass;
}

}

