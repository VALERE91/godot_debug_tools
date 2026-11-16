#pragma once
// Dear ImGui user config (IMGUI_USER_CONFIG, set in SConstruct). Included by every imgui TU.

// Report to Godot instead of abort(): a GDScript Begin/End mismatch must not kill the game.
// Common user errors are handled by ImGui's error recovery (ConfigErrorRecoveryEnableAssert = false).
void debug_tools_imgui_assert(const char *p_expr, const char *p_file, int p_line);
void debug_tools_imgui_log(const char *p_fmt, ...);

#define IM_ASSERT(_EXPR) ((_EXPR) ? (void)0 : debug_tools_imgui_assert(#_EXPR, __FILE__, __LINE__))
#define IMGUI_DEBUG_PRINTF(...) debug_tools_imgui_log(__VA_ARGS__)
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
