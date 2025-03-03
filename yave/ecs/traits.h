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
#ifndef YAVE_ECS_TRAITS_H
#define YAVE_ECS_TRAITS_H

#include "ecs.h"

#include <y/utils/traits.h>

namespace yave {
namespace ecs {


template<typename T>
struct Mutate {};

template<typename T>
struct Not {};



namespace traits {
template<typename T>
struct component_type {
    using raw_type = remove_cvref_t<T>;
    using type = const raw_type;
    static constexpr bool required = true;
};


template<typename T>
struct component_type<Mutate<T>> {
    using raw_type = typename component_type<T>::raw_type;
    using type = std::remove_const_t<typename component_type<T>::type>;
    static constexpr bool required = component_type<T>::required;
};

template<typename T>
struct component_type<Not<T>> {
    using raw_type = typename component_type<T>::raw_type;
    using type = typename component_type<T>::type;
    static constexpr bool required = !component_type<T>::required;
};




template<typename T>
using component_raw_type_t = typename component_type<T>::raw_type;

template<typename T>
using component_type_t = typename component_type<T>::type;

template<typename T>
static constexpr bool component_required_v = component_type<T>::required;

template<typename T>
static constexpr bool is_component_const_v = std::is_const_v<typename component_type<T>::type> || !component_required_v<T>;
}

}
}

#endif // YAVE_ECS_TRAITS_H
