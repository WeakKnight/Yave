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
#ifndef Y_UTILS_MEMORY_H
#define Y_UTILS_MEMORY_H

#include <y/utils.h>

namespace y {

constexpr usize max_alignment = std::alignment_of<std::max_align_t>::value;

constexpr usize align_up_to(usize value, usize alignment) {
    y_debug_assert(alignment);
    if(const usize diff = value % alignment) {
        y_debug_assert(diff <= value + alignment);
        return value + alignment - diff;
    }
    return value;
}

constexpr usize align_down_to(usize value, usize alignment) {
    y_debug_assert(alignment);
    const usize diff = value % alignment;
    return value - diff;
}

constexpr usize align_up_to_max(usize size) {
    return align_up_to(size, max_alignment);
}

}

#endif // Y_UTILS_MEMORY_H

