// ImGuiAPI: GDScript-facing widget wrappers and bindings. Backend lives in imgui_backend.cpp.
#include "imgui_api.h"

#include <imgui.h>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

#include <cfloat>
#include <cstring>
#include <vector>

using namespace godot;

// Temporary CharString lives until the end of the full expression: safe as a call argument.
#define U8(s) (s).utf8().get_data()

namespace {
ImVec2 iv(const Vector2 &p_v) {
	return ImVec2(p_v.x, p_v.y);
}

ImVec4 iv(const Color &p_c) {
	return ImVec4(p_c.r, p_c.g, p_c.b, p_c.a);
}

// One-element Array used as an out parameter.
bool ref_ok(Array p_ref) {
	ERR_FAIL_COND_V_MSG(p_ref.is_empty(), false, "ImGui: value must be passed as a one-element Array, e.g. [0.5].");
	return true;
}

// Same scale rule as ImGui: min == max means auto-fit.
float scale_or_auto(double p_v, bool p_auto) {
	return p_auto ? FLT_MAX : float(p_v);
}
} // namespace

bool ImGuiAPI::begin(const String &p_name, Array p_open, int64_t p_flags) {
	if (!begin_frame()) {
		return false;
	}
	bool open = p_open.is_empty() ? true : bool(p_open[0]);
	const bool visible = ImGui::Begin(U8(p_name), p_open.is_empty() ? nullptr : &open, ImGuiWindowFlags(p_flags));
	if (!p_open.is_empty()) {
		p_open[0] = open;
	}
	return visible;
}

void ImGuiAPI::end() {
	if (begin_frame()) {
		ImGui::End();
	}
}

void ImGuiAPI::text(const String &p_text) {
	if (begin_frame()) {
		ImGui::TextUnformatted(U8(p_text));
	}
}

void ImGuiAPI::text_colored(const Color &p_color, const String &p_text) {
	if (begin_frame()) {
		ImGui::PushStyleColor(ImGuiCol_Text, iv(p_color));
		ImGui::TextUnformatted(U8(p_text));
		ImGui::PopStyleColor();
	}
}

void ImGuiAPI::text_wrapped(const String &p_text) {
	if (begin_frame()) {
		ImGui::TextWrapped("%s", U8(p_text));
	}
}

bool ImGuiAPI::button(const String &p_label, const Vector2 &p_size) {
	return begin_frame() && ImGui::Button(U8(p_label), iv(p_size));
}

bool ImGuiAPI::checkbox(const String &p_label, Array r_value) {
	if (!begin_frame() || !ref_ok(r_value)) {
		return false;
	}
	bool v = r_value[0];
	const bool changed = ImGui::Checkbox(U8(p_label), &v);
	if (changed) {
		r_value[0] = v;
	}
	return changed;
}

bool ImGuiAPI::slider_float(const String &p_label, Array r_value, double p_min, double p_max, const String &p_format) {
	if (!begin_frame() || !ref_ok(r_value)) {
		return false;
	}
	float v = float(double(r_value[0]));
	const bool changed = ImGui::SliderFloat(U8(p_label), &v, float(p_min), float(p_max), U8(p_format));
	if (changed) {
		r_value[0] = v;
	}
	return changed;
}

bool ImGuiAPI::slider_int(const String &p_label, Array r_value, int64_t p_min, int64_t p_max) {
	if (!begin_frame() || !ref_ok(r_value)) {
		return false;
	}
	int v = int(int64_t(r_value[0]));
	const bool changed = ImGui::SliderInt(U8(p_label), &v, int(p_min), int(p_max));
	if (changed) {
		r_value[0] = v;
	}
	return changed;
}

bool ImGuiAPI::drag_float(const String &p_label, Array r_value, double p_speed, double p_min, double p_max) {
	if (!begin_frame() || !ref_ok(r_value)) {
		return false;
	}
	float v = float(double(r_value[0]));
	const bool changed = ImGui::DragFloat(U8(p_label), &v, float(p_speed), float(p_min), float(p_max));
	if (changed) {
		r_value[0] = v;
	}
	return changed;
}

bool ImGuiAPI::drag_int(const String &p_label, Array r_value, double p_speed, int64_t p_min, int64_t p_max) {
	if (!begin_frame() || !ref_ok(r_value)) {
		return false;
	}
	int v = int(int64_t(r_value[0]));
	const bool changed = ImGui::DragInt(U8(p_label), &v, float(p_speed), int(p_min), int(p_max));
	if (changed) {
		r_value[0] = v;
	}
	return changed;
}

bool ImGuiAPI::input_text(const String &p_label, Array r_value, int64_t p_max_length) {
	if (!begin_frame() || !ref_ok(r_value)) {
		return false;
	}
	const CharString current = String(r_value[0]).utf8();
	std::vector<char> buf(std::max<size_t>(size_t(p_max_length) + 1, size_t(current.length()) + 1), '\0');
	memcpy(buf.data(), current.get_data(), current.length());
	const bool changed = ImGui::InputText(U8(p_label), buf.data(), buf.size());
	if (changed) {
		r_value[0] = String::utf8(buf.data());
	}
	return changed;
}

bool ImGuiAPI::color_edit(const String &p_label, Array r_value) {
	if (!begin_frame() || !ref_ok(r_value)) {
		return false;
	}
	const Color c = r_value[0];
	float rgba[4] = { c.r, c.g, c.b, c.a };
	const bool changed = ImGui::ColorEdit4(U8(p_label), rgba);
	if (changed) {
		r_value[0] = Color(rgba[0], rgba[1], rgba[2], rgba[3]);
	}
	return changed;
}

bool ImGuiAPI::combo(const String &p_label, Array r_current, const PackedStringArray &p_items) {
	if (!begin_frame() || !ref_ok(r_current)) {
		return false;
	}
	std::vector<CharString> storage;
	std::vector<const char *> items;
	storage.reserve(p_items.size());
	for (int64_t i = 0; i < p_items.size(); i++) {
		storage.push_back(p_items[i].utf8());
		items.push_back(storage.back().get_data());
	}
	int current = int(int64_t(r_current[0]));
	const bool changed = ImGui::Combo(U8(p_label), &current, items.data(), int(items.size()));
	if (changed) {
		r_current[0] = current;
	}
	return changed;
}

bool ImGuiAPI::collapsing_header(const String &p_label, int64_t p_flags) {
	return begin_frame() && ImGui::CollapsingHeader(U8(p_label), ImGuiTreeNodeFlags(p_flags));
}

bool ImGuiAPI::tree_node(const String &p_label) {
	return begin_frame() && ImGui::TreeNode(U8(p_label));
}

void ImGuiAPI::tree_pop() {
	if (begin_frame()) {
		ImGui::TreePop();
	}
}

void ImGuiAPI::same_line(double p_offset, double p_spacing) {
	if (begin_frame()) {
		ImGui::SameLine(float(p_offset), float(p_spacing));
	}
}

void ImGuiAPI::separator() {
	if (begin_frame()) {
		ImGui::Separator();
	}
}

void ImGuiAPI::spacing() {
	if (begin_frame()) {
		ImGui::Spacing();
	}
}

void ImGuiAPI::plot_lines(const String &p_label, const PackedFloat32Array &p_values, const String &p_overlay, double p_min, double p_max, const Vector2 &p_size) {
	if (begin_frame()) {
		const bool fit = p_min == p_max;
		ImGui::PlotLines(U8(p_label), p_values.ptr(), int(p_values.size()), 0, p_overlay.is_empty() ? nullptr : U8(p_overlay),
				scale_or_auto(p_min, fit), scale_or_auto(p_max, fit), iv(p_size));
	}
}

void ImGuiAPI::plot_histogram(const String &p_label, const PackedFloat32Array &p_values, const String &p_overlay, double p_min, double p_max, const Vector2 &p_size) {
	if (begin_frame()) {
		const bool fit = p_min == p_max;
		ImGui::PlotHistogram(U8(p_label), p_values.ptr(), int(p_values.size()), 0, p_overlay.is_empty() ? nullptr : U8(p_overlay),
				scale_or_auto(p_min, fit), scale_or_auto(p_max, fit), iv(p_size));
	}
}

void ImGuiAPI::progress_bar(double p_fraction, const Vector2 &p_size, const String &p_overlay) {
	if (begin_frame()) {
		ImGui::ProgressBar(float(p_fraction), iv(p_size), p_overlay.is_empty() ? nullptr : U8(p_overlay));
	}
}

bool ImGuiAPI::begin_table(const String &p_id, int64_t p_columns, int64_t p_flags) {
	return begin_frame() && ImGui::BeginTable(U8(p_id), int(p_columns), ImGuiTableFlags(p_flags));
}

void ImGuiAPI::end_table() {
	if (begin_frame()) {
		ImGui::EndTable();
	}
}

void ImGuiAPI::table_setup_column(const String &p_label) {
	if (begin_frame()) {
		ImGui::TableSetupColumn(U8(p_label));
	}
}

void ImGuiAPI::table_headers_row() {
	if (begin_frame()) {
		ImGui::TableHeadersRow();
	}
}

void ImGuiAPI::table_next_row() {
	if (begin_frame()) {
		ImGui::TableNextRow();
	}
}

bool ImGuiAPI::table_next_column() {
	return begin_frame() && ImGui::TableNextColumn();
}

void ImGuiAPI::image(const Ref<Texture2D> &p_texture, const Vector2 &p_size) {
	if (!begin_frame() || p_texture.is_null()) {
		return;
	}
	frame_textures.push_back(p_texture);
	const Vector2 size = p_size == Vector2() ? p_texture->get_size() : p_size;
	ImGui::Image(ImTextureRef(ImTextureID(uintptr_t(p_texture.ptr()))), iv(size));
}

void ImGuiAPI::set_next_window_pos(const Vector2 &p_pos, int64_t p_cond) {
	if (begin_frame()) {
		ImGui::SetNextWindowPos(iv(p_pos), ImGuiCond(p_cond));
	}
}

void ImGuiAPI::set_next_window_size(const Vector2 &p_size, int64_t p_cond) {
	if (begin_frame()) {
		ImGui::SetNextWindowSize(iv(p_size), ImGuiCond(p_cond));
	}
}

void ImGuiAPI::push_id(const String &p_id) {
	if (begin_frame()) {
		ImGui::PushID(U8(p_id));
	}
}

void ImGuiAPI::pop_id() {
	if (begin_frame()) {
		ImGui::PopID();
	}
}

bool ImGuiAPI::is_item_hovered() {
	return begin_frame() && ImGui::IsItemHovered();
}

void ImGuiAPI::set_tooltip(const String &p_text) {
	if (begin_frame()) {
		ImGui::SetTooltip("%s", U8(p_text));
	}
}

void ImGuiAPI::show_demo_window() {
	if (begin_frame()) {
		ImGui::ShowDemoWindow();
	}
}

bool ImGuiAPI::want_capture_mouse() const {
	return context && drew_last_frame && ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiAPI::want_capture_keyboard() const {
	return context && drew_last_frame && ImGui::GetIO().WantCaptureKeyboard;
}

int64_t ImGuiAPI::get_vertex_count() const {
	return vertex_count;
}

// --- Bindings -------------------------------------------------------------------

void ImGuiAPI::_bind_methods() {
	ClassDB::bind_method(D_METHOD("begin", "name", "open", "flags"), &ImGuiAPI::begin, DEFVAL(Array()), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("end"), &ImGuiAPI::end);
	ClassDB::bind_method(D_METHOD("text", "text"), &ImGuiAPI::text);
	ClassDB::bind_method(D_METHOD("text_colored", "color", "text"), &ImGuiAPI::text_colored);
	ClassDB::bind_method(D_METHOD("text_wrapped", "text"), &ImGuiAPI::text_wrapped);
	ClassDB::bind_method(D_METHOD("button", "label", "size"), &ImGuiAPI::button, DEFVAL(Vector2()));
	ClassDB::bind_method(D_METHOD("checkbox", "label", "value"), &ImGuiAPI::checkbox);
	ClassDB::bind_method(D_METHOD("slider_float", "label", "value", "min", "max", "format"), &ImGuiAPI::slider_float, DEFVAL("%.3f"));
	ClassDB::bind_method(D_METHOD("slider_int", "label", "value", "min", "max"), &ImGuiAPI::slider_int);
	ClassDB::bind_method(D_METHOD("drag_float", "label", "value", "speed", "min", "max"), &ImGuiAPI::drag_float, DEFVAL(1.0), DEFVAL(0.0), DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("drag_int", "label", "value", "speed", "min", "max"), &ImGuiAPI::drag_int, DEFVAL(1.0), DEFVAL(0), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("input_text", "label", "value", "max_length"), &ImGuiAPI::input_text, DEFVAL(256));
	ClassDB::bind_method(D_METHOD("color_edit", "label", "value"), &ImGuiAPI::color_edit);
	ClassDB::bind_method(D_METHOD("combo", "label", "current", "items"), &ImGuiAPI::combo);
	ClassDB::bind_method(D_METHOD("collapsing_header", "label", "flags"), &ImGuiAPI::collapsing_header, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("tree_node", "label"), &ImGuiAPI::tree_node);
	ClassDB::bind_method(D_METHOD("tree_pop"), &ImGuiAPI::tree_pop);
	ClassDB::bind_method(D_METHOD("same_line", "offset", "spacing"), &ImGuiAPI::same_line, DEFVAL(0.0), DEFVAL(-1.0));
	ClassDB::bind_method(D_METHOD("separator"), &ImGuiAPI::separator);
	ClassDB::bind_method(D_METHOD("spacing"), &ImGuiAPI::spacing);
	ClassDB::bind_method(D_METHOD("plot_lines", "label", "values", "overlay", "min", "max", "size"), &ImGuiAPI::plot_lines, DEFVAL(""), DEFVAL(0.0), DEFVAL(0.0), DEFVAL(Vector2(0, 60)));
	ClassDB::bind_method(D_METHOD("plot_histogram", "label", "values", "overlay", "min", "max", "size"), &ImGuiAPI::plot_histogram, DEFVAL(""), DEFVAL(0.0), DEFVAL(0.0), DEFVAL(Vector2(0, 60)));
	ClassDB::bind_method(D_METHOD("progress_bar", "fraction", "size", "overlay"), &ImGuiAPI::progress_bar, DEFVAL(Vector2(-FLT_MIN, 0)), DEFVAL(""));
	ClassDB::bind_method(D_METHOD("begin_table", "id", "columns", "flags"), &ImGuiAPI::begin_table, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("end_table"), &ImGuiAPI::end_table);
	ClassDB::bind_method(D_METHOD("table_setup_column", "label"), &ImGuiAPI::table_setup_column);
	ClassDB::bind_method(D_METHOD("table_headers_row"), &ImGuiAPI::table_headers_row);
	ClassDB::bind_method(D_METHOD("table_next_row"), &ImGuiAPI::table_next_row);
	ClassDB::bind_method(D_METHOD("table_next_column"), &ImGuiAPI::table_next_column);
	ClassDB::bind_method(D_METHOD("image", "texture", "size"), &ImGuiAPI::image, DEFVAL(Vector2()));
	ClassDB::bind_method(D_METHOD("set_next_window_pos", "position", "cond"), &ImGuiAPI::set_next_window_pos, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("set_next_window_size", "size", "cond"), &ImGuiAPI::set_next_window_size, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("push_id", "id"), &ImGuiAPI::push_id);
	ClassDB::bind_method(D_METHOD("pop_id"), &ImGuiAPI::pop_id);
	ClassDB::bind_method(D_METHOD("is_item_hovered"), &ImGuiAPI::is_item_hovered);
	ClassDB::bind_method(D_METHOD("set_tooltip", "text"), &ImGuiAPI::set_tooltip);
	ClassDB::bind_method(D_METHOD("show_demo_window"), &ImGuiAPI::show_demo_window);
	ClassDB::bind_method(D_METHOD("want_capture_mouse"), &ImGuiAPI::want_capture_mouse);
	ClassDB::bind_method(D_METHOD("want_capture_keyboard"), &ImGuiAPI::want_capture_keyboard);
	ClassDB::bind_method(D_METHOD("get_vertex_count"), &ImGuiAPI::get_vertex_count);

	// Flags most useful from GDScript; values are ImGui's own.
	const StringName cls = get_class_static();
	const struct {
		const char *name;
		int64_t value;
	} constants[] = {
		{ "WINDOW_NO_TITLE_BAR", ImGuiWindowFlags_NoTitleBar },
		{ "WINDOW_NO_RESIZE", ImGuiWindowFlags_NoResize },
		{ "WINDOW_NO_MOVE", ImGuiWindowFlags_NoMove },
		{ "WINDOW_NO_COLLAPSE", ImGuiWindowFlags_NoCollapse },
		{ "WINDOW_NO_BACKGROUND", ImGuiWindowFlags_NoBackground },
		{ "WINDOW_NO_INPUTS", ImGuiWindowFlags_NoInputs },
		{ "WINDOW_ALWAYS_AUTO_RESIZE", ImGuiWindowFlags_AlwaysAutoResize },
		{ "WINDOW_NO_SAVED_SETTINGS", ImGuiWindowFlags_NoSavedSettings },
		{ "TREE_NODE_DEFAULT_OPEN", ImGuiTreeNodeFlags_DefaultOpen },
		{ "TABLE_BORDERS", ImGuiTableFlags_Borders },
		{ "TABLE_ROW_BG", ImGuiTableFlags_RowBg },
		{ "TABLE_RESIZABLE", ImGuiTableFlags_Resizable },
		{ "COND_ALWAYS", ImGuiCond_Always },
		{ "COND_ONCE", ImGuiCond_Once },
		{ "COND_FIRST_USE_EVER", ImGuiCond_FirstUseEver },
	};
	for (const auto &c : constants) {
		ClassDB::bind_integer_constant(cls, StringName(), c.name, c.value);
	}
}
