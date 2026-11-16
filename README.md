# godot_debug_tools

Godot 4.7+ GDExtension: [Dear ImGui](https://github.com/ocornut/imgui) and immediate-mode debug drawing (3D + 2D), callable from GDScript.

- Works on Forward+, Mobile and Compatibility (ImGui renders through `RenderingServer` canvas items).
- No autoload or scene setup: call it from anywhere, it hooks itself into the root viewport.
- No-op in release builds. ImGui and DebugDraw2D are disabled in the editor.
- macOS (universal), Windows x86_64, Linux x86_64.

## Install

Download `debug_tools.zip` from the [latest release](https://github.com/VALERE91/godot_debug_tools/releases/latest),
extract it into your project root (you get `addons/debug_tools/`), then restart the editor.

## Usage

```gdscript
var speed := [1.0]   # values edited by ImGui are passed as one-element Arrays

func _process(delta: float) -> void:
	# 3D, in the world. duration = 0 draws one frame; > 0 keeps it for that many seconds.
	DebugDraw3D.line(Vector3.ZERO, Vector3.UP, Color.RED)
	DebugDraw3D.sphere(target.global_position, 0.5, Color.CYAN)
	DebugDraw3D.arrow(from, to, Color.GREEN, 2.0)
	DebugDraw3D.text(enemy.global_position, "alert")

	# 2D, screen space (above the game, below ImGui windows).
	DebugDraw2D.rect(Rect2(10, 10, 200, 40), Color(0, 0, 0, 0.5), true)
	DebugDraw2D.graph("fps", Engine.get_frames_per_second())

	# ImGui
	if ImGui.begin("Tools", [], ImGui.WINDOW_ALWAYS_AUTO_RESIZE):
		ImGui.slider_float("speed", speed, 0.0, 4.0)
		if ImGui.button("Reset"):
			speed[0] = 1.0
	ImGui.end()
```

Call ImGui from the main thread (`_process`). DebugDraw3D is safe to call from threaded physics.

### API

| Singleton | Functions |
|---|---|
| `DebugDraw3D` | `line`, `ray`, `arrow`, `path`, `point`, `aabb`, `box`, `sphere`, `capsule`, `text`, `clear`, `depth_test` (property), `get_line_count` |
| `DebugDraw2D` | `line`, `rect`, `circle`, `text`, `graph` |
| `ImGui` | `begin`/`end`, `text`, `text_colored`, `text_wrapped`, `button`, `checkbox`, `slider_float`/`slider_int`, `drag_float`/`drag_int`, `input_text`, `color_edit`, `combo`, `collapsing_header`, `tree_node`/`tree_pop`, `same_line`, `separator`, `spacing`, `plot_lines`, `plot_histogram`, `progress_bar`, `begin_table`/`end_table` + `table_*`, `image`, `set_next_window_pos`/`set_next_window_size`, `push_id`/`pop_id`, `is_item_hovered`, `set_tooltip`, `show_demo_window`, `want_capture_mouse`/`want_capture_keyboard` |

Flag constants live on the singleton: `ImGui.WINDOW_NO_TITLE_BAR`, `ImGui.TABLE_BORDERS`, `ImGui.COND_FIRST_USE_EVER`, etc.
ImGui window layout is saved to `user://imgui.ini`.

## Build

```bash
git clone --recursive https://github.com/VALERE91/godot_debug_tools.git
cd godot_debug_tools
scons target=template_debug     # editor + debug exports
scons target=template_release
```

Output goes to `project/addons/debug_tools/bin/`. `project/` is a demo (open it in Godot and run it) and holds the headless tests:

```bash
godot --headless --path project -s res://tests/test_debug_draw_3d.gd
godot --headless --path project -s res://tests/test_imgui.gd
```

Push a `v*` tag to have CI build all platforms and attach `debug_tools.zip` to a GitHub release.

## Third-party

Bundles Dear ImGui (MIT) and godot-cpp (MIT). Their licenses ship in the release zip.
