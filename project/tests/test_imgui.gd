extends SceneTree
## Headless ImGui / DebugDraw2D test: geometry, Array out-params, simulated mouse click, input capture,
## error recovery, graphs, DebugDraw3D.text.
##   godot --headless --path project -s res://tests/test_imgui.gd

var _failed := 0


func _initialize() -> void:
	_run.call_deferred()


func _run() -> void:
	root.size = Vector2i(1280, 720) # headless root window defaults to 64x64
	# Counts events that reach game code (_unhandled_input), i.e. not consumed by ImGui.
	var probe_script := GDScript.new()
	probe_script.source_code = "extends Node\nvar count := 0\nfunc _unhandled_input(_e: InputEvent) -> void:\n\tcount += 1\n"
	probe_script.reload()
	var probe := Node.new()
	probe.set_script(probe_script)
	root.add_child(probe)

	# Geometry from a window with every widget.
	var f := [0.5]
	var i := [3]
	var b := [true]
	var s := ["hello"]
	var c := [Color.RED]
	var combo := [1]
	for n in 3:
		await process_frame
		_widgets(f, i, b, s, c, combo)
	await process_frame # flush (headless: on process_frame)
	_check("widgets render geometry", ImGui.get_vertex_count() > 500, true)
	_check("untouched out-params keep values", [f[0], i[0], b[0], s[0], c[0], combo[0]] == [0.5, 3, true, "hello", Color.RED, 1], true)

	# Simulated click on a button at a known position (no title bar, 8 px padding -> button at 8..128 x 8..48).
	var clicked := 0
	var before: int = probe.count
	var events: Array = [_motion(Vector2(60, 28)), _button(Vector2(60, 28), true), _button(Vector2(60, 28), false)]
	for n in 8:
		await process_frame
		if _click_window():
			clicked += 1
		if n >= 2 and n - 2 < events.size(): # input node is added deferred on first use
			root.push_input(events[n - 2])
	_check("button clicked exactly once", clicked, 1)
	_check("hovered UI captures mouse", ImGui.want_capture_mouse(), true)
	# The first motion arrives before ImGui knows the mouse is over a window; press + release are consumed.
	_check("press/release over UI consumed", probe.count - before, 1)
	root.push_input(_motion(Vector2(600, 400)))
	await process_frame
	_click_window()
	await process_frame
	_click_window()
	before = probe.count
	root.push_input(_motion(Vector2(610, 400)))
	root.push_input(_button(Vector2(610, 400), true))
	_check("events outside UI reach the game", probe.count - before, 2)

	# Unbalanced begin(): ImGui error recovery must keep running (warning, no crash).
	for n in 3:
		await process_frame
		ImGui.begin("Unbalanced", [], ImGui.WINDOW_NO_SAVED_SETTINGS)
		ImGui.text("no end()")
	await process_frame
	await process_frame
	_click_window()
	await process_frame
	_check("still rendering after unbalanced begin", ImGui.get_vertex_count() > 0, true)

	# DebugDraw2D + graphs.
	await process_frame
	await process_frame
	var base := ImGui.get_vertex_count() # nothing drawn last frame
	for n in 3:
		await process_frame
		DebugDraw2D.line(Vector2(10, 10), Vector2(200, 100), Color.RED, 2.0)
		DebugDraw2D.rect(Rect2(50, 50, 100, 60), Color.GREEN)
		DebugDraw2D.circle(Vector2(300, 200), 40, Color.BLUE, true)
		DebugDraw2D.text(Vector2(20, 300), "debug text")
	await process_frame
	var shapes := ImGui.get_vertex_count()
	_check("DebugDraw2D draws", shapes > base, true)
	for n in 3:
		await process_frame
		DebugDraw2D.graph("fps", 60.0 + n)
	await process_frame
	_check("graph window draws", ImGui.get_vertex_count() > 100, true)

	# DebugDraw3D.text projects through the current camera.
	var cam := Camera3D.new()
	root.add_child(cam)
	cam.look_at_from_position(Vector3(0, 0, 5), Vector3.ZERO)
	await process_frame
	await process_frame
	await process_frame
	DebugDraw3D.text(Vector3.ZERO, "origin")
	await process_frame
	_check("DebugDraw3D.text in front of camera draws", ImGui.get_vertex_count() > 0, true)
	await process_frame
	await process_frame
	DebugDraw3D.text(Vector3(0, 0, 10), "behind") # behind the camera: skipped
	await process_frame
	_check("DebugDraw3D.text behind camera skipped", ImGui.get_vertex_count(), 0)

	print("%s (%d failed)" % ["ALL PASS" if _failed == 0 else "FAILURES", _failed])
	quit(1 if _failed else 0)


func _widgets(f: Array, i: Array, b: Array, s: Array, c: Array, combo: Array) -> void:
	ImGui.set_next_window_pos(Vector2(400, 20), ImGui.COND_ALWAYS)
	if ImGui.begin("Widgets", [], ImGui.WINDOW_NO_SAVED_SETTINGS):
		ImGui.text("text")
		ImGui.text_colored(Color.YELLOW, "colored")
		ImGui.text_wrapped("wrapped")
		ImGui.button("button")
		ImGui.same_line()
		ImGui.checkbox("checkbox", b)
		ImGui.slider_float("slider_float", f, 0.0, 1.0)
		ImGui.slider_int("slider_int", i, 0, 10)
		ImGui.drag_float("drag_float", f, 0.01)
		ImGui.drag_int("drag_int", i)
		ImGui.input_text("input_text", s)
		ImGui.color_edit("color_edit", c)
		ImGui.combo("combo", combo, PackedStringArray(["a", "b", "c"]))
		ImGui.separator()
		if ImGui.collapsing_header("header", ImGui.TREE_NODE_DEFAULT_OPEN):
			if ImGui.tree_node("node"):
				ImGui.text("inside")
				ImGui.tree_pop()
		ImGui.plot_lines("plot", PackedFloat32Array([0, 1, 0.5, 2]))
		ImGui.plot_histogram("hist", PackedFloat32Array([1, 2, 3]))
		ImGui.progress_bar(0.4)
		if ImGui.begin_table("table", 2, ImGui.TABLE_BORDERS):
			ImGui.table_setup_column("A")
			ImGui.table_setup_column("B")
			ImGui.table_headers_row()
			ImGui.table_next_row()
			ImGui.table_next_column()
			ImGui.text("a")
			ImGui.table_next_column()
			ImGui.text("b")
			ImGui.end_table()
		ImGui.push_id("id")
		ImGui.button("same label")
		ImGui.pop_id()
	ImGui.end()


func _click_window() -> bool:
	ImGui.set_next_window_pos(Vector2.ZERO, ImGui.COND_ALWAYS)
	var flags := ImGui.WINDOW_NO_TITLE_BAR | ImGui.WINDOW_NO_MOVE | ImGui.WINDOW_NO_SAVED_SETTINGS | ImGui.WINDOW_ALWAYS_AUTO_RESIZE
	ImGui.begin("Click", [], flags)
	var pressed := ImGui.button("Click", Vector2(120, 40))
	ImGui.end()
	return pressed


func _motion(pos: Vector2) -> InputEventMouseMotion:
	var e := InputEventMouseMotion.new()
	e.position = pos
	return e


func _button(pos: Vector2, pressed: bool) -> InputEventMouseButton:
	var e := InputEventMouseButton.new()
	e.position = pos
	e.button_index = MOUSE_BUTTON_LEFT
	e.pressed = pressed
	return e


func _check(label: String, got: Variant, expected: Variant) -> void:
	var ok: bool = got == expected
	print("%s %s: %s%s" % ["PASS" if ok else "FAIL", label, got, "" if ok else " (expected %s)" % expected])
	_failed += 0 if ok else 1
