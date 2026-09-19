#include <QApplication>
#include <QTimer>
#include <QDebug>
#include "src/domain/TabController.h"
#include "src/presentation/NoteEditorOverlay.h"
#include "src/infrastructure/DatabaseManager.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    
    // Setup DB
    infrastructure::DatabaseManager::instance().initialize("/home/ahmed/.local/share/gutterTab/profiles/bdba16e4-44f8-4382-9159-15247318bae9/notes.db");
    
    domain::TabController controller;
    
    // Read notes
    auto notes = controller.notes();
    qDebug() << "Notes loaded:" << notes.size();
    
    // Create new note if empty
    if (notes.isEmpty()) {
        controller.createNewNote();
        notes = controller.notes();
    }
    
    int targetId = notes[0].id;
    
    // Create overlay
    presentation::NoteEditorOverlay overlay(&controller);
    
    // Open note
    controller.openNote(targetId);
    
    // Simulate typing by finding the MarkdownEditor and calling insertPlainText
    QTimer::singleShot(1000, [&]() {
        auto* editor = overlay.findChild<QTextEdit*>();
        if (editor) {
            qDebug() << "Editor found! Typing...";
            editor->insertPlainText(" QT GUI TYPING TEST ");
        } else {
            qDebug() << "EDITOR NOT FOUND!";
        }
    });
    
    QTimer::singleShot(2000, &app, &QCoreApplication::quit);
    
    return app.exec();
}
