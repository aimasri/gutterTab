#pragma once

#include <QString>

namespace domain {

/**
 * @brief Domain model representing a persistent folder shortcut.
 * 
 * @details This struct is a plain data object used by the domain and presentation layers
 *          to manage shortcuts to local system folders. It is serialized and deserialized
 *          by the infrastructure::DatabaseManager.
 */
struct FolderShortcut {
    int id = -1;              ///< Database primary key. -1 indicates unsaved.
    QString name;             ///< Display name of the shortcut.
    QString path;             ///< Absolute path to the directory on the local filesystem.
    QString iconPath;         ///< Absolute path to the custom brand mark/icon image, if any.
};

} // namespace domain
