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
#include "MemoryInfo.h"

#include <editor/context/EditorContext.h>
#include <yave/graphics/utils.h>
#include <yave/graphics/memory/DeviceMemoryAllocator.h>
#include <yave/graphics/descriptors/DescriptorSetAllocator.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

static usize to_kb(usize b) {
    return b / 1024;
}

static float to_mb(usize b) {
    return b / float(1024 * 1024);
}

static const char* memory_type_name(MemoryType type) {
    const char* names[] = {"Generic", "Device local", "Host visible", "Staging"};
    return names[usize(type)];
}

MemoryInfo::MemoryInfo(ContextPtr cptr) : Widget("Memory info", ImGuiWindowFlags_AlwaysAutoResize), ContextLinked(cptr) {
    std::fill(_history.begin(), _history.end(), 0.0f);
}

void MemoryInfo::paint(CmdBufferRecorder&) {
    y_profile();

    usize total_used = 0;
    usize total_allocated = 0;
    for(auto&& [type, heaps] : device_allocator(device()).heaps()) {
        ImGui::BulletText(fmt_c_str("Heap [%]", memory_type_name(type.second)));
        ImGui::Indent();
        for(const auto& heap : heaps) {
            usize free = heap->available();
            const usize used = heap->size() - free;
            total_used += used;
            total_allocated += heap->size();

            ImGui::ProgressBar(used / float(heap->size()), ImVec2(0, 0), fmt_c_str("%KB / %KB", to_kb(used), to_kb(heap->size())));
            ImGui::Text("Free blocks: %u", unsigned(heap->free_blocks()));
            ImGui::Spacing();
        }
        ImGui::Unindent();
    }

    usize dedicated = 0;
    for(auto&& [type, heap] : device_allocator(device()).dedicated_heaps()) {
        unused(type);
        dedicated += heap->allocated_size();
    }
    total_used += dedicated;
    total_allocated += dedicated;

    _max_usage = std::max(_max_usage, total_used);

    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Total used: %.1fMB", to_mb(total_used));
        ImGui::Text("Dedicated allocations size: %.1fMB", to_mb(dedicated));
        ImGui::Text("Total allocated: %.1fMB", to_mb(total_allocated));
        ImGui::SetNextItemWidth(-1);
        ImGui::ProgressBar(total_used / float(total_allocated), ImVec2(0, 0), fmt_c_str("%MB / %MB", usize(to_mb(total_used)), usize(to_mb(total_allocated))));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Max usage: %.1fMB", to_mb(_max_usage));

        if(_timer.elapsed().to_secs() * _history.size() > 60.0) {
            _timer.reset();
            _history[_current_index] = to_mb(total_used);
            _current_index = (_current_index + 1) % _history.size();
        }

        ImGui::SetNextItemWidth(-1);
        ImGui::PlotLines("##graph", _history.data(), _history.size(), _current_index, "", 0.0f, to_mb(_max_usage) * 1.33f, ImVec2(0, 80));
    }

    {
        ImGui::Spacing();
        ImGui::Separator();

        DescriptorSetAllocator& alloc = descriptor_set_allocator(context()->device());
        usize pools = alloc.pool_count();
        const usize total_sets = pools * DescriptorSetPool::pool_size;
        usize free_sets = alloc.free_sets();
        const usize used_sets = total_sets - free_sets;

        ImGui::Text("Descriptor set layouts: %u", u32(alloc.layout_count()));
        ImGui::Text("Descriptor set pools: %u", u32(pools));

        ImGui::ProgressBar(used_sets / float(total_sets), ImVec2(0, 0), fmt_c_str("% / % sets", used_sets, total_sets));
    }
}

}

