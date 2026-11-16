extends Node3D
## DebugDraw3D demo: every shape, animated. Half of the red sphere is under the floor to show depth testing.
## Screenshot mode (automated visual check):
##   DEBUG_TOOLS_SCREENSHOT=/abs/path/out.png godot --path project --resolution 960x540

var _t: float = 0.0
var _frame: int = 0


func _ready() -> void:
	$Camera3D.look_at_from_position(Vector3(5, 4, 7), Vector3(0, 0.5, 0))
	$Sun.rotation_degrees = Vector3(-50, 30, 0)


func _process(delta: float) -> void:
	_t += delta
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

	var shot := OS.get_environment("DEBUG_TOOLS_SCREENSHOT")
	_frame += 1
	if not shot.is_empty() and _frame == 60:
		await RenderingServer.frame_post_draw
		get_viewport().get_texture().get_image().save_png(shot)
		get_tree().quit()
