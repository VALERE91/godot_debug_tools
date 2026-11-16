#include "debug_draw_3d.h"

#include "debug_draw_2d.h"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

#include <algorithm>
#include <cmath>

using namespace godot;

namespace {
// Two unit vectors perpendicular to p_dir (and to each other).
void perpendicular_basis(const Vector3 &p_dir, Vector3 &r_u, Vector3 &r_v) {
	const Vector3 up = std::abs(p_dir.y) < 0.99f ? Vector3(0, 1, 0) : Vector3(1, 0, 0);
	r_u = p_dir.cross(up).normalized();
	r_v = p_dir.cross(r_u);
}
} // namespace

DebugDraw3D::DebugDraw3D() {
	enabled = OS::get_singleton()->is_debug_build();
}

// RenderingServer isn't registered yet at extension init, so hook the frame on first use.
// Headless never emits frame_pre_draw: flush on process_frame there, or segments pile up forever
// (dedicated servers).
bool DebugDraw3D::_active() {
	if (enabled && !connected) {
		connected = true;
		const Callable flush = callable_mp(this, &DebugDraw3D::_on_frame_pre_draw);
		SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
		if (DisplayServer::get_singleton()->get_name() == "headless" && tree) {
			tree->connect("process_frame", flush);
		} else {
			RenderingServer::get_singleton()->connect("frame_pre_draw", flush);
		}
	}
	return enabled;
}

DebugDraw3D::~DebugDraw3D() {
	RenderingServer *rs = RenderingServer::get_singleton();
	if (!rs) {
		return;
	}
	if (instance.is_valid()) {
		rs->free_rid(instance);
	}
	if (mesh.is_valid()) {
		rs->free_rid(mesh);
	}
}

// --- Primitives -------------------------------------------------------------

void DebugDraw3D::_push(const Vector3 &p_a, const Vector3 &p_b, const Color &p_color, double p_duration) {
	std::lock_guard<std::mutex> lock(mutex);
	segments.push_back({ p_a, p_b, p_color, p_duration });
}

void DebugDraw3D::_arc(const Vector3 &p_center, const Vector3 &p_u, const Vector3 &p_v, double p_radius,
		double p_from, double p_to, int p_steps, const Color &p_color, double p_duration) {
	Vector3 prev = p_center + (p_u * std::cos(p_from) + p_v * std::sin(p_from)) * p_radius;
	for (int i = 1; i <= p_steps; i++) {
		const double t = p_from + (p_to - p_from) * i / p_steps;
		const Vector3 p = p_center + (p_u * std::cos(t) + p_v * std::sin(t)) * p_radius;
		_push(prev, p, p_color, p_duration);
		prev = p;
	}
}

void DebugDraw3D::line(const Vector3 &p_a, const Vector3 &p_b, const Color &p_color, double p_duration) {
	if (_active()) {
		_push(p_a, p_b, p_color, p_duration);
	}
}

void DebugDraw3D::ray(const Vector3 &p_origin, const Vector3 &p_direction, const Color &p_color, double p_duration) {
	line(p_origin, p_origin + p_direction, p_color, p_duration);
}

void DebugDraw3D::arrow(const Vector3 &p_from, const Vector3 &p_to, const Color &p_color, double p_duration) {
	if (!_active()) {
		return;
	}
	_push(p_from, p_to, p_color, p_duration);
	const Vector3 dir = p_to - p_from;
	const real_t len = dir.length();
	if (len < CMP_EPSILON) {
		return;
	}
	const Vector3 d = dir / len;
	Vector3 u, v;
	perpendicular_basis(d, u, v);
	const real_t head = std::min<real_t>(len * 0.2f, 0.5f);
	const Vector3 base = p_to - d * head;
	for (const Vector3 &side : { u, -u, v, -v }) {
		_push(p_to, base + side * head * 0.4f, p_color, p_duration);
	}
}

void DebugDraw3D::path(const PackedVector3Array &p_points, const Color &p_color, double p_duration) {
	if (!_active()) {
		return;
	}
	for (int64_t i = 1; i < p_points.size(); i++) {
		_push(p_points[i - 1], p_points[i], p_color, p_duration);
	}
}

void DebugDraw3D::point(const Vector3 &p_position, double p_size, const Color &p_color, double p_duration) {
	if (!_active()) {
		return;
	}
	const real_t h = p_size * 0.5;
	for (const Vector3 &axis : { Vector3(h, 0, 0), Vector3(0, h, 0), Vector3(0, 0, h) }) {
		_push(p_position - axis, p_position + axis, p_color, p_duration);
	}
}

void DebugDraw3D::aabb(const AABB &p_aabb, const Color &p_color, double p_duration) {
	box(Transform3D(Basis::from_scale(p_aabb.size), p_aabb.get_center()), p_color, p_duration);
}

void DebugDraw3D::box(const Transform3D &p_xform, const Color &p_color, double p_duration) {
	if (!_active()) {
		return;
	}
	// Unit cube corners indexed by bits (x=1, y=2, z=4); an edge joins corners differing by one bit.
	Vector3 corners[8];
	for (int i = 0; i < 8; i++) {
		corners[i] = p_xform.xform(Vector3(i & 1 ? 0.5 : -0.5, i & 2 ? 0.5 : -0.5, i & 4 ? 0.5 : -0.5));
	}
	for (int i = 0; i < 8; i++) {
		for (int bit = 1; bit < 8; bit <<= 1) {
			if (!(i & bit)) {
				_push(corners[i], corners[i | bit], p_color, p_duration);
			}
		}
	}
}

void DebugDraw3D::sphere(const Vector3 &p_center, double p_radius, const Color &p_color, double p_duration) {
	if (!_active()) {
		return;
	}
	const Vector3 x(1, 0, 0), y(0, 1, 0), z(0, 0, 1);
	_arc(p_center, x, y, p_radius, 0, Math::TAU, CIRCLE_STEPS, p_color, p_duration);
	_arc(p_center, y, z, p_radius, 0, Math::TAU, CIRCLE_STEPS, p_color, p_duration);
	_arc(p_center, z, x, p_radius, 0, Math::TAU, CIRCLE_STEPS, p_color, p_duration);
}

void DebugDraw3D::capsule(const Vector3 &p_a, const Vector3 &p_b, double p_radius, const Color &p_color, double p_duration) {
	if (!_active()) {
		return;
	}
	const Vector3 axis = p_b - p_a;
	const real_t len = axis.length();
	if (len < CMP_EPSILON) {
		sphere(p_a, p_radius, p_color, p_duration);
		return;
	}
	const Vector3 d = axis / len;
	Vector3 u, v;
	perpendicular_basis(d, u, v);
	_arc(p_a, u, v, p_radius, 0, Math::TAU, CIRCLE_STEPS, p_color, p_duration);
	_arc(p_b, u, v, p_radius, 0, Math::TAU, CIRCLE_STEPS, p_color, p_duration);
	for (const Vector3 &side : { u, -u, v, -v }) {
		_push(p_a + side * p_radius, p_b + side * p_radius, p_color, p_duration);
	}
	// Hemispheres: half circles from +side through the pole to -side.
	for (const Vector3 &side : { u, v }) {
		_arc(p_b, side, d, p_radius, 0, Math::PI, CIRCLE_STEPS / 2, p_color, p_duration);
		_arc(p_a, side, -d, p_radius, 0, Math::PI, CIRCLE_STEPS / 2, p_color, p_duration);
	}
}

void DebugDraw3D::text(const Vector3 &p_position, const String &p_text, const Color &p_color) {
	SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	Camera3D *camera = tree && tree->get_root() ? tree->get_root()->get_camera_3d() : nullptr;
	if (!enabled || !DebugDraw2D::singleton || !camera || camera->is_position_behind(p_position)) {
		return;
	}
	DebugDraw2D::singleton->text(camera->unproject_position(p_position), p_text, p_color);
}

void DebugDraw3D::clear() {
	std::lock_guard<std::mutex> lock(mutex);
	segments.clear();
}

void DebugDraw3D::set_depth_test(bool p_enabled) {
	depth_test = p_enabled;
	if (material.is_valid()) {
		material->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, !depth_test);
	}
}

bool DebugDraw3D::get_depth_test() const {
	return depth_test;
}

int64_t DebugDraw3D::get_line_count() const {
	std::lock_guard<std::mutex> lock(mutex);
	return last_line_count;
}

// --- Rendering ----------------------------------------------------------------

bool DebugDraw3D::_ensure_instance() {
	if (instance.is_valid()) {
		return true;
	}
	SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	if (!tree || !tree->get_root()) {
		return false;
	}
	const Ref<World3D> world = tree->get_root()->get_world_3d();
	if (world.is_null()) {
		return false;
	}
	material.instantiate();
	material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
	material->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
	material->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	material->set_flag(BaseMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);
	material->set_flag(BaseMaterial3D::FLAG_DISABLE_FOG, true);
	material->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, !depth_test);

	RenderingServer *rs = RenderingServer::get_singleton();
	mesh = rs->mesh_create();
	instance = rs->instance_create2(mesh, world->get_scenario());
	rs->instance_geometry_set_cast_shadows_setting(instance, RenderingServer::SHADOW_CASTING_SETTING_OFF);
	return true;
}

void DebugDraw3D::_on_frame_pre_draw() {
	const uint64_t now = Time::get_singleton()->get_ticks_usec();
	const double dt = last_frame_usec ? (now - last_frame_usec) * 1e-6 : 0.0;
	last_frame_usec = now;

	PackedVector3Array vertices;
	PackedColorArray colors;
	{
		std::lock_guard<std::mutex> lock(mutex);
		last_line_count = int64_t(segments.size());
		vertices.resize(last_line_count * 2);
		colors.resize(last_line_count * 2);
		Vector3 *v = vertices.ptrw();
		Color *c = colors.ptrw();
		for (const Segment &s : segments) {
			*v++ = s.a;
			*v++ = s.b;
			*c++ = s.color;
			*c++ = s.color;
		}
		// Everything was drawn once; age it and drop what expired (duration 0 goes now).
		segments.erase(std::remove_if(segments.begin(), segments.end(), [dt](Segment &s) { return (s.remaining -= dt) <= 0.0; }), segments.end());
	}

	if (vertices.is_empty() && !instance.is_valid()) {
		return; // never drew anything: don't create render resources
	}
	if (!_ensure_instance()) {
		return;
	}
	RenderingServer *rs = RenderingServer::get_singleton();
	rs->mesh_clear(mesh);
	if (vertices.is_empty()) {
		return;
	}
	Array arrays;
	arrays.resize(RenderingServer::ARRAY_MAX);
	arrays[RenderingServer::ARRAY_VERTEX] = vertices;
	arrays[RenderingServer::ARRAY_COLOR] = colors;
	rs->mesh_add_surface_from_arrays(mesh, RenderingServer::PRIMITIVE_LINES, arrays);
	rs->mesh_surface_set_material(mesh, 0, material->get_rid());
}

// --- Bindings -----------------------------------------------------------------

void DebugDraw3D::_bind_methods() {
	const Color red(1, 0, 0);
	ClassDB::bind_method(D_METHOD("line", "a", "b", "color", "duration"), &DebugDraw3D::line, DEFVAL(red), DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("ray", "origin", "direction", "color", "duration"), &DebugDraw3D::ray, DEFVAL(red), DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("arrow", "from", "to", "color", "duration"), &DebugDraw3D::arrow, DEFVAL(red), DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("path", "points", "color", "duration"), &DebugDraw3D::path, DEFVAL(red), DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("point", "position", "size", "color", "duration"), &DebugDraw3D::point, DEFVAL(0.25), DEFVAL(red), DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("aabb", "aabb", "color", "duration"), &DebugDraw3D::aabb, DEFVAL(red), DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("box", "xform", "color", "duration"), &DebugDraw3D::box, DEFVAL(red), DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("sphere", "center", "radius", "color", "duration"), &DebugDraw3D::sphere, DEFVAL(0.5), DEFVAL(red), DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("capsule", "a", "b", "radius", "color", "duration"), &DebugDraw3D::capsule, DEFVAL(0.5), DEFVAL(red), DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("text", "position", "text", "color"), &DebugDraw3D::text, DEFVAL(Color(1, 1, 1)));
	ClassDB::bind_method(D_METHOD("clear"), &DebugDraw3D::clear);
	ClassDB::bind_method(D_METHOD("set_depth_test", "enabled"), &DebugDraw3D::set_depth_test);
	ClassDB::bind_method(D_METHOD("get_depth_test"), &DebugDraw3D::get_depth_test);
	ClassDB::bind_method(D_METHOD("get_line_count"), &DebugDraw3D::get_line_count);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "depth_test"), "set_depth_test", "get_depth_test");
}
