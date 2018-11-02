
#include "MainWindow.h"

#include <y/io/File.h>

#include <yave/framegraph/renderers.h>

using namespace editor;


int test_graph() {
	Instance instance(DebugParams::debug());
	Device device(instance);
	Scene scene;

	{
		FrameGraph graph(&device);

		auto& gbuffer = render_gbuffer(graph, SceneView(scene), math::Vec2ui(512));

		{
			CmdBufferRecorder rec(device.create_disposable_cmd_buffer());
			std::move(graph).render(rec, gbuffer.color);
			RecordedCmdBuffer cmd(std::move(rec));
			device.queue(vk::QueueFlagBits::eGraphics).submit<SyncSubmit>(std::move(cmd));
		}
	}

	return 4;
}

int main(int argc, char** argv) {
	perf::set_output(std::move(io::File::create("perfdump.json").unwrap()));


	/*log_msg("Testing frame graph");

	return test_graph();*/




	bool debug = true;
	for(std::string_view arg : core::ArrayView<const char*>(argv, argc)) {
		if(arg == "--nodebug") {
			log_msg("Vulkan debugging disabled", Log::Warning);
			debug = false;
		}
	}

	Instance instance(debug ? DebugParams::debug() : DebugParams::none());
	Device device(instance);

	EditorContext ctx(&device);

	MainWindow window(&ctx);
	window.exec();

	return 0;
}
