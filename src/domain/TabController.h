#pragma once

#include <QObject>
#include <QVector>
#include "Note.h"

namespace domain {

/**
 * @brief Domain coordinator managing note lifecycle, presentation states, and persistence synchronization.
 * 
 * @details Acts as the central domain service and finite state machine (FSM) for the gutterTab
 * application. In accordance with Domain-Driven Design (DDD) principles, TabController mediates
 * between the presentation layer (OverlayWindow, GutterStrip, NoteEditorOverlay) and the underlying
 * data infrastructure (infrastructure::DatabaseManager).
 * 
 * Architectural Role & State Machine:
 * - TabController maintains a tri-state machine (State::IDLE, State::PEEKING, State::OPEN) governing
 *   how the edge gutter strip and modal note overlays are presented to the user.
 * - Presentation components subscribe reactively to TabController via Qt signals (stateChanged,
 *   notesLoaded, activeNoteChanged) rather than directly interrogating or mutating the SQLite database.
 * 
 * Performance & Caching Trade-offs:
 * - TabController maintains an in-memory cache (m_notes) of domain::Note entities. This provides O(1)
 *   random access for rendering tabs and layout calculations without incurring repetitive SQLite queries
 *   during high-frequency mouse hover or animation events.
 * - Write operations (createNewNote, updateNoteContent, deleteNote, reorderNote) write-through to
 *   DatabaseManager and refresh the in-memory cache to guarantee persistence consistency.
 * 
 * @note Thread Safety & Affinity: TabController inherits from QObject and is strictly affined to
 * the Qt main GUI thread. All member functions, slot invocations, and signal emissions must occur
 * on the GUI thread. Concurrent access across worker threads without Qt queued signal/slot connections
 * or explicit thread synchronization violates memory safety guarantees.
 */
class TabController : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Visual and operational states of the gutter tab interface.
     */
    enum class State {
        /// @brief Default idle state; gutter tabs are collapsed to minimal width and editor overlay is hidden.
        IDLE,
        /// @brief Hover/peek state; gutter tabs expand partially to reveal labels and color indicators.
        PEEKING,
        /// @brief Active editing state; a note is open in NoteEditorOverlay with background dimming enabled.
        OPEN,
        /// @brief Dashboard mode; all notes are displayed in a full-screen bento grid.
        DASHBOARD
    };

    /**
     * @brief Constructs a TabController instance.
     * 
     * @param parent Optional parent QObject for Qt's hierarchical ownership tree.
     * @note If parent is nullptr, the caller is responsible for the controller's lifetime management.
     */
    explicit TabController(QObject* parent = nullptr);
    
    /**
     * @brief Loads all persisted notes from the database into the in-memory cache and emits notesLoaded().
     * 
     * @details Queries infrastructure::DatabaseManager for all stored notes ordered by sort_order.
     * Overwrites m_notes and fires notesLoaded() to signal presentation layers to reconstruct or update tabs.
     * 
     * Edge Cases:
     * - If the database is empty or uninitialized, m_notes becomes empty; handled gracefully by the UI.
     */
    void loadNotes();

    /**
     * @brief Provides read-only access to the in-memory cache of domain notes.
     * 
     * @return Const reference to QVector<Note> sorted by sort_order.
     * @note The returned reference is valid only until the next operation that modifies m_notes
     * (e.g., loadNotes(), deleteNote()).
     */
    const QVector<Note>& notes() const { return m_notes; }
    
    /**
     * @brief Creates a new default note, persists it to storage, reloads the cache, and activates it.
     * 
     * @details Generates a default note with placeholder title "New Prompt" and default color,
     * writes it through DatabaseManager, updates in-memory notes, and triggers openNote() with
     * the newly assigned database primary key.
     * 
     * Edge Cases:
     * - If DatabaseManager::saveNote fails, the note is neither cached nor opened, preventing UI desync.
     */
    void createNewNote();

    /**
     * @brief Updates the Markdown content of an existing note both in-memory and on disk.
     * 
     * @param id Database primary key of the note to update.
     * @param content Updated Markdown text string.
     * 
     * Edge Cases:
     * - If no note matching id exists in m_notes, the call safely no-ops without throwing or persisting.
     */
    void updateNoteContent(int id, const QString& content);

    /**
     * @brief Deletes a note by primary key from persistent storage and refreshes the cache.
     * 
     * @param id Database primary key of the note to delete.
     * 
     * Edge Cases:
     * - If the note being deleted is currently open (m_activeNoteId == id), closeNote() is
     *   automatically invoked first to safely dismiss the editor overlay.
     * - If the database deletion fails, in-memory state remains untouched.
     */
    void deleteNote(int id);

    /**
     * @brief Reorders a note to a target index position and synchronizes sort orders.
     * 
     * @param id Database primary key of the dragged/repositioned note.
     * @param newIndex Destination zero-based index in the visual list.
     * 
     * Edge Cases:
     * - DatabaseManager clamps out-of-bounds indices; loadNotes() is called to guarantee in-memory order
     *   matches database persistence.
     */
    void reorderNote(int id, int newIndex);
    
    /**
     * @brief Retrieves the current state of the state machine.
     * 
     * @return Current State (IDLE, PEEKING, OPEN, or DASHBOARD).
     */
    State currentState() const { return m_state; }

    /**
     * @brief Retrieves the database ID of the currently open/active note.
     * 
     * @return Positive integer ID if a note is active; -1 if no note is open.
     */
    int activeNoteId() const { return m_activeNoteId; }

public slots:
    /**
     * @brief Transitions the state machine to a new visual state if different from the current state.
     * 
     * @param state The target State to transition to.
     * 
     * Edge Cases:
     * - Deduplication: If state matches m_state, the transition is skipped and stateChanged() is not emitted.
     */
    void setState(State state);

    /**
     * @brief Activates a note for editing, sets state to OPEN, and notifies listeners.
     * 
     * @param noteId Database primary key of the note to open.
     * 
     * Edge Cases:
     * - Always transitions state to State::OPEN via setState() and emits activeNoteChanged(noteId).
     */
    void openNote(int noteId);

    /**
     * @brief Closes the currently active note, resets state to IDLE, and notifies listeners.
     * 
     * Edge Cases:
     * - Resets m_activeNoteId to -1, transitions state to State::IDLE via setState(), and
     *   emits activeNoteChanged(-1).
     */
    void closeNote();

    /**
     * @brief Toggles between DASHBOARD state and IDLE state.
     */
    void toggleDashboard();

signals:
    /**
     * @brief Emitted when the state machine transitions between IDLE, PEEKING, and OPEN states.
     * 
     * @param newState The new State after the transition.
     */
    void stateChanged(State newState);

    /**
     * @brief Emitted after notes have been fetched from the database and cached in m_notes.
     */
    void notesLoaded();

    /**
     * @brief Emitted when the active note selection changes.
     * 
     * @param noteId Database primary key of the newly active note, or -1 if closed.
     */
    void activeNoteChanged(int noteId);

private:
    /// @brief Current operational visual state of the gutter interface.
    State m_state = State::IDLE;

    /// @brief Database ID of the active note open in the editor overlay (-1 indicates none).
    int m_activeNoteId = -1;

    /// @brief Cached list of notes loaded from the database, sorted by sort_order.
    QVector<Note> m_notes;
};

} // namespace domain
