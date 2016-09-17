/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/
#ifndef YAVE_YAVEAPP_H
#define YAVE_YAVEAPP_H

#include <yave/yave.h>

#include <yave/Device.h>

#include <yave/material/MaterialCompiler.h>
#include <yave/command/CmdBufferRecorder.h>
#include <yave/image/Image.h>
#include <yave/mesh/StaticMeshInstance.h>
#include <yave/Swapchain.h>

namespace yave {

class Window;

class YaveApp : NonCopyable {

	struct MVP {
		math::Matrix4<> model;
		math::Matrix4<> view;
		math::Matrix4<> proj;
	};

	public:
		YaveApp(DebugParams params);
		~YaveApp();

		void init(Window* window);

		void draw();
		void update(math::Vec2 angles = math::Vec2(0));


	private:
		void create_graphic_pipeline();
		void create_command_buffers();

		void create_mesh();

		MaterialCompiler* material_compiler;

		Instance instance;
		Device device;

		Swapchain* swapchain;

		GraphicPipeline pipeline;

		CmdBufferPool command_pool;
		core::Vector<RecordedCmdBuffer> command_buffers;

		Texture mesh_texture;
		StaticMeshInstance static_mesh;
		core::Rc<TypedBuffer<MVP, BufferUsage::UniformBuffer>> uniform_buffer;



};

}

#endif // YAVE_YAVEAPP_H