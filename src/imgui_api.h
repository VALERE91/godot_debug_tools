#pragma once

#include <godot_cpp/classes/object.hpp>

// GDScript-facing Dear ImGui API, registered as the `ImGui` singleton. Filled in step 3.
class ImGuiAPI : public godot::Object {
	GDCLASS(ImGuiAPI, godot::Object)

protected:
	static void _bind_methods() {}
};
