extends SceneTree
## Headless DebugDraw3D test: segment counts per shape, duration-0 and timed expiry, clear().
##   godot --headless --path project -s res://tests/test_debug_draw_3d.gd

var _failed := 0


func _initialize() -> void:
	_run.call_deferred()


func _run() -> void:
	# Expected segments per shape (24-step circles).
	var cases: Array = [
		["line", 1, func() -> void: DebugDraw3D.line(Vector3.ZERO, Vector3.ONE)],
		["ray", 1, func() -> void: DebugDraw3D.ray(Vector3.ZERO, Vector3.UP, Color.GREEN)],
		["arrow", 5, func() -> void: DebugDraw3D.arrow(Vector3.ZERO, Vector3(0, 0, 2))],
		["path", 3, func() -> void: DebugDraw3D.path(PackedVector3Array([Vector3.ZERO, Vector3.UP, Vector3.ONE, Vector3.RIGHT]))],
		["point", 3, func() -> void: DebugDraw3D.point(Vector3.ONE)],
		["aabb", 12, func() -> void: DebugDraw3D.aabb(AABB(Vector3.ZERO, Vector3.ONE))],
		["box", 12, func() -> void: DebugDraw3D.box(Transform3D.IDENTITY)],
		["sphere", 72, func() -> void: DebugDraw3D.sphere(Vector3.ZERO)],
		["capsule", 100, func() -> void: DebugDraw3D.capsule(Vector3.ZERO, Vector3.UP)],
		["capsule degenerate -> sphere", 72, func() -> void: DebugDraw3D.capsule(Vector3.ONE, Vector3.ONE)],
	]
	for c: Array in cases:
		c[2].call()
		_check(c[0], await _next_draw_count(), c[1])
		await _frames(2)
		_check(c[0] + " expired after one frame", DebugDraw3D.get_line_count(), 0)

	DebugDraw3D.line(Vector3.ZERO, Vector3.UP, Color.RED, 0.3)
	_check("timed line drawn", await _next_draw_count(), 1)
	await _frames(3)
	_check("timed line still alive", DebugDraw3D.get_line_count(), 1)
	await create_timer(0.5).timeout
	await _frames(2)
	_check("timed line expired", DebugDraw3D.get_line_count(), 0)

	DebugDraw3D.sphere(Vector3.ZERO, 1.0, Color.RED, 10.0)
	DebugDraw3D.clear()
	await _frames(3)
	_check("clear()", DebugDraw3D.get_line_count(), 0)

	DebugDraw3D.depth_test = false
	_check("depth_test property", int(DebugDraw3D.depth_test), 0)
	DebugDraw3D.depth_test = true

	print("%s (%d failed)" % ["ALL PASS" if _failed == 0 else "FAILURES", _failed])
	quit(1 if _failed else 0)


func _check(label: String, got: int, expected: int) -> void:
	var ok := got == expected
	print("%s %s: %d%s" % ["PASS" if ok else "FAIL", label, got, "" if ok else " (expected %d)" % expected])
	_failed += 0 if ok else 1


## Line count of the first rendered frame that has something (or 0 after 10 frames).
func _next_draw_count() -> int:
	for i in 10:
		await process_frame
		if DebugDraw3D.get_line_count() > 0:
			return DebugDraw3D.get_line_count()
	return 0


func _frames(n: int) -> void:
	for i in n:
		await process_frame
