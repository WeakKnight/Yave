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
#ifndef YAVE_GRAPHICS_IMAGES_IMAGEUSAGE_H
#define YAVE_GRAPHICS_IMAGES_IMAGEUSAGE_H

#include "ImageFormat.h"

#include <algorithm>

namespace yave {

enum class ImageUsage {
	None = 0,

	TextureBit = uenum(vk::ImageUsageFlagBits::eSampled),
	DepthBit = uenum(vk::ImageUsageFlagBits::eDepthStencilAttachment),
	ColorBit = uenum(vk::ImageUsageFlagBits::eColorAttachment),
	StorageBit = uenum(vk::ImageUsageFlagBits::eStorage),

	TransferSrcBit = uenum(vk::ImageUsageFlagBits::eTransferSrc),
	TransferDstBit = uenum(vk::ImageUsageFlagBits::eTransferDst),

#ifdef Y_MSVC
	Y_TODO(MSVC fix)
	SwapchainBit = 0x00000400,
#else
	SwapchainBit = std::max({None, DepthBit, ColorBit, TextureBit, StorageBit}) << 1,
#endif

	// Never use directly:
	Attachment = ColorBit | DepthBit,
	DepthTexture = TextureBit | DepthBit
};

constexpr ImageUsage operator|(ImageUsage l, ImageUsage r) {
	return ImageUsage(uenum(l) | uenum(r));
}

inline ImageUsage operator|(ImageUsage l, vk::ImageUsageFlagBits r) {
	return ImageUsage(uenum(l) | uenum(r));
}

constexpr ImageUsage operator&(ImageUsage l, ImageUsage r)  {
	return ImageUsage(uenum(l) & uenum(r));
}

constexpr ImageUsage operator~(ImageUsage l) {
	return ImageUsage(~uenum(l));
}

constexpr bool is_copy_usage(ImageUsage usage) {
	return (usage & (ImageUsage::TransferDstBit | ImageUsage::TransferSrcBit)) != ImageUsage::None;
}

constexpr bool is_attachment_usage(ImageUsage usage) {
	return (usage & ImageUsage::Attachment) != ImageUsage::None;
}

constexpr bool is_storage_usage(ImageUsage usage) {
	return (usage & ImageUsage::StorageBit) != ImageUsage::None;
}

constexpr bool is_texture_usage(ImageUsage usage) {
	return (usage & ImageUsage::TextureBit) != ImageUsage::None;
}

VkImageLayout vk_image_layout(ImageUsage usage);
vk::ImageLayout vk_image_layout_2(ImageUsage usage);


enum class ImageType {
	TwoD = uenum(vk::ImageViewType::e2D),
	Layered = uenum(vk::ImageViewType::e2DArray),
	Cube = uenum(vk::ImageViewType::eCube),
	ThreeD = uenum(vk::ImageViewType::e3D),
};

}

#endif // YAVE_GRAPHICS_IMAGES_IMAGEUSAGE_H
