#!/usr/bin/env python
# Builds the debug_tools GDExtension into project/addons/debug_tools/bin/.
#   scons target=template_debug   (editor + debug exports)
#   scons target=template_release

env = SConscript("godot-cpp/SConstruct", {"api_version": "4.7"})

IMGUI = "thirdparty/imgui/"
env.Append(CPPPATH=["src/", IMGUI])
env.Append(CPPDEFINES=["IMGUI_DISABLE_OBSOLETE_FUNCTIONS"])

# Objects go to build/, not next to the sources: that would dirty the imgui submodule.
imgui_objects = [
    env.SharedObject("build/imgui/{}.{}".format(f, env["target"]), IMGUI + f + ".cpp")
    for f in ["imgui", "imgui_draw", "imgui_tables", "imgui_widgets", "imgui_demo"]
]
sources = Glob("src/*.cpp") + imgui_objects

BIN = "project/addons/debug_tools/bin/"
if env["platform"] == "macos":
    name = "libdebug_tools.{}.{}".format(env["platform"], env["target"])
    library = env.SharedLibrary(BIN + name + ".framework/" + name, source=sources)
else:
    library = env.SharedLibrary(BIN + "libdebug_tools" + env["suffix"] + env["SHLIBSUFFIX"], source=sources)

env.NoCache(library)
Default(library)
