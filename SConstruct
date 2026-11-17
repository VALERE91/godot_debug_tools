#!/usr/bin/env python
# Builds the debug_tools GDExtension into project/addons/debug_tools/bin/.
#   scons target=template_debug   (editor + debug exports)
#   scons target=template_release

MACOS_MIN = "11.0"
# godot-cpp leaves the target unset (= build host's OS); pin it so binary and Info.plist agree.
ARGUMENTS.setdefault("macos_deployment_target", MACOS_MIN)

env = SConscript("godot-cpp/SConstruct", {"api_version": "4.7"})

IMGUI = "thirdparty/imgui/"
env.Append(CPPPATH=["src/", IMGUI])
env.Append(CPPDEFINES=[("IMGUI_USER_CONFIG", r'\"imgui_config.h\"')])  # src/imgui_config.h

# Objects go to build/, not next to the sources: that would dirty the imgui submodule.
imgui_objects = [
    env.SharedObject("build/imgui/{}.{}".format(f, env["target"]), IMGUI + f + ".cpp")
    for f in ["imgui", "imgui_draw", "imgui_tables", "imgui_widgets", "imgui_demo"]
]
sources = Glob("src/*.cpp") + imgui_objects

BIN = "project/addons/debug_tools/bin/"
if env["platform"] == "macos":
    name = "libdebug_tools.{}.{}".format(env["platform"], env["target"])
    framework = BIN + name + ".framework/"
    library = env.SharedLibrary(framework + name, source=sources)
    # codesign (macOS export) rejects a framework bundle without Info.plist.
    plist = env.Textfile(framework + "Resources/Info.plist", [
        '<?xml version="1.0" encoding="UTF-8"?>',
        '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">',
        '<plist version="1.0"><dict>',
        "<key>CFBundleExecutable</key><string>{}</string>".format(name),
        "<key>CFBundleIdentifier</key><string>io.gamedev4fun.debug-tools.{}</string>".format(env["target"].replace("_", "-")),
        "<key>CFBundleInfoDictionaryVersion</key><string>6.0</string>",
        "<key>CFBundleName</key><string>{}</string>".format(name),
        "<key>CFBundlePackageType</key><string>FMWK</string>",
        "<key>CFBundleShortVersionString</key><string>1.0.0</string>",
        "<key>CFBundleVersion</key><string>1.0.0</string>",
        "<key>LSMinimumSystemVersion</key><string>{}</string>".format(env["macos_deployment_target"]),
        "</dict></plist>",
    ])
    Default(plist)
else:
    library = env.SharedLibrary(BIN + "libdebug_tools" + env["suffix"] + env["SHLIBSUFFIX"], source=sources)

env.NoCache(library)
Default(library)
