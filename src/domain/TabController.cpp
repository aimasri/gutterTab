#include "TabController.h"
#include "../infrastructure/DatabaseManager.h"
#include <QDebug>

namespace domain {

TabController::TabController(QObject* parent) : QObject(parent) {
    // Controller initializes in State::IDLE with m_activeNoteId = -1 (no active note selected).
}

void TabController::loadNotes() {
    // Step 1: Fetch all persisted notes ordered by sort_order from SQLite via DatabaseManager.
    // Step 2: Overwrite the in-memory cache m_notes with the freshly loaded record set.
    m_notes = infrastructure::DatabaseManager::instance().getAllNotes();

    // Step 3: Notify presentation observers (e.g. GutterStrip) to synchronize their UI tabs.
    emit notesLoaded();
}

void TabController::createNewNote() {
    // Step 1: Assemble a new Note entity with initial default parameters.
    Note newNote;
    newNote.title = "New Prompt";
    newNote.content = "";
    newNote.color = "#FF5555"; // Default red accent color
    
    // Step 2: Write-through persist the new note to SQLite.
    // DatabaseManager::saveNote inserts the row and updates newNote.id in-place with the autoincrement ID.
    if (infrastructure::DatabaseManager::instance().saveNote(newNote)) {
        // Step 3: Invalidate and refresh in-memory cache to maintain strict ordering and persistence parity.
        loadNotes();

        // Step 4: Immediately transition the state machine to State::OPEN and activate the new note.
        openNote(newNote.id);
    }
}

void TabController::updateNoteContent(int id, const QString& content) {
    // Linear scan of in-memory cache to find target note.
    // For small scratchpad sets (typically < 50 notes), vector iteration avoids map overhead.
    for (auto& note : m_notes) {
        if (note.id == id) {
            // Step 1: Update in-memory content cache for immediate consistency.
            note.content = content;

            // Step 2: Write-through update to SQLite database.
            infrastructure::DatabaseManager::instance().saveNote(note);
            break;
        }
    }
}

void TabController::deleteNote(int id) {
    // Step 1: Attempt deletion in persistent database storage first.
    if (infrastructure::DatabaseManager::instance().deleteNote(id)) {
        // Step 2: Check if the deleted note is currently being viewed or edited in the overlay.
        // If active, safely dismiss the editor and transition FSM state back to State::IDLE.
        if (m_activeNoteId == id) {
            closeNote();
        }

        // Step 3: Refresh in-memory cache to reflect the removal and emit notesLoaded() to update UI.
        loadNotes();
    }
}

void TabController::setState(State state) {
    // State machine transition logic with deduplication:
    // Only apply state change and fire signal if the new state differs from the current state.
    if (m_state != state) {
        m_state = state;

        // Notify presentation layer (OverlayWindow, GutterStrip) to adjust window geometry,
        // input transparency, and visual animations.
        emit stateChanged(m_state);
    }
}

void TabController::openNote(int noteId) {
    // Step 1: Record active note primary key identifier.
    m_activeNoteId = noteId;

    // Step 2: Transition finite state machine to OPEN state.
    setState(State::OPEN);

    // Step 3: Notify UI listeners (e.g. NoteEditorOverlay) to load content and display the editor.
    emit activeNoteChanged(noteId);
}

void TabController::closeNote() {
    // Step 1: Clear active note identifier using sentinel -1.
    m_activeNoteId = -1;

    // Step 2: Transition finite state machine back to IDLE state.
    setState(State::IDLE);

    // Step 3: Notify UI listeners that no note is active so they can hide overlays.
    emit activeNoteChanged(-1);
}

void TabController::toggleDashboard() {
    if (m_state == State::DASHBOARD) {
        setState(State::IDLE);
    } else {
        // If a note was open, close it first.
        if (m_state == State::OPEN) {
            closeNote();
        }
        setState(State::DASHBOARD);
    }
}

void TabController::reorderNote(int id, int newIndex) {
    // Step 1: Persist the new tab sequence in DatabaseManager.
    // DatabaseManager executes an atomic transaction that re-indexes sort_order across notes.
    infrastructure::DatabaseManager::instance().reorderNotes(id, newIndex);

    // Step 2: Invalidate and synchronize the in-memory cache with the new persisted order.
    // loadNotes() re-queries all notes ordered by sort_order and emits notesLoaded().
    loadNotes();
}

} // namespace domain
