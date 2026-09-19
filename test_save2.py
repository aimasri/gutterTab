import re

cpp_path = 'src/domain/TabController.cpp'
with open(cpp_path, 'r') as f:
    content = f.read()

new_fn = """void TabController::updateNoteContent(int id, const QString& content) {
    bool found = false;
    for (auto& note : m_notes) {
        if (note.id == id) {
            note.content = content;
            bool ok = infrastructure::DatabaseManager::instance().saveNote(note);
            qDebug() << "TabController::updateNoteContent found note ID:" << id << "Saved OK:" << ok;
            found = true;
            break;
        }
    }
    if (!found) qDebug() << "TabController::updateNoteContent WARNING: note not found in m_notes! ID:" << id;
}"""
content = re.sub(r'void TabController::updateNoteContent\(int id, const QString& content\) \{.*?\n\}', new_fn, content, flags=re.DOTALL)
with open(cpp_path, 'w') as f:
    f.write(content)
