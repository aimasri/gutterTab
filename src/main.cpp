/**
 * @file main.cpp
 * @brief Application entry point, profile resolution, and lifecycle bootstrapping root.
 * 
 * @details Coordinates the bootstrapping sequence for gutterTab:
 *          1. Initializes QApplication and application metadata.
 *          2. Resolves active profile via auto-launch preference or modal ProfilePickerWindow.
 *          3. Loads profile JSON configuration into ConfigManager.
 *          4. Initializes profile SQLite database via DatabaseManager.
 *          5. Instantiates domain TabController and presentation OverlayWindow.
 *          6. Seeds default sample notes on first-time profile creation.
 *          7. Enters the primary Qt GUI event loop.
 */

#include <QApplication>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>
#include <QPushButton>
#include "infrastructure/ConfigManager.h"
#include "infrastructure/DatabaseManager.h"
#include "domain/TabController.h"
#include "presentation/OverlayWindow.h"
#include "presentation/ProfilePickerWindow.h"

/**
 * @brief Main application entry point.
 * 
 * @param argc Command line argument count.
 * @param argv Command line argument vectors.
 * @return Process exit code (0 on normal exit, -1 on fatal infrastructure initialization failure).
 * 
 * @details Implements the full desktop application lifecycle:
 *          - Bootstraps the Qt graphical subsystem.
 *          - Resolves profile target: checks auto-launch profile setting. If empty or if its config
 *            file is missing on disk, launches the ProfilePickerWindow and runs a local event loop
 *            to let the user choose or create a profile. Exits gracefully (0) if user closes picker.
 *          - Configures the infrastructure layer: loads profile settings and connects SQLite database.
 *            Returns -1 if database initialization fails.
 *          - Instantiates the domain TabController and presentation OverlayWindow.
 *          - Seeds default notes if the profile database is freshly created and empty.
 *          - Displays the frameless desktop gutter overlay and runs the main event loop.
 * 
 * @note Operates on Qt's primary GUI thread.
 */
int main(int argc, char *argv[]) {
    // Step 1: Initialize Qt application framework and application name metadata
    QApplication app(argc, argv);
    app.setApplicationName("gutterTab");
    
    qDebug() << "gutterTab is starting...";
    
    // Step 2: Resolve target profile ID from auto-launch configuration
    QString targetProfileId = infrastructure::ConfigManager::getAutoLaunchProfile();
    
    // Step 3: If auto-launch is unset or target config file does not exist, prompt via ProfilePickerWindow
    if (targetProfileId.isEmpty() || !QFile::exists(infrastructure::ConfigManager::getProfileConfigPath(targetProfileId))) {
        ProfilePickerWindow picker;
        
        // Connect profile selection signal to capture choice and close picker
        QObject::connect(&picker, &ProfilePickerWindow::profileSelected, [&](const QString& id) {
            targetProfileId = id;
            picker.close();
        });
        
        // Connect abort signal if user dismisses picker window without choosing
        QObject::connect(&picker, &ProfilePickerWindow::closedWithoutSelection, [&]() {
            QApplication::quit();
        });
        
        picker.show();
        app.exec(); // Run temporary event loop until picker interaction completes
        
        // If user cancelled without selecting a profile, exit process cleanly
        if (targetProfileId.isEmpty()) {
            return 0;
        }
    }
    
    // Step 4: Load configuration settings for the resolved profile
    infrastructure::ConfigManager::instance().load(targetProfileId);
    
    // Step 5: Initialize SQLite database for the selected profile
    QString dbPath = infrastructure::ConfigManager::getProfileDbPath(targetProfileId);
    if (!infrastructure::DatabaseManager::instance().initialize(dbPath)) {
        qCritical() << "Failed to initialize database. Exiting.";
        return -1;
    }
    
    // Step 6: Construct domain TabController (business logic orchestration)
    domain::TabController controller;
    
    // Step 7: Construct presentation OverlayWindow, injecting the domain controller
    presentation::OverlayWindow overlay(&controller);
    
    // Step 8: Load notes from SQLite; on first run with empty database, seed initial sample notes
    controller.loadNotes();
    if (controller.notes().isEmpty()) {
        domain::Note note1; note1.title = "Sample 1"; note1.content = ""; note1.color = "#FF5555";
        domain::Note note2; note2.title = "Sample 2"; note2.content = ""; note2.color = "#55FF55";
        domain::Note note3; note3.title = "Sample 3"; note3.content = ""; note3.color = "#5555FF";
        
        infrastructure::DatabaseManager::instance().saveNote(note1);
        infrastructure::DatabaseManager::instance().saveNote(note2);
        infrastructure::DatabaseManager::instance().saveNote(note3);
        
        // Refresh controller state with newly seeded notes
        controller.loadNotes();
    }
    
    // Step 9: Reveal overlay on screen and enter the primary application event loop
    overlay.show();
    
    return app.exec();
}

