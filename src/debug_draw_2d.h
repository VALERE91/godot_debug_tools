#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

// Screen-space debug drawing, registered as the `DebugDraw2D` singleton.
// Draws on ImGui's background draw list: above the game, below ImGui windows, viewport coordinates.
// Immediate mode: call every frame (no duration). graph() feeds a rolling plot shown in a "Graphs" window.
class DebugDraw2D : public godot::Object {
	GDCLASS(DebugDraw2D, godot::Object)

public:
	static DebugDraw2D *singleton;

	DebugDraw2D();
	~DebugDraw2D() override;

	void line(const godot::Vector2 &p_a, const godot::Vector2 &p_b, const godot::Color &p_color, double p_thickness);
	void rect(const godot::Rect2 &p_rect, const godot::Color &p_color, bool p_filled, double p_thickness);
	void circle(const godot::Vector2 &p_center, double p_radius, const godot::Color &p_color, bool p_filled, double p_thickness);
	void text(const godot::Vector2 &p_position, const godot::String &p_text, const godot::Color &p_color);
	void graph(const godot::String &p_name, double p_value);

	// Called by ImGuiAPI right before rendering.
	void draw_graphs();

protected:
	static void _bind_methods();

private:
	static constexpr int GRAPH_SAMPLES = 120;
	static constexpr uint64_t GRAPH_TIMEOUT_USEC = 2000000; // hide graphs not fed for 2 s

	struct Graph {
		std::vector<float> values = std::vector<float>(GRAPH_SAMPLES, 0.0f);
		int head = 0; // next write index (ring buffer)
		uint64_t last_update_usec = 0;
	};
	std::map<std::string, Graph> graphs; // sorted: stable order in the window
};
