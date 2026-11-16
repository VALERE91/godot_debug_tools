extends Node3D
## debug_tools demo: DebugDraw3D shapes + labels, DebugDraw2D overlay and graph, an ImGui control panel.
## Half of the red sphere is under the floor to show depth testing.
## Screenshot mode (automated visual check):
##   DEBUG_TOOLS_SCREENSHOT=/abs/path/out.png godot --path project --resolution 1280x720
## DEBUG_TOOLS_IMGUI_DEMO=1 opens the Dear ImGui demo window at start.

var _t: float = 0.0
var _frame: int = 0
var _speed := [1.0]
var _depth_test := [true]
var _show_demo := [OS.has_environment("DEBUG_TOOLS_IMGUI_DEMO")]
var _label := ["debug_tools"]


func _ready() -> void:
	$Camera3D.look_at_from_position(Vector3(5, 4, 7), Vector3(0, 0.5, 0))
	$Sun.rotation_degrees = Vector3(-50, 30, 0)


func _process(delta: float) -> void:
	_t += delta * _speed[0]
	_draw_3d()
	_draw_ui(delta)

	var shot := OS.get_environment("DEBUG_TOOLS_SCREENSHOT")
	_frame += 1
	if not shot.is_empty() and _frame == 60:
		await RenderingServer.frame_post_draw
		get_viewport().get_texture().get_image().save_png(shot)
		get_tree().quit()


func _draw_3d() -> void:
	DebugDraw3D.line(Vector3(-3, 0.01, 0), Vector3(3, 0.01, 0), Color.RED)
	DebugDraw3D.line(Vector3(0, 0.01, -3), Vector3(0, 0.01, 3), Color.BLUE)
	DebugDraw3D.arrow(Vector3.ZERO, Vector3.UP * 2, Color.GREEN)
	DebugDraw3D.sphere(Vector3(-2, 1, 0), 0.75, Color.CYAN)
	DebugDraw3D.box(Transform3D(Basis(Vector3.UP, _t), Vector3(2, 0.5, 0)), Color.YELLOW)
	DebugDraw3D.capsule(Vector3(0, 0.5, -2), Vector3(0, 1.8, -2), 0.4, Color.MAGENTA)
	DebugDraw3D.aabb(AABB(Vector3(-1, 0, 1.5), Vector3(2, 1, 1)), Color.ORANGE)
	DebugDraw3D.point(Vector3(sin(_t) * 2, 1.5, cos(_t) * 2), 0.3, Color.WHITE)
	DebugDraw3D.sphere(Vector3(2, 0, 2.5), 1.0, Color.RED)
	var pts := PackedVector3Array()
	for i in 32:
		pts.append(Vector3(i * 0.2 - 3.2, 0.3 + sin(i * 0.5 + _t) * 0.2, 3.5))
	DebugDraw3D.path(pts, Color.LIME_GREEN)
	DebugDraw3D.text(Vector3(-2, 1.9, 0), "sphere", Color.CYAN)
	DebugDraw3D.text(Vector3(0, 2.4, -2), "capsule", Color.MAGENTA)
	DebugDraw3D.text(Vector3(0, 2.2, 0), _label[0])


func _draw_ui(delta: float) -> void:
	DebugDraw2D.graph("fps", Engine.get_frames_per_second())
	DebugDraw2D.graph("frame ms", delta * 1000.0)
	var size := get_viewport().get_visible_rect().size
	DebugDraw2D.rect(Rect2(Vector2(8, size.y - 40), Vector2(260, 32)), Color(0, 0, 0, 0.5), true)
	DebugDraw2D.text(Vector2(16, size.y - 32), "lines: %d  imgui verts: %d" % [DebugDraw3D.get_line_count(), ImGui.get_vertex_count()])
	DebugDraw2D.circle(get_viewport().get_mouse_position(), 6.0, Color.YELLOW)

	ImGui.set_next_window_pos(Vector2(10, 10), ImGui.COND_FIRST_USE_EVER)
	if ImGui.begin("debug_tools", [], ImGui.WINDOW_ALWAYS_AUTO_RESIZE):
		ImGui.text("DebugDraw3D")
		ImGui.slider_float("speed", _speed, 0.0, 4.0)
		if ImGui.checkbox("depth test", _depth_test):
			DebugDraw3D.depth_test = _depth_test[0]
		ImGui.input_text("label", _label)
		ImGui.separator()
		ImGui.checkbox("ImGui demo window", _show_demo)
		if ImGui.begin_table("stats", 2, ImGui.TABLE_BORDERS | ImGui.TABLE_ROW_BG):
			for row: Array in [["lines", DebugDraw3D.get_line_count()], ["fps", Engine.get_frames_per_second()]]:
				ImGui.table_next_row()
				ImGui.table_next_column()
				ImGui.text(str(row[0]))
				ImGui.table_next_column()
				ImGui.text(str(row[1]))
			ImGui.end_table()
	ImGui.end()
	if _show_demo[0]:
		ImGui.show_demo_window()
