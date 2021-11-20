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
#ifndef YAVE_CAMERA_FRUSTUM_H
#define YAVE_CAMERA_FRUSTUM_H

#include <yave/meshes/AABB.h>

#include <array>

namespace yave {

enum class Intersection {
    Inside,
    Intersects,
    Outside
};

class Frustum {

    public:
        Frustum() = default;

        Frustum(const std::array<math::Vec3, 4>& normals, const math::Vec3& pos, const math::Vec3& forward);

        bool is_inside(const math::Vec3& pos, float radius) const;

        Intersection intersection(const AABB& aabb) const;
        Intersection intersection(const AABB& aabb, float far_dist) const;

    private:
        std::array<math::Vec3, 5> _normals;
        math::Vec3 _pos;

};

}

#endif // YAVE_CAMERA_FRUSTUM_H

