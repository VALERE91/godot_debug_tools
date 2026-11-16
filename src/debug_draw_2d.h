#pragma once

#include <godot_cpp/classes/object.hpp>

// Screen-space debug drawing on top of ImGui. Filled in step 3.
class DebugDraw2D : public godot::Object {
	GDCLASS(DebugDraw2D, godot::Object)

protected:
	static void _bind_methods() {}
};
