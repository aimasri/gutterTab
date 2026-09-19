#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>
#include "src/domain/TabController.h"
#include "src/infrastructure/DatabaseManager.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    
    // Initialize DB (this creates a test db or uses active)
    infrastructure::DatabaseManager::instance().initialize("/home/ahmed/.local/share/gutterTab/profiles/bdba16e4-44f8-4382-9159-15247318bae9/notes.db");
    
    domain::TabController controller;
    
    // Wait for DB load
    auto notes = controller.notes();
    qDebug() << "Loaded" << notes.size() << "notes.";
    
    if (notes.isEmpty()) {
        controller.createNewNote();
        notes = controller.notes();
    }
    
    int id = notes[0].id;
    qDebug() << "Editing note ID:" << id;
    
    controller.updateNoteContent(id, "TEST SAVE " + QString::number(QDateTime::currentMSecsSinceEpoch()));
    
    return 0;
}
