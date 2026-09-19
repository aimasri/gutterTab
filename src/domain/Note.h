#pragma once

#include <QString>

namespace domain {

/**
 * @brief Core domain entity representing a single prompt or note tab.
 * 
 * @details Encapsulates the persistent and in-memory attributes of a scratchpad note
 * within the gutterTab application. In terms of Domain-Driven Design (DDD), Note serves
 * as a lightweight value entity designed for seamless copying, serialization to SQLite
 * via infrastructure::DatabaseManager, and reactive binding across presentation components
 * (GutterStrip, NoteEditorOverlay).
 * 
 * Architectural Notes & Trade-offs:
 * - Designed as a plain struct with value semantics rather than an active record or
 *   QObject-derived class. This eliminates QObject heap allocation overhead and avoids
 *   complex lifetime/parenting management when passing collections of notes between layers.
 * - Persistence operations are intentionally decoupled and handled by DatabaseManager.
 * 
 * @note Thread Safety: Note is a pure value type and does not possess internal synchronization.
 * Concurrent mutations across threads without external synchronization are unsafe; access
 * should be confined to the Qt GUI thread.
 */
struct Note {
    /// @brief Unique database primary key identifier (0 indicates unpersisted/transient note).
    int id = 0;

    /// @brief Display title or prompt label shown on the gutter tab or its tooltip.
    QString title;

    /// @brief Raw Markdown formatted body text of the note, edited in the overlay.
    QString content; // Markdown text

    /// @brief Hexadecimal color string (e.g., "#333333", "#FF5555") used for the tab background accent.
    QString color = "#333333";   // Hex color for the tab

    /// @brief External synchronization identifier (e.g., Google Keep note ID) for cloud sync.
    QString keepSyncId;

    /// @brief Zero-based index defining the vertical positioning and sort order in the gutter strip.
    int sort_order = 0;
};

} // namespace domain
