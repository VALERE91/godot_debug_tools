// ImGuiAPI backend: context, frame lifecycle, input, textures, rendering through RenderingServer canvas items.
#include "imgui_api.h"

#include <imgui.h>

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>

using namespace godot;

// --- imgui_config.h hooks -------------------------------------------------------

void debug_tools_imgui_assert(const char *p_expr, const char *p_file, int p_line) {
	ERR_PRINT(vformat("ImGui assertion failed: %s (%s:%d)", p_expr, p_file, p_line));
}

void debug_tools_imgui_log(const char *p_fmt, ...) {
	char buf[1024];
	va_list args;
	va_start(args, p_fmt);
	vsnprintf(buf, sizeof(buf), p_fmt, args);
	va_end(args);
	String msg = String::utf8(buf).strip_edges();
	if (!msg.is_empty()) {
		UtilityFunctions::push_warning("ImGui: ", msg); // error recovery reports (Begin/End mismatch...)
	}
}

namespace {
SceneTree *get_tree() {
	return Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
}

ImGuiKey to_imgui_key(Key p_key) {
	if (p_key >= KEY_A && p_key <= KEY_Z) {
		return ImGuiKey(ImGuiKey_A + (p_key - KEY_A));
	}
	if (p_key >= KEY_0 && p_key <= KEY_9) {
		return ImGuiKey(ImGuiKey_0 + (p_key - KEY_0));
	}
	if (p_key >= KEY_F1 && p_key <= KEY_F12) {
		return ImGuiKey(ImGuiKey_F1 + (p_key - KEY_F1));
	}
	switch (p_key) {
		case KEY_TAB: return ImGuiKey_Tab;
		case KEY_LEFT: return ImGuiKey_LeftArrow;
		case KEY_RIGHT: return ImGuiKey_RightArrow;
		case KEY_UP: return ImGuiKey_UpArrow;
		case KEY_DOWN: return ImGuiKey_DownArrow;
		case KEY_PAGEUP: return ImGuiKey_PageUp;
		case KEY_PAGEDOWN: return ImGuiKey_PageDown;
		case KEY_HOME: return ImGuiKey_Home;
		case KEY_END: return ImGuiKey_End;
		case KEY_INSERT: return ImGuiKey_Insert;
		case KEY_DELETE: return ImGuiKey_Delete;
		case KEY_BACKSPACE: return ImGuiKey_Backspace;
		case KEY_SPACE: return ImGuiKey_Space;
		case KEY_ENTER: return ImGuiKey_Enter;
		case KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
		case KEY_ESCAPE: return ImGuiKey_Escape;
		case KEY_CTRL: return ImGuiKey_LeftCtrl;
		case KEY_SHIFT: return ImGuiKey_LeftShift;
		case KEY_ALT: return ImGuiKey_LeftAlt;
		case KEY_META: return ImGuiKey_LeftSuper;
		case KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
		case KEY_COMMA: return ImGuiKey_Comma;
		case KEY_MINUS: return ImGuiKey_Minus;
		case KEY_PERIOD: return ImGuiKey_Period;
		case KEY_SLASH: return ImGuiKey_Slash;
		case KEY_SEMICOLON: return ImGuiKey_Semicolon;
		case KEY_EQUAL: return ImGuiKey_Equal;
		case KEY_BRACKETLEFT: return ImGuiKey_LeftBracket;
		case KEY_BACKSLASH: return ImGuiKey_Backslash;
		case KEY_BRACKETRIGHT: return ImGuiKey_RightBracket;
		case KEY_QUOTELEFT: return ImGuiKey_GraveAccent;
		default: return ImGuiKey_None;
	}
}

std::string clipboard_buffer;

const char *get_clipboard(ImGuiContext *) {
	clipboard_buffer = DisplayServer::get_singleton()->clipboard_get().utf8().get_data();
	return clipboard_buffer.c_str();
}

void set_clipboard(ImGuiContext *, const char *p_text) {
	DisplayServer::get_singleton()->clipboard_set(String::utf8(p_text));
}
} // namespace

// --- Input node -------------------------------------------------------------------

void ImGuiInputNode::_input(const Ref<InputEvent> &p_event) {
	if (ImGuiAPI::singleton) {
		ImGuiAPI::singleton->_input_event(p_event, this);
	}
}

void ImGuiInputNode::_notification(int p_what) {
	if (!ImGuiAPI::singleton) {
		return;
	}
	if (p_what == NOTIFICATION_APPLICATION_FOCUS_IN || p_what == NOTIFICATION_APPLICATION_FOCUS_OUT) {
		ImGuiAPI::singleton->_focus_changed(p_what == NOTIFICATION_APPLICATION_FOCUS_IN);
	}
}

// --- Lifecycle ----------------------------------------------------------------------

ImGuiAPI *ImGuiAPI::singleton = nullptr;

ImGuiAPI::ImGuiAPI() {
	singleton = this;
	enabled = OS::get_singleton()->is_debug_build() && !Engine::get_singleton()->is_editor_hint();
}

ImGuiAPI::~ImGuiAPI() {
	if (context) {
		ImGui::DestroyContext(context);
	}
	RenderingServer *rs = RenderingServer::get_singleton();
	if (rs) {
		for (const RID &item : items) {
			rs->free_rid(item);
		}
		if (canvas.is_valid()) {
			rs->free_rid(canvas);
		}
	}
	singleton = nullptr;
}

bool ImGuiAPI::_init_context() {
	SceneTree *tree = get_tree();
	if (!tree || !tree->get_root()) {
		return false;
	}
	context = ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.BackendPlatformName = "godot";
	io.BackendRendererName = "godot_canvas";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures | ImGuiBackendFlags_RendererHasVtxOffset;
	io.ConfigErrorRecoveryEnableAssert = false; // recover from Begin/End mismatches with a warning
	ini_path = ProjectSettings::get_singleton()->globalize_path("user://imgui.ini").utf8().get_data();
	io.IniFilename = ini_path.c_str();
	ImGuiPlatformIO &pio = ImGui::GetPlatformIO();
	pio.Platform_GetClipboardTextFn = get_clipboard;
	pio.Platform_SetClipboardTextFn = set_clipboard;

	// Render on frame_pre_draw; headless never draws, so flush on process_frame there (see DebugDraw3D).
	const Callable flush = callable_mp(this, &ImGuiAPI::_flush);
	if (DisplayServer::get_singleton()->get_name() == "headless") {
		tree->connect("process_frame", flush);
	} else {
		RenderingServer::get_singleton()->connect("frame_pre_draw", flush);
	}

	RenderingServer *rs = RenderingServer::get_singleton();
	canvas = rs->canvas_create();
	const RID viewport = tree->get_root()->get_viewport_rid();
	rs->viewport_attach_canvas(viewport, canvas);
	rs->viewport_set_canvas_stacking(viewport, canvas, 1000, 0); // above every CanvasLayer (max 128)

	ImGuiInputNode *input = memnew(ImGuiInputNode);
	input->set_name("ImGuiInput");
	input->set_process_mode(Node::PROCESS_MODE_ALWAYS);
	tree->get_root()->call_deferred("add_child", input); // may be called mid-tree-update
	return true;
}

bool ImGuiAPI::begin_frame() {
	if (!enabled) {
		return false;
	}
	if (in_frame) {
		return true;
	}
	if (!context && !_init_context()) {
		return false;
	}
	ImGui::SetCurrentContext(context);
	ImGuiIO &io = ImGui::GetIO();
	const uint64_t now = Time::get_singleton()->get_ticks_usec();
	io.DeltaTime = last_frame_usec ? std::max((now - last_frame_usec) * 1e-6f, 1e-5f) : 1.0f / 60.0f;
	last_frame_usec = now;
	const Vector2 size = get_tree()->get_root()->get_visible_rect().size;
	io.DisplaySize = ImVec2(size.x, size.y);
	ImGui::NewFrame();
	in_frame = true;
	return true;
}

void ImGuiAPI::_flush() {
	if (!in_frame) {
		if (drew_last_frame) { // UI disappeared: clear it once, stop capturing input
			RenderingServer *rs = RenderingServer::get_singleton();
			for (const RID &item : items) {
				rs->canvas_item_clear(item);
			}
			vertex_count = 0;
			drew_last_frame = false;
		}
		return;
	}
	ImGui::SetCurrentContext(context);
	if (pre_render) {
		pre_render();
	}
	ImGui::Render();
	in_frame = false;
	ImDrawData *draw_data = ImGui::GetDrawData();
	if (draw_data->Textures) {
		for (ImTextureData *tex : *draw_data->Textures) {
			if (tex->Status != ImTextureStatus_OK) {
				_update_texture(tex);
			}
		}
	}
	_render(draw_data);
	frame_textures.clear();
	drew_last_frame = true;
}

// --- Textures (ImGuiBackendFlags_RendererHasTextures protocol) -------------------------

void ImGuiAPI::_update_texture(ImTextureData *p_tex) {
	if (p_tex->Status == ImTextureStatus_WantDestroy && p_tex->UnusedFrames > 0) {
		textures.erase(p_tex->UniqueID);
		p_tex->SetTexID(ImTextureID_Invalid);
		p_tex->SetStatus(ImTextureStatus_Destroyed);
		return;
	}
	if (p_tex->Status != ImTextureStatus_WantCreate && p_tex->Status != ImTextureStatus_WantUpdates) {
		return;
	}
	PackedByteArray bytes;
	bytes.resize(int64_t(p_tex->Width) * p_tex->Height * 4);
	uint8_t *dst = bytes.ptrw();
	const uint8_t *src = static_cast<const uint8_t *>(p_tex->GetPixels());
	if (p_tex->Format == ImTextureFormat_RGBA32) {
		memcpy(dst, src, bytes.size());
	} else { // Alpha8 -> white with alpha
		for (int64_t i = 0; i < int64_t(p_tex->Width) * p_tex->Height; i++) {
			dst[i * 4 + 0] = dst[i * 4 + 1] = dst[i * 4 + 2] = 255;
			dst[i * 4 + 3] = src[i];
		}
	}
	const Ref<Image> image = Image::create_from_data(p_tex->Width, p_tex->Height, false, Image::FORMAT_RGBA8, bytes);
	// shortcut: re-uploads the whole texture on WantUpdates instead of p_tex->Updates rects; fine for a
	// font atlas that changes rarely. Upgrade if dynamic font sizes cause hitches.
	auto it = textures.find(p_tex->UniqueID);
	if (it != textures.end() && it->second->get_size() == Vector2(p_tex->Width, p_tex->Height)) {
		it->second->update(image);
	} else {
		const Ref<ImageTexture> texture = ImageTexture::create_from_image(image);
		textures[p_tex->UniqueID] = texture;
		p_tex->SetTexID(ImTextureID(uintptr_t(static_cast<Texture2D *>(texture.ptr()))));
	}
	p_tex->SetStatus(ImTextureStatus_OK);
}

// ImTextureID is a Texture2D* (atlas textures above, user images kept alive in frame_textures).
RID ImGuiAPI::_texture_rid(uint64_t p_tex_id) const {
	const Texture2D *texture = reinterpret_cast<const Texture2D *>(uintptr_t(p_tex_id));
	return texture ? texture->get_rid() : RID();
}

// --- Rendering ------------------------------------------------------------------------

void ImGuiAPI::_render(ImDrawData *p_draw_data) {
	RenderingServer *rs = RenderingServer::get_singleton();
	for (const RID &item : items) {
		rs->canvas_item_clear(item);
	}
	vertex_count = p_draw_data->TotalVtxCount;
	const ImVec2 origin = p_draw_data->DisplayPos;
	size_t used = 0;

	for (const ImDrawList *list : p_draw_data->CmdLists) {
		const int vtx_count = list->VtxBuffer.Size;
		PackedVector2Array points;
		PackedVector2Array uvs;
		PackedColorArray colors;
		points.resize(vtx_count);
		uvs.resize(vtx_count);
		colors.resize(vtx_count);
		Vector2 *p = points.ptrw();
		Vector2 *uv = uvs.ptrw();
		Color *c = colors.ptrw();
		for (int i = 0; i < vtx_count; i++) {
			const ImDrawVert &v = list->VtxBuffer[i];
			p[i] = Vector2(v.pos.x - origin.x, v.pos.y - origin.y);
			uv[i] = Vector2(v.uv.x, v.uv.y);
			const ImVec4 col = ImGui::ColorConvertU32ToFloat4(v.col);
			c[i] = Color(col.x, col.y, col.z, col.w);
		}

		for (const ImDrawCmd &cmd : list->CmdBuffer) {
			if (cmd.UserCallback) {
				if (cmd.UserCallback != ImGui::GetPlatformIO().DrawCallback_ResetRenderState) {
					cmd.UserCallback(list, &cmd);
				}
				continue;
			}
			if (cmd.ElemCount == 0) {
				continue;
			}
			PackedInt32Array indices;
			indices.resize(cmd.ElemCount);
			int32_t *idx = indices.ptrw();
			for (unsigned int i = 0; i < cmd.ElemCount; i++) {
				idx[i] = int32_t(list->IdxBuffer[cmd.IdxOffset + i] + cmd.VtxOffset);
			}
			// One canvas item per draw command: each needs its own clip rect.
			if (used == items.size()) {
				const RID item = rs->canvas_item_create();
				rs->canvas_item_set_parent(item, canvas);
				rs->canvas_item_set_draw_index(item, int32_t(used));
				rs->canvas_item_set_clip(item, true);
				items.push_back(item);
			}
			const RID item = items[used++];
			const Rect2 clip(cmd.ClipRect.x - origin.x, cmd.ClipRect.y - origin.y,
					cmd.ClipRect.z - cmd.ClipRect.x, cmd.ClipRect.w - cmd.ClipRect.y);
			rs->canvas_item_set_custom_rect(item, true, clip);
			rs->canvas_item_add_triangle_array(item, indices, points, colors, uvs, PackedInt32Array(),
					PackedFloat32Array(), _texture_rid(cmd.GetTexID()));
		}
	}
}

// --- Input ------------------------------------------------------------------------------

void ImGuiAPI::_input_event(const Ref<InputEvent> &p_event, Node *p_receiver) {
	if (!context) {
		return;
	}
	ImGui::SetCurrentContext(context);
	ImGuiIO &io = ImGui::GetIO();
	bool is_mouse = false;
	bool is_key = false;

	if (const InputEventMouseMotion *mm = Object::cast_to<InputEventMouseMotion>(p_event.ptr())) {
		io.AddMousePosEvent(mm->get_position().x, mm->get_position().y);
		is_mouse = true;
	} else if (const InputEventMouseButton *mb = Object::cast_to<InputEventMouseButton>(p_event.ptr())) {
		io.AddMousePosEvent(mb->get_position().x, mb->get_position().y);
		const float wheel = mb->get_factor() > 0.0f ? mb->get_factor() : 1.0f;
		switch (mb->get_button_index()) {
			case MOUSE_BUTTON_LEFT: io.AddMouseButtonEvent(0, mb->is_pressed()); break;
			case MOUSE_BUTTON_RIGHT: io.AddMouseButtonEvent(1, mb->is_pressed()); break;
			case MOUSE_BUTTON_MIDDLE: io.AddMouseButtonEvent(2, mb->is_pressed()); break;
			case MOUSE_BUTTON_WHEEL_UP: if (mb->is_pressed()) io.AddMouseWheelEvent(0, wheel); break;
			case MOUSE_BUTTON_WHEEL_DOWN: if (mb->is_pressed()) io.AddMouseWheelEvent(0, -wheel); break;
			case MOUSE_BUTTON_WHEEL_LEFT: if (mb->is_pressed()) io.AddMouseWheelEvent(wheel, 0); break;
			case MOUSE_BUTTON_WHEEL_RIGHT: if (mb->is_pressed()) io.AddMouseWheelEvent(-wheel, 0); break;
			default: break;
		}
		is_mouse = true;
	} else if (const InputEventKey *k = Object::cast_to<InputEventKey>(p_event.ptr())) {
		io.AddKeyEvent(ImGuiMod_Ctrl, k->is_ctrl_pressed());
		io.AddKeyEvent(ImGuiMod_Shift, k->is_shift_pressed());
		io.AddKeyEvent(ImGuiMod_Alt, k->is_alt_pressed());
		io.AddKeyEvent(ImGuiMod_Super, k->is_meta_pressed());
		const ImGuiKey key = to_imgui_key(k->get_keycode());
		if (key != ImGuiKey_None) {
			io.AddKeyEvent(key, k->is_pressed());
		}
		if (k->is_pressed() && k->get_unicode() >= 32) {
			io.AddInputCharacter(uint32_t(k->get_unicode()));
		}
		is_key = true;
	}

	// WantCapture* reflects the previous frame; ignore it once the UI is gone.
	if (drew_last_frame && ((is_mouse && io.WantCaptureMouse) || (is_key && io.WantCaptureKeyboard))) {
		p_receiver->get_viewport()->set_input_as_handled();
	}
}

void ImGuiAPI::_focus_changed(bool p_focused) {
	if (context) {
		ImGui::SetCurrentContext(context);
		ImGui::GetIO().AddFocusEvent(p_focused);
	}
}
