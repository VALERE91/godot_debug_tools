#pragma once

#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/rid.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

struct ImGuiContext;
struct ImTextureData;
struct ImDrawData;

// Forwards input and focus to ImGuiAPI. Added to the scene root on first ImGui use.
class ImGuiInputNode : public godot::Node {
	GDCLASS(ImGuiInputNode, godot::Node)

public:
	void _input(const godot::Ref<godot::InputEvent> &p_event) override;
	void _notification(int p_what);

protected:
	static void _bind_methods() {}
};

// Dear ImGui for GDScript, registered as the `ImGui` singleton.
// Immediate mode, main thread only: call widgets from _process(). The frame starts on the first call
// and is rendered on RenderingServer.frame_pre_draw into a canvas on top of the root viewport.
// "By reference" values use a one-element Array: `var v := [0.5]; ImGui.slider_float("x", v, 0, 1)`.
// No-op in non-debug builds and in the editor.
class ImGuiAPI : public godot::Object {
	GDCLASS(ImGuiAPI, godot::Object)

public:
	static ImGuiAPI *singleton;

	ImGuiAPI();
	~ImGuiAPI() override;

	// Starts the ImGui frame if needed; false when ImGui is unavailable. Every widget calls it.
	bool begin_frame();
	// Called right before ImGui::Render() (DebugDraw2D graphs window).
	std::function<void()> pre_render;

	// --- Widgets (imgui_api.cpp) ---
	bool begin(const godot::String &p_name, godot::Array p_open, int64_t p_flags);
	void end();
	void text(const godot::String &p_text);
	void text_colored(const godot::Color &p_color, const godot::String &p_text);
	void text_wrapped(const godot::String &p_text);
	bool button(const godot::String &p_label, const godot::Vector2 &p_size);
	bool checkbox(const godot::String &p_label, godot::Array r_value);
	bool slider_float(const godot::String &p_label, godot::Array r_value, double p_min, double p_max, const godot::String &p_format);
	bool slider_int(const godot::String &p_label, godot::Array r_value, int64_t p_min, int64_t p_max);
	bool drag_float(const godot::String &p_label, godot::Array r_value, double p_speed, double p_min, double p_max);
	bool drag_int(const godot::String &p_label, godot::Array r_value, double p_speed, int64_t p_min, int64_t p_max);
	bool input_text(const godot::String &p_label, godot::Array r_value, int64_t p_max_length);
	bool color_edit(const godot::String &p_label, godot::Array r_value);
	bool combo(const godot::String &p_label, godot::Array r_current, const godot::PackedStringArray &p_items);
	bool collapsing_header(const godot::String &p_label, int64_t p_flags);
	bool tree_node(const godot::String &p_label);
	void tree_pop();
	void same_line(double p_offset, double p_spacing);
	void separator();
	void spacing();
	void plot_lines(const godot::String &p_label, const godot::PackedFloat32Array &p_values, const godot::String &p_overlay, double p_min, double p_max, const godot::Vector2 &p_size);
	void plot_histogram(const godot::String &p_label, const godot::PackedFloat32Array &p_values, const godot::String &p_overlay, double p_min, double p_max, const godot::Vector2 &p_size);
	void progress_bar(double p_fraction, const godot::Vector2 &p_size, const godot::String &p_overlay);
	bool begin_table(const godot::String &p_id, int64_t p_columns, int64_t p_flags);
	void end_table();
	void table_setup_column(const godot::String &p_label);
	void table_headers_row();
	void table_next_row();
	bool table_next_column();
	void image(const godot::Ref<godot::Texture2D> &p_texture, const godot::Vector2 &p_size);
	void set_next_window_pos(const godot::Vector2 &p_pos, int64_t p_cond);
	void set_next_window_size(const godot::Vector2 &p_size, int64_t p_cond);
	void push_id(const godot::String &p_id);
	void pop_id();
	bool is_item_hovered();
	void set_tooltip(const godot::String &p_text);
	void show_demo_window();
	bool want_capture_mouse() const;
	bool want_capture_keyboard() const;
	// Vertices rendered on the last frame (tests, HUDs).
	int64_t get_vertex_count() const;

	// --- Backend (imgui_backend.cpp) ---
	void _input_event(const godot::Ref<godot::InputEvent> &p_event, godot::Node *p_receiver);
	void _focus_changed(bool p_focused);

protected:
	static void _bind_methods();

private:
	bool _init_context();
	void _flush();
	void _update_texture(ImTextureData *p_tex);
	void _render(ImDrawData *p_draw_data);
	godot::RID _texture_rid(uint64_t p_tex_id) const;

	bool enabled = false;
	bool in_frame = false;
	bool drew_last_frame = false; // input capture only applies while UI is on screen
	ImGuiContext *context = nullptr;
	uint64_t last_frame_usec = 0;
	int64_t vertex_count = 0;
	std::string ini_path;

	godot::RID canvas;
	std::vector<godot::RID> items;
	std::unordered_map<int, godot::Ref<godot::ImageTexture>> textures; // ImTextureData::UniqueID -> texture
	std::vector<godot::Ref<godot::Texture2D>> frame_textures;         // user images kept alive until rendered
};
