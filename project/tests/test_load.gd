extends SceneTree
## Headless check that the extension loads and its singletons exist:
##   godot --headless --path project -s res://tests/test_load.gd

func _initialize() -> void:
	var failed := 0
	for s: String in ["DebugDraw3D", "DebugDraw2D", "ImGui"]:
		var ok := Engine.has_singleton(s)
		print("%s singleton %s" % ["PASS" if ok else "FAIL", s])
		failed += 0 if ok else 1
	quit(1 if failed else 0)
