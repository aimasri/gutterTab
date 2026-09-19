#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>

namespace infrastructure {

/**
 * @brief Retrieves the singleton instance of DatabaseManager.
 * @details Utilizes C++11 magic statics (Meyers' Singleton) for thread-safe lazy initialization.
 * @return Reference to the global DatabaseManager instance.
 */
DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

/**
 * @brief Closes the SQLite database connection upon destruction.
 * @details RAII cleanup ensuring any open database handle is flushed and closed gracefully.
 */
DatabaseManager::~DatabaseManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

/**
 * @brief Opens or connects to the SQLite database, executes schema migrations, and sets up tables.
 * @details Execution steps:
 *          1. Check for existing "qt_sql_default_connection"; reuse if present or register "QSQLITE" driver.
 *          2. Ensure the parent directory exists on disk using QDir::mkpath().
 *          3. Set database name to the requested dbPath and open the SQLite connection.
 *          4. Execute idempotent migration to add sort_order column to legacy databases.
 *          5. Invoke createTables() to enforce foreign keys and ensure notes table existence.
 * @param dbPath Absolute filesystem path to the profile's SQLite database file.
 * @return True if connection was established and initialization succeeded; false otherwise.
 */
bool DatabaseManager::initialize(const QString& dbPath) {
    // Step 1: Reuse existing default connection or instantiate new SQLite driver instance
    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        m_db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE");
    }
    
    // Step 2: Ensure destination directory exists on disk
    QFileInfo fi(dbPath);
    QDir().mkpath(fi.absolutePath());

    m_db.setDatabaseName(dbPath);

    // Step 3: Open database connection
    if (!m_db.open()) {
        qCritical() << "Error: connection with database fail:" << m_db.lastError().text();
        return false;
    }
    
    // Step 4: Run migration - ensure sort_order column exists on legacy schemas.
    // In SQLite, if the column already exists, this fails harmlessly without affecting data.
    QSqlQuery migration(m_db);
    migration.exec("ALTER TABLE notes ADD COLUMN sort_order INTEGER DEFAULT 0");
    
    // Step 5: Initialize tables and pragmas
    return createTables();
}

/**
 * @brief Enforces database pragmas and initializes the notes table schema if missing.
 * @details Execution steps:
 *          1. Execute `PRAGMA foreign_keys = ON` to enforce relational constraints.
 *          2. Execute `CREATE TABLE IF NOT EXISTS notes` defining:
 *             - id: INTEGER PRIMARY KEY AUTOINCREMENT
 *             - title: TEXT NOT NULL
 *             - content: TEXT (stores raw Markdown)
 *             - color: TEXT (hex color code, e.g. #333333)
 *             - keepSyncId: TEXT (external sync identifier)
 *             - sort_order: INTEGER DEFAULT 0 (manual reordering index)
 * @return True if table schema is verified or created successfully; false on SQL error.
 */
bool DatabaseManager::createTables() {
    QSqlQuery query(m_db);
    
    // Step 1: Enable SQLite foreign key constraint checking
    query.exec("PRAGMA foreign_keys = ON");
    
    // Step 2: Create notes table if it does not already exist
    bool success = query.exec(
        "CREATE TABLE IF NOT EXISTS notes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "title TEXT NOT NULL, "
        "content TEXT, "
        "color TEXT, "
        "keepSyncId TEXT, "
        "sort_order INTEGER DEFAULT 0"
        ")"
    );

    if (!success) {
        qCritical() << "Couldn't create notes table:" << query.lastError().text();
        return false;
    }
    
    // Create folder_shortcuts table
    success = query.exec(
        "CREATE TABLE IF NOT EXISTS folder_shortcuts ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "path TEXT NOT NULL, "
        "icon_path TEXT"
        ")"
    );

    if (!success) {
        qCritical() << "Couldn't create folder_shortcuts table:" << query.lastError().text();
        return false;
    }
    
    return true;
}

/**
 * @brief Retrieves a note entity by its primary key ID.
 * @details Uses parameterized query binding to prevent SQL injection vulnerabilities.
 * @param id Database primary key identifier.
 * @return std::optional containing the domain::Note if found; std::nullopt if not found or query fails.
 */
std::optional<domain::Note> DatabaseManager::getNote(int id) {
    QSqlQuery query(m_db);
    // Prepare parameterized statement
    query.prepare("SELECT title, content, color, keepSyncId, sort_order FROM notes WHERE id = :id");
    query.bindValue(":id", id);
    
    if (query.exec() && query.next()) {
        domain::Note note;
        note.id = id;
        note.title = query.value("title").toString();
        note.content = query.value("content").toString();
        note.color = query.value("color").toString();
        note.keepSyncId = query.value("keepSyncId").toString();
        note.sort_order = query.value("sort_order").toInt();
        return note;
    }
    return std::nullopt;
}

/**
 * @brief Retrieves all notes ordered by their display sequence.
 * @details Sorts primarily by sort_order ascending, using primary key id as a tie-breaker.
 * @return QVector of domain::Note entities.
 */
QVector<domain::Note> DatabaseManager::getAllNotes() {
    QVector<domain::Note> notes;
    QSqlQuery query("SELECT id, title, content, color, keepSyncId, sort_order FROM notes ORDER BY sort_order ASC, id ASC", m_db);
    
    while (query.next()) {
        domain::Note note;
        note.id = query.value("id").toInt();
        note.title = query.value("title").toString();
        note.content = query.value("content").toString();
        note.color = query.value("color").toString();
        note.keepSyncId = query.value("keepSyncId").toString();
        note.sort_order = query.value("sort_order").toInt();
        notes.push_back(note);
    }
    
    return notes;
}

/**
 * @brief Saves a note to the database via INSERT or UPDATE depending on primary key ID.
 * @details Execution steps:
 *          - If note.id <= 0: Executes parameterized INSERT query and mutates note.id
 *            with SQLite's generated lastInsertId().
 *          - If note.id > 0: Executes parameterized UPDATE query matching WHERE id = :id.
 * @param note Reference to domain::Note object to be inserted or updated.
 * @return True on successful database write; false on query failure.
 */
bool DatabaseManager::saveNote(domain::Note& note) {
    QSqlQuery query(m_db);
    
    if (note.id <= 0) {
        // Step 1a: Insert new record
        query.prepare("INSERT INTO notes (title, content, color, keepSyncId) VALUES (:title, :content, :color, :keepSyncId)");
        query.bindValue(":title", note.title);
        query.bindValue(":content", note.content);
        query.bindValue(":color", note.color);
        query.bindValue(":keepSyncId", note.keepSyncId);
        
        if (query.exec()) {
            // Populate assigned primary key back into the domain entity
            note.id = query.lastInsertId().toInt();
            return true;
        }
    } else {
        // Step 1b: Update existing record
        query.prepare("UPDATE notes SET title = :title, content = :content, color = :color, keepSyncId = :keepSyncId WHERE id = :id");
        query.bindValue(":title", note.title);
        query.bindValue(":content", note.content);
        query.bindValue(":color", note.color);
        query.bindValue(":keepSyncId", note.keepSyncId);
        query.bindValue(":id", note.id);
        
        return query.exec();
    }
    
    qCritical() << "Failed to save note:" << query.lastError().text();
    return false;
}

/**
 * @brief Deletes a note by its primary key ID.
 * @details Executes parameterized DELETE FROM notes WHERE id = :id.
 * @param id Primary key ID of the note to remove.
 * @return True if deletion query succeeded; false on SQL error.
 */
bool DatabaseManager::deleteNote(int id) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM notes WHERE id = :id");
    query.bindValue(":id", id);
    
    if (!query.exec()) {
        qCritical() << "Failed to delete note:" << query.lastError().text();
        return false;
    }
    return true;
}

/**
 * @brief Rearranges notes in the display order and updates sort_order values inside an atomic transaction.
 * @details Execution steps:
 *          1. Retrieve current ordered list of all notes.
 *          2. Locate the dragged note by draggedId and record its oldIndex.
 *          3. If the note is missing or already at targetIndex, exit early.
 *          4. Remove dragged note from oldIndex and clamp targetIndex to vector boundaries.
 *          5. Insert dragged note into targetIndex.
 *          6. Open SQLite transaction (`m_db.transaction()`) to ensure atomic batch update.
 *          7. Iterate reordered vector and update `sort_order = i` for each note id.
 *          8. Commit transaction (`m_db.commit()`), ensuring all-or-nothing consistency.
 * @param draggedId Primary key ID of the note being moved.
 * @param targetIndex Target zero-based display index.
 */
void DatabaseManager::reorderNotes(int draggedId, int targetIndex) {
    // Step 1: Retrieve all notes in current sequence
    auto notes = getAllNotes();
    
    // Step 2: Locate dragged note and its current index
    domain::Note draggedNote;
    int oldIndex = -1;
    for (int i = 0; i < notes.size(); ++i) {
        if (notes[i].id == draggedId) {
            draggedNote = notes[i];
            oldIndex = i;
            break;
        }
    }
    
    // Step 3: Early exit if note not found or position is unchanged
    if (oldIndex == -1 || oldIndex == targetIndex) return;
    
    // Step 4: Reorder in memory
    notes.removeAt(oldIndex);
    
    // Clamp target index if necessary
    if (targetIndex > notes.size()) {
        targetIndex = notes.size();
    }
    
    notes.insert(targetIndex, draggedNote);
    
    // Step 5: Commit batch updates atomically within a transaction
    QSqlQuery query(m_db);
    m_db.transaction();
    for (int i = 0; i < notes.size(); ++i) {
        query.prepare("UPDATE notes SET sort_order = :sort_order WHERE id = :id");
        query.bindValue(":sort_order", i);
        query.bindValue(":id", notes[i].id);
        query.exec();
    }
    m_db.commit();
}

std::optional<domain::FolderShortcut> DatabaseManager::getFolderShortcut(int id) {
    QSqlQuery query(m_db);
    query.prepare("SELECT name, path, icon_path FROM folder_shortcuts WHERE id = :id");
    query.bindValue(":id", id);
    
    if (query.exec() && query.next()) {
        domain::FolderShortcut shortcut;
        shortcut.id = id;
        shortcut.name = query.value("name").toString();
        shortcut.path = query.value("path").toString();
        shortcut.iconPath = query.value("icon_path").toString();
        return shortcut;
    }
    return std::nullopt;
}

QVector<domain::FolderShortcut> DatabaseManager::getAllFolderShortcuts() {
    QVector<domain::FolderShortcut> shortcuts;
    // Order alphabetically by name (case insensitive ideally)
    QSqlQuery query("SELECT id, name, path, icon_path FROM folder_shortcuts ORDER BY name COLLATE NOCASE ASC", m_db);
    
    while (query.next()) {
        domain::FolderShortcut shortcut;
        shortcut.id = query.value("id").toInt();
        shortcut.name = query.value("name").toString();
        shortcut.path = query.value("path").toString();
        shortcut.iconPath = query.value("icon_path").toString();
        shortcuts.push_back(shortcut);
    }
    return shortcuts;
}

bool DatabaseManager::saveFolderShortcut(domain::FolderShortcut& shortcut) {
    QSqlQuery query(m_db);
    
    if (shortcut.id <= 0) {
        query.prepare("INSERT INTO folder_shortcuts (name, path, icon_path) VALUES (:name, :path, :icon_path)");
        query.bindValue(":name", shortcut.name);
        query.bindValue(":path", shortcut.path);
        query.bindValue(":icon_path", shortcut.iconPath);
        
        if (query.exec()) {
            shortcut.id = query.lastInsertId().toInt();
            return true;
        }
    } else {
        query.prepare("UPDATE folder_shortcuts SET name = :name, path = :path, icon_path = :icon_path WHERE id = :id");
        query.bindValue(":name", shortcut.name);
        query.bindValue(":path", shortcut.path);
        query.bindValue(":icon_path", shortcut.iconPath);
        query.bindValue(":id", shortcut.id);
        
        return query.exec();
    }
    
    qCritical() << "Failed to save folder shortcut:" << query.lastError().text();
    return false;
}

bool DatabaseManager::deleteFolderShortcut(int id) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM folder_shortcuts WHERE id = :id");
    query.bindValue(":id", id);
    
    if (!query.exec()) {
        qCritical() << "Failed to delete folder shortcut:" << query.lastError().text();
        return false;
    }
    return true;
}

} // namespace infrastructure
