#include "debug_draw_2d.h"

#include "imgui_api.h"

#include <imgui.h>

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <cfloat>
#include <cstdio>
#include <iterator>

using namespace godot;

namespace {
ImU32 col32(const Color &p_c) {
	return ImGui::ColorConvertFloat4ToU32(ImVec4(p_c.r, p_c.g, p_c.b, p_c.a));
}

// Background draw list of the current ImGui frame, or null when ImGui is unavailable.
ImDrawList *draw_list() {
	return ImGuiAPI::singleton && ImGuiAPI::singleton->begin_frame() ? ImGui::GetBackgroundDrawList() : nullptr;
}
} // namespace

DebugDraw2D *DebugDraw2D::singleton = nullptr;

DebugDraw2D::DebugDraw2D() {
	singleton = this;
}

DebugDraw2D::~DebugDraw2D() {
	singleton = nullptr;
}

void DebugDraw2D::line(const Vector2 &p_a, const Vector2 &p_b, const Color &p_color, double p_thickness) {
	if (ImDrawList *dl = draw_list()) {
		dl->AddLine(ImVec2(p_a.x, p_a.y), ImVec2(p_b.x, p_b.y), col32(p_color), float(p_thickness));
	}
}

void DebugDraw2D::rect(const Rect2 &p_rect, const Color &p_color, bool p_filled, double p_thickness) {
	if (ImDrawList *dl = draw_list()) {
		const ImVec2 a(p_rect.position.x, p_rect.position.y);
		const ImVec2 b(p_rect.position.x + p_rect.size.x, p_rect.position.y + p_rect.size.y);
		if (p_filled) {
			dl->AddRectFilled(a, b, col32(p_color));
		} else {
			dl->AddRect(a, b, col32(p_color), 0.0f, float(p_thickness));
		}
	}
}

void DebugDraw2D::circle(const Vector2 &p_center, double p_radius, const Color &p_color, bool p_filled, double p_thickness) {
	if (ImDrawList *dl = draw_list()) {
		if (p_filled) {
			dl->AddCircleFilled(ImVec2(p_center.x, p_center.y), float(p_radius), col32(p_color));
		} else {
			dl->AddCircle(ImVec2(p_center.x, p_center.y), float(p_radius), col32(p_color), 0, float(p_thickness));
		}
	}
}

void DebugDraw2D::text(const Vector2 &p_position, const String &p_text, const Color &p_color) {
	if (ImDrawList *dl = draw_list()) {
		dl->AddText(ImVec2(p_position.x, p_position.y), col32(p_color), p_text.utf8().get_data());
	}
}

void DebugDraw2D::graph(const String &p_name, double p_value) {
	if (!draw_list()) { // also starts the frame, so draw_graphs() runs this frame
		return;
	}
	Graph &g = graphs[p_name.utf8().get_data()];
	g.values[g.head] = float(p_value);
	g.head = (g.head + 1) % GRAPH_SAMPLES;
	g.last_update_usec = Time::get_singleton()->get_ticks_usec();
}

void DebugDraw2D::draw_graphs() {
	const uint64_t now = Time::get_singleton()->get_ticks_usec();
	for (auto it = graphs.begin(); it != graphs.end();) {
		it = now - it->second.last_update_usec > GRAPH_TIMEOUT_USEC ? graphs.erase(it) : std::next(it);
	}
	if (graphs.empty()) {
		return;
	}
	const ImVec2 display = ImGui::GetIO().DisplaySize;
	// Anchored top-right every session: a saved position from another resolution would misplace it.
	ImGui::SetNextWindowPos(ImVec2(display.x - 10.0f, 10.0f), ImGuiCond_FirstUseEver, ImVec2(1.0f, 0.0f));
	const ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings;
	if (ImGui::Begin("Graphs", nullptr, flags)) {
		for (const auto &[name, g] : graphs) {
			char overlay[32];
			snprintf(overlay, sizeof(overlay), "%.3f", g.values[(g.head + GRAPH_SAMPLES - 1) % GRAPH_SAMPLES]);
			ImGui::PlotLines(name.c_str(), g.values.data(), GRAPH_SAMPLES, g.head, overlay, FLT_MAX, FLT_MAX, ImVec2(240, 40));
		}
	}
	ImGui::End();
}

void DebugDraw2D::_bind_methods() {
	const Color white(1, 1, 1);
	ClassDB::bind_method(D_METHOD("line", "a", "b", "color", "thickness"), &DebugDraw2D::line, DEFVAL(white), DEFVAL(1.0));
	ClassDB::bind_method(D_METHOD("rect", "rect", "color", "filled", "thickness"), &DebugDraw2D::rect, DEFVAL(white), DEFVAL(false), DEFVAL(1.0));
	ClassDB::bind_method(D_METHOD("circle", "center", "radius", "color", "filled", "thickness"), &DebugDraw2D::circle, DEFVAL(white), DEFVAL(false), DEFVAL(1.0));
	ClassDB::bind_method(D_METHOD("text", "position", "text", "color"), &DebugDraw2D::text, DEFVAL(white));
	ClassDB::bind_method(D_METHOD("graph", "name", "value"), &DebugDraw2D::graph);
}
