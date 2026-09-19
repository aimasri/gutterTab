#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>
#include <optional>

#include "../domain/Note.h"
#include "../domain/FolderShortcut.h"

namespace infrastructure {

/**
 * @brief SQLite database persistence layer and CRUD repository for notes.
 * @details Encapsulates low-level SQLite database management through Qt's QSqlDatabase and QSqlQuery abstractions.
 *          Responsible for connection lifecycle, schema initialization (notes table), idempotent schema migrations
 *          (such as adding the sort_order column to legacy schemas), foreign key pragma enforcement, transactional
 *          reordering algorithms, and note CRUD operations. Adheres to the Repository pattern to decouple data storage
 *          from domain models (domain::Note) and presentation components.
 * @note Thread safety: SQLite connection handles in Qt are thread-affine. DatabaseManager is designed for single-threaded
 *       usage on the main GUI thread. Connection name defaults to "qt_sql_default_connection". Transactions are used
 *       during bulk operations like reorderNotes() to guarantee ACID properties and prevent intermediate inconsistent state.
 */
class DatabaseManager {
public:
    /**
     * @brief Retrieves the singleton instance of DatabaseManager.
     * @return Reference to the static DatabaseManager instance.
     * @note Uses Meyers' Singleton pattern.
     */
    static DatabaseManager& instance();

    /**
     * @brief Opens or establishes the SQLite connection to the specified database file, applies migrations, and creates tables.
     * @param dbPath Absolute filesystem path to the profile's SQLite database file (e.g. `<AppDataLocation>/profiles/<profileId>/notes.db`).
     * @return True if the database was opened and initialized successfully, false on error.
     * @note Ensures parent directory exists using QDir::mkpath(). Executes an idempotent migration for sort_order
     *       before verifying table schema via createTables().
     */
    bool initialize(const QString& dbPath);
    
    // CRUD Operations

    /**
     * @brief Fetches a single note by its primary key ID.
     * @param id Unique database ID of the note.
     * @return std::optional<domain::Note> containing the note if found, or std::nullopt if no matching record exists or query fails.
     */
    std::optional<domain::Note> getNote(int id);

    /**
     * @brief Retrieves all notes sorted in ascending display order.
     * @return QVector<domain::Note> ordered by `sort_order ASC, id ASC`.
     */
    QVector<domain::Note> getAllNotes();

    /**
     * @brief Persists a note by inserting a new row or updating an existing one.
     * @param note In-out reference to a domain::Note. If note.id <= 0, an INSERT query is executed
     *             and note.id is updated with SQLite's auto-generated primary key via lastInsertId().
     *             If note.id > 0, an UPDATE query is executed.
     * @return True if the insert or update operation succeeded, false on SQL query failure.
     */
    bool saveNote(domain::Note& note); // Updates note.id if it's a new insert

    /**
     * @brief Deletes a note from the database by its primary key ID.
     * @param id Unique database ID of the note to remove.
     * @return True if the deletion succeeded, false on SQL failure.
     */
    bool deleteNote(int id);

    /**
     * @brief Atomically reorders notes and updates sort indices in the database.
     * @param draggedId Primary key ID of the note being dragged/moved.
     * @param targetIndex Zero-based destination index in the ordered list.
     * @details Fetches all notes ordered by current sort order, locates the dragged note, clamps target index
     *          within valid bounds, rearranges elements in memory, and writes updated zero-based sort_order values
     *          to the database inside an atomic transaction (m_db.transaction() / m_db.commit()).
     * @note If draggedId is not found or already at targetIndex, exits early without executing queries.
     */
    void reorderNotes(int draggedId, int targetIndex);

    // Folder Shortcuts CRUD
    std::optional<domain::FolderShortcut> getFolderShortcut(int id);
    QVector<domain::FolderShortcut> getAllFolderShortcuts();
    bool saveFolderShortcut(domain::FolderShortcut& shortcut);
    bool deleteFolderShortcut(int id);

private:
    DatabaseManager() = default;
    ~DatabaseManager();

    // Prevent copying
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    /**
     * @brief Configures database pragmas and creates the notes table if missing.
     * @return True if table creation succeeded or table already exists, false on SQL error.
     * @note Enforces SQLite foreign keys via `PRAGMA foreign_keys = ON`.
     */
    bool createTables();
    
    QSqlDatabase m_db;
};

} // namespace infrastructure
