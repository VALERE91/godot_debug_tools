#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <cstdint>
#include <mutex>
#include <vector>

// Immediate-mode 3D debug lines, registered as the `DebugDraw3D` singleton.
// Everything is batched into one PRIMITIVE_LINES mesh rebuilt on RenderingServer.frame_pre_draw.
// duration == 0 draws for exactly one rendered frame; > 0 keeps the shape for that many seconds.
// Draws into the root viewport's World3D only. No-op in non-debug builds.
class DebugDraw3D : public godot::Object {
	GDCLASS(DebugDraw3D, godot::Object)

public:
	DebugDraw3D();
	~DebugDraw3D() override;

	void line(const godot::Vector3 &p_a, const godot::Vector3 &p_b, const godot::Color &p_color, double p_duration);
	void ray(const godot::Vector3 &p_origin, const godot::Vector3 &p_direction, const godot::Color &p_color, double p_duration);
	void arrow(const godot::Vector3 &p_from, const godot::Vector3 &p_to, const godot::Color &p_color, double p_duration);
	void path(const godot::PackedVector3Array &p_points, const godot::Color &p_color, double p_duration);
	void point(const godot::Vector3 &p_position, double p_size, const godot::Color &p_color, double p_duration);
	void aabb(const godot::AABB &p_aabb, const godot::Color &p_color, double p_duration);
	void box(const godot::Transform3D &p_xform, const godot::Color &p_color, double p_duration);
	void sphere(const godot::Vector3 &p_center, double p_radius, const godot::Color &p_color, double p_duration);
	void capsule(const godot::Vector3 &p_a, const godot::Vector3 &p_b, double p_radius, const godot::Color &p_color, double p_duration);
	// Screen-space label at a world position (via DebugDraw2D). Per frame only, no duration.
	void text(const godot::Vector3 &p_position, const godot::String &p_text, const godot::Color &p_color);
	void clear();

	void set_depth_test(bool p_enabled);
	bool get_depth_test() const;
	// Segments submitted to the renderer on the last frame (for tests and HUDs).
	int64_t get_line_count() const;

protected:
	static void _bind_methods();

private:
	struct Segment {
		godot::Vector3 a;
		godot::Vector3 b;
		godot::Color color;
		double remaining;
	};

	static constexpr int CIRCLE_STEPS = 24;

	void _push(const godot::Vector3 &p_a, const godot::Vector3 &p_b, const godot::Color &p_color, double p_duration);
	void _arc(const godot::Vector3 &p_center, const godot::Vector3 &p_u, const godot::Vector3 &p_v, double p_radius,
			double p_from, double p_to, int p_steps, const godot::Color &p_color, double p_duration);
	bool _active();
	bool _ensure_instance();
	void _on_frame_pre_draw();

	mutable std::mutex mutex;
	std::vector<Segment> segments;
	int64_t last_line_count = 0;
	uint64_t last_frame_usec = 0;
	bool depth_test = true;
	bool enabled = false;
	bool connected = false;

	godot::RID mesh;
	godot::RID instance;
	godot::Ref<godot::StandardMaterial3D> material;
};
