import re

header_path = 'src/presentation/OverlayWindow.h'
with open(header_path, 'r') as f:
    content = f.read()

if '#include "BentoDashboard.h"' not in content:
    content = content.replace('#include "../domain/TabController.h"', '#include "../domain/TabController.h"\n#include "BentoDashboard.h"')
    content = content.replace('NoteEditorOverlay* m_editorOverlay;', 'NoteEditorOverlay* m_editorOverlay;\n    BentoDashboard* m_dashboardOverlay;')
    with open(header_path, 'w') as f:
        f.write(content)

cpp_path = 'src/presentation/OverlayWindow.cpp'
with open(cpp_path, 'r') as f:
    content = f.read()

# Instantiate BentoDashboard
if 'm_dashboardOverlay = new BentoDashboard' not in content:
    insert_idx = content.find('m_editorOverlay = new NoteEditorOverlay')
    dashboard_code = "    m_dashboardOverlay = new BentoDashboard(controller, this);\n"
    content = content[:insert_idx] + dashboard_code + content[insert_idx:]
    
    with open(cpp_path, 'w') as f:
        f.write(content)

