#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "debug_draw_2d.h"
#include "debug_draw_3d.h"
#include "imgui_api.h"

using namespace godot;

// Engine singletons: GDScript calls them as `DebugDraw3D.line(...)`, `ImGui.button(...)`.
static DebugDraw3D *debug_draw_3d = nullptr;
static DebugDraw2D *debug_draw_2d = nullptr;
static ImGuiAPI *imgui_api = nullptr;

void initialize_debug_tools(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_CLASS(DebugDraw3D);
	GDREGISTER_CLASS(DebugDraw2D);
	GDREGISTER_CLASS(ImGuiAPI);
	GDREGISTER_INTERNAL_CLASS(ImGuiInputNode);

	debug_draw_3d = memnew(DebugDraw3D);
	debug_draw_2d = memnew(DebugDraw2D);
	imgui_api = memnew(ImGuiAPI);
	imgui_api->pre_render = [] { DebugDraw2D::singleton->draw_graphs(); };
	Engine::get_singleton()->register_singleton("DebugDraw3D", debug_draw_3d);
	Engine::get_singleton()->register_singleton("DebugDraw2D", debug_draw_2d);
	Engine::get_singleton()->register_singleton("ImGui", imgui_api);
}

void uninitialize_debug_tools(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	Engine::get_singleton()->unregister_singleton("ImGui");
	Engine::get_singleton()->unregister_singleton("DebugDraw2D");
	Engine::get_singleton()->unregister_singleton("DebugDraw3D");
	memdelete(imgui_api);
	memdelete(debug_draw_2d);
	memdelete(debug_draw_3d);
}

extern "C" {
GDExtensionBool GDE_EXPORT debug_tools_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
	init_obj.register_initializer(initialize_debug_tools);
	init_obj.register_terminator(uninitialize_debug_tools);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
	return init_obj.init();
}
}
