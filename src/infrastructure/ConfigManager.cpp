#include "ConfigManager.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QStandardPaths>
#include <QJsonArray>
#include <QUuid>

namespace infrastructure {

/**
 * @brief Retrieves the singleton instance of ConfigManager.
 * @details Utilizes C++11 magic statics (Meyers' Singleton) to ensure thread-safe,
 *          lazy initialization on first access and automatic destruction on application teardown.
 * @return Reference to the global ConfigManager instance.
 */
ConfigManager& ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

/**
 * @brief Computes the absolute filesystem path to the global profiles registry JSON file.
 * @details Located at `<QStandardPaths::AppDataLocation>/profiles.json`
 *          (typically `~/.config/gutterTab/profiles.json` on Linux/X11).
 * @return Path string to profiles.json.
 */
QString ConfigManager::getProfilesRegistryPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles.json";
}

/**
 * @brief Computes the absolute filesystem path to a profile's geometry configuration file.
 * @param profileId UUID string identifying the profile.
 * @return Path string to `<AppDataLocation>/profiles/<profileId>/config.json`.
 */
QString ConfigManager::getProfileConfigPath(const QString& profileId) {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles/" + profileId + "/config.json";
}

/**
 * @brief Computes the absolute filesystem path to a profile's SQLite database file.
 * @param profileId UUID string identifying the profile.
 * @return Path string to `<AppDataLocation>/profiles/<profileId>/notes.db`.
 */
QString ConfigManager::getProfileDbPath(const QString& profileId) {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles/" + profileId + "/notes.db";
}

/**
 * @brief Computes the absolute path to a profile's attachments directory and guarantees existence.
 * @details Creates all intermediate directories using QDir::mkpath() if not already present.
 * @param profileId UUID string identifying the profile.
 * @return Path string to `<AppDataLocation>/profiles/<profileId>/attachments`.
 */
QString ConfigManager::getProfileAttachmentsPath(const QString& profileId) {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles/" + profileId + "/attachments";
    // Ensure the attachments folder exists before returning
    QDir().mkpath(path);
    return path;
}

/**
 * @brief Reads and parses all registered user profiles from profiles.json.
 * @details Execution steps:
 *          1. Open profiles.json in ReadOnly mode.
 *          2. If missing or unreadable, return an empty vector gracefully.
 *          3. Parse JSON document and extract the "profiles" JSON array.
 *          4. Iterate each object and populate ProfileInfo (id, displayName, accentColor).
 * @return QVector of ProfileInfo structs representing all known profiles.
 */
QVector<ProfileInfo> ConfigManager::listProfiles() {
    QVector<ProfileInfo> profiles;
    QFile file(getProfilesRegistryPath());

    // Step 1: Attempt to open the registry file
    if (file.open(QIODevice::ReadOnly)) {
        // Step 2: Parse the root JSON document
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonArray arr = doc.object()["profiles"].toArray();

        // Step 3: Deserialize each profile record into a ProfileInfo struct
        for (const auto& val : arr) {
            QJsonObject obj = val.toObject();
            ProfileInfo info;
            info.id = obj["id"].toString();
            info.displayName = obj["displayName"].toString();
            info.accentColor = QColor(obj["accentColor"].toString());
            profiles.append(info);
        }
    }
    return profiles;
}

/**
 * @brief Helper function to serialize profile metadata and auto-launch preference to disk.
 * @details Constructs the canonical registry JSON document:
 *          @code
 *          {
 *              "profiles": [
 *                  { "id": "...", "displayName": "...", "accentColor": "#..." }
 *              ],
 *              "autoLaunchProfile": "..."
 *          }
 *          @endcode
 *          Ensures directory existence and writes atomically using Qt's QFile.
 * @param profiles List of all current ProfileInfo objects.
 * @param autoLaunchId UUID of the auto-launch profile, or empty string if disabled.
 */
static void saveProfilesRegistry(const QVector<ProfileInfo>& profiles, const QString& autoLaunchId) {
    // Step 1: Build the JSON structures
    QJsonObject root;
    QJsonArray arr;
    for (const auto& p : profiles) {
        QJsonObject obj;
        obj["id"] = p.id;
        obj["displayName"] = p.displayName;
        obj["accentColor"] = p.accentColor.name();
        arr.append(obj);
    }
    root["profiles"] = arr;
    root["autoLaunchProfile"] = autoLaunchId;
    
    // Step 2: Ensure destination directory exists
    QFile file(ConfigManager::getProfilesRegistryPath());
    QDir().mkpath(QFileInfo(file).absolutePath());

    // Step 3: Write serialized JSON payload
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
    }
}

/**
 * @brief Creates a new user profile, registers it, and seeds default configuration.
 * @details Execution steps:
 *          1. Load existing profiles and active auto-launch setting.
 *          2. Generate a fresh UUID v4 without curly braces for clean filesystem paths.
 *          3. Assemble ProfileInfo and append to registry.
 *          4. Persist updated registry via saveProfilesRegistry().
 *          5. Instantiate a temporary ConfigManager and save default config (Edge::Right) for the new profile.
 * @param displayName Human-readable name for the profile.
 * @param accentColor UI highlight color.
 * @return True when profile creation completes successfully.
 */
bool ConfigManager::createProfile(const QString& displayName, const QColor& accentColor) {
    auto profiles = listProfiles();
    QString autoLaunch = getAutoLaunchProfile();
    
    // Step 1: Generate UUID without enclosing braces
    ProfileInfo info;
    info.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    info.displayName = displayName;
    info.accentColor = accentColor;
    
    // Step 2: Update registry file
    profiles.append(info);
    saveProfilesRegistry(profiles, autoLaunch);
    
    // Step 3: Seed initial configuration file with default Right edge docking
    ConfigManager temp;
    temp.m_config.edge = Config::Edge::Right;
    temp.save(info.id);
    
    return true;
}

/**
 * @brief Removes a profile from the registry and deletes its directory tree.
 * @details Execution steps:
 *          1. Load existing profiles and auto-launch preference.
 *          2. If the deleted profile matches autoLaunchProfile, clear the auto-launch key.
 *          3. Locate and remove the profile entry from the registry list.
 *          4. Persist updated registry to profiles.json.
 *          5. Recursively remove the profile directory (`<AppDataLocation>/profiles/<profileId>`).
 * @param profileId UUID string of the profile to remove.
 * @return True upon successful removal.
 */
bool ConfigManager::deleteProfile(const QString& profileId) {
    auto profiles = listProfiles();
    QString autoLaunch = getAutoLaunchProfile();

    // Reset auto-launch if the deleted profile was designated as default
    if (autoLaunch == profileId) autoLaunch = "";
    
    // Remove profile entry from in-memory list
    for (int i = 0; i < profiles.size(); ++i) {
        if (profiles[i].id == profileId) {
            profiles.removeAt(i);
            break;
        }
    }
    saveProfilesRegistry(profiles, autoLaunch);
    
    // Recursively delete all profile assets, config, and SQLite database
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles/" + profileId);
    if (dir.exists()) {
        dir.removeRecursively();
    }
    return true;
}

/**
 * @brief Updates the human-readable display name of an existing profile.
 * @param profileId UUID string of the profile to rename.
 * @param newDisplayName New display name string.
 * @return True if profile was found and updated; false if profileId does not exist.
 */
bool ConfigManager::renameProfile(const QString& profileId, const QString& newDisplayName) {
    auto profiles = listProfiles();
    QString autoLaunch = getAutoLaunchProfile();
    for (auto& p : profiles) {
        if (p.id == profileId) {
            p.displayName = newDisplayName;
            saveProfilesRegistry(profiles, autoLaunch);
            return true;
        }
    }
    return false;
}

/**
 * @brief Updates the theme accent color of an existing profile.
 * @param profileId UUID string of the profile to modify.
 * @param newColor New visual accent color.
 * @return True if profile was found and updated; false if profileId does not exist.
 */
bool ConfigManager::updateProfileColor(const QString& profileId, const QColor& newColor) {
    auto profiles = listProfiles();
    QString autoLaunch = getAutoLaunchProfile();
    for (auto& p : profiles) {
        if (p.id == profileId) {
            p.accentColor = newColor;
            saveProfilesRegistry(profiles, autoLaunch);
            return true;
        }
    }
    return false;
}

/**
 * @brief Reads the current auto-launch profile setting from profiles.json.
 * @return Profile UUID string if configured, or empty string if disabled/unreadable.
 */
QString ConfigManager::getAutoLaunchProfile() {
    QFile file(getProfilesRegistryPath());
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        return doc.object()["autoLaunchProfile"].toString();
    }
    return "";
}

/**
 * @brief Sets or updates the auto-launch profile preference in profiles.json.
 * @param profileId UUID of the profile to auto-launch on startup, or empty string to clear.
 */
void ConfigManager::setAutoLaunchProfile(const QString& profileId) {
    auto profiles = listProfiles();
    saveProfilesRegistry(profiles, profileId);
}

/**
 * @brief Loads geometry and docking settings from a profile's config.json into memory.
 * @details Execution steps:
 *          1. Sets m_activeProfileId to profileId.
 *          2. Attempts to open `<AppDataLocation>/profiles/<profileId>/config.json`.
 *          3. If missing, logs a warning and calls save(profileId) to write default settings.
 *          4. If present, deserializes "edge", "gutterRestWidth", "gutterHoverWidth", and "gutterPeekWidth".
 * @param profileId UUID of the profile to load.
 * @return True if file was successfully parsed; false if defaults were written or file was missing.
 */
bool ConfigManager::load(const QString& profileId) {
    m_activeProfileId = profileId;
    QString path = getProfileConfigPath(profileId);
    QFile file(path);

    // Fallback: If config file is missing, initialize with defaults and save
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open config file, creating defaults at:" << path;
        save(profileId);
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc(QJsonDocument::fromJson(data));
    QJsonObject obj = doc.object();

    // Parse screen docking edge ("left" vs "right")
    if (obj.contains("edge")) {
        QString edgeStr = obj["edge"].toString();
        if (edgeStr.toLower() == "left") {
            m_config.edge = Config::Edge::Left;
        } else {
            m_config.edge = Config::Edge::Right;
        }
    }
    
    // Parse pixel dimensions
    if (obj.contains("gutterRestWidth")) m_config.gutterRestWidth = obj["gutterRestWidth"].toInt();
    if (obj.contains("gutterHoverWidth")) m_config.gutterHoverWidth = obj["gutterHoverWidth"].toInt();
    if (obj.contains("gutterPeekWidth")) m_config.gutterPeekWidth = obj["gutterPeekWidth"].toInt();

    return true;
}

/**
 * @brief Serializes current in-memory configuration to the profile's config.json.
 * @details Execution steps:
 *          1. Build QJsonObject with edge and width metrics.
 *          2. Ensure parent directories exist via QDir::mkpath().
 *          3. Open target file in WriteOnly mode and write formatted JSON.
 * @param profileId UUID of the profile to write.
 * @return True if save succeeded; false if file could not be opened.
 */
bool ConfigManager::save(const QString& profileId) {
    QString path = getProfileConfigPath(profileId);
    QJsonObject obj;

    // Map enum to string value
    obj["edge"] = (m_config.edge == Config::Edge::Left) ? "left" : "right";
    obj["gutterRestWidth"] = m_config.gutterRestWidth;
    obj["gutterHoverWidth"] = m_config.gutterHoverWidth;
    obj["gutterPeekWidth"] = m_config.gutterPeekWidth;

    QJsonDocument doc(obj);
    
    // Ensure directory exists
    QFileInfo fi(path);
    QDir().mkpath(fi.absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qCritical() << "Could not save config file:" << path;
        return false;
    }

    file.write(doc.toJson());
    return true;
}

} // namespace infrastructure
