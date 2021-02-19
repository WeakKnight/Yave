/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/

#include "PropertyPanel.h"
#include "widgets.h"

#include <editor/context/EditorContext.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

PropertyPanel::PropertyPanel(ContextPtr cptr) :
        Widget(ICON_FA_WRENCH " Properties"),
        ContextLinked(cptr) {

    set_closable(false);
}

void PropertyPanel::paint(CmdBufferRecorder&) {
    if(!context()->selection().has_selected_entity()) {
        return;
    }

    ecs::EntityId id = context()->selection().selected_entity();
    if(id.is_valid()) {
        ImGui::PushID("widgets");
        draw_component_widgets(context(), id);
        ImGui::PopID();
    }
}

}

