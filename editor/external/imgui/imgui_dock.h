#ifndef IMGUI_DOCK_H
#define IMGUI_DOCK_H

#include "imgui.h"

namespace ImGui
{


IMGUI_API void ShutdownDock();
IMGUI_API void RootDock(const ImVec2& pos, const ImVec2& size);
IMGUI_API bool BeginDock(const char* label, bool* opened = nullptr, ImGuiWindowFlags extra_flags = 0, const ImVec2& default_size = ImVec2(-1, -1));
IMGUI_API void EndDock();
IMGUI_API void SetDockActive();
IMGUI_API void SaveDockContext(const char* filename);
IMGUI_API void LoadDockContext(const char* filename);

} // namespace ImGui

#endif // IMGUI_DOCK_H
