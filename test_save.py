import re

cpp_path = 'src/presentation/NoteEditorOverlay.cpp'
with open(cpp_path, 'r') as f:
    content = f.read()

# add a qDebug
new_fn = """void NoteEditorOverlay::onTextChanged() {
    int currentId = m_controller->activeNoteId();
    if (currentId != -1) {
        QString md = m_textEdit->toMarkdown();
        qDebug() << "Saving note ID:" << currentId << "Content Length:" << md.length();
        m_controller->updateNoteContent(currentId, md);
    }
}"""
content = re.sub(r'void NoteEditorOverlay::onTextChanged\(\) \{.*?\n\}', new_fn, content, flags=re.DOTALL)
with open(cpp_path, 'w') as f:
    f.write(content)
