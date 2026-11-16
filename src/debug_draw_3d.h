#pragma once

#include <godot_cpp/classes/object.hpp>

// 3D debug geometry (lines, shapes, text). Filled in step 2.
class DebugDraw3D : public godot::Object {
	GDCLASS(DebugDraw3D, godot::Object)

protected:
	static void _bind_methods() {}
};
