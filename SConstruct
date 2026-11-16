#!/usr/bin/env python
# Builds the debug_tools GDExtension into project/addons/debug_tools/bin/.
#   scons target=template_debug   (editor + debug exports)
#   scons target=template_release

env = SConscript("godot-cpp/SConstruct", {"api_version": "4.7"})

IMGUI = "thirdparty/imgui/"
env.Append(CPPPATH=["src/", IMGUI])
env.Append(CPPDEFINES=["IMGUI_DISABLE_OBSOLETE_FUNCTIONS"])

sources = Glob("src/*.cpp") + [
    IMGUI + f for f in ["imgui.cpp", "imgui_draw.cpp", "imgui_tables.cpp", "imgui_widgets.cpp", "imgui_demo.cpp"]
]

BIN = "project/addons/debug_tools/bin/"
if env["platform"] == "macos":
    name = "libdebug_tools.{}.{}".format(env["platform"], env["target"])
    library = env.SharedLibrary(BIN + name + ".framework/" + name, source=sources)
else:
    library = env.SharedLibrary(BIN + "libdebug_tools" + env["suffix"] + env["SHLIBSUFFIX"], source=sources)

env.NoCache(library)
Default(library)
