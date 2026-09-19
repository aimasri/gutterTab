#include <QApplication>
#include <QDebug>
#include <QTimer>
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
    
    // Open Note 1
    int targetId = 1;
    if (notes.size() > 0) targetId = notes[0].id;
    
    // Create overlay
    presentation::NoteEditorOverlay overlay(&controller);
    
    // Open note
    controller.openNote(targetId);
    
    // Simulate typing via QTimer
    QTimer::singleShot(500, [&]() {
        qDebug() << "Simulating typing...";
        // We need to access m_textEdit. Since it's private, we will just simulate a signal or rely on focus?
        // Wait, we can't easily access m_textEdit from outside.
        // Let's just use X11 simulation.
    });
    
    QTimer::singleShot(1500, &app, &QCoreApplication::quit);
    
    return app.exec();
}
