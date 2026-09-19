#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVector>
#include <QColor>

namespace infrastructure {

/**
 * @brief Runtime layout and visual geometry configuration for the gutter tab.
 * @details Encapsulates display-side settings including screen edge attachment (Left or Right)
 *          and dimensional metrics across interactive states: rest width (collapsed strip),
 *          hover width (expanded peek zone), and peek width. Implemented as a POD-like value struct
 *          to facilitate clean serialization to/from JSON and decoupled distribution to presentation components.
 * @note Not thread-safe; intended to be accessed on the main GUI thread. Default values are chosen to
 *       balance minimal screen occlusion (2px rest) with ergonomic hover hit-testing (26px hover, 30px peek).
 */
struct Config {
    /**
     * @brief Screen edge attachment enumeration.
     */
    enum class Edge { Left, Right };
    
    Edge edge = Edge::Right;           ///< Screen boundary where the gutter tab is docked.
    int gutterRestWidth = 2;          ///< Width of the gutter strip in pixels when idle/unhovered.
    int gutterHoverWidth = 26;        ///< Width of the gutter strip in pixels when hovered.
    int gutterPeekWidth = 30;         ///< Width of the gutter strip in pixels when peeking.
};

/**
 * @brief Metadata descriptor representing a user profile within the profile registry.
 * @details Stores lightweight identity information (UUID, user-facing display name, and UI accent color)
 *          used by the profile picker UI without requiring loading the full profile configuration or database.
 * @note Profile IDs are generated as UUID v4 strings without braces. Color serialization uses standard hex strings (e.g. #RRGGBB).
 */
struct ProfileInfo {
    QString id;           ///< Unique identifier for the profile (UUID string without braces).
    QString displayName;  ///< Human-readable name displayed in profile picker and UI banners.
    QColor accentColor;   ///< Visual theme accent color associated with this profile.
};

/**
 * @brief Centralized configuration repository and profile registry manager.
 * @details Implements the Repository and Singleton design patterns to isolate persistence of profile metadata,
 *          profile directories, and layout settings from UI presentation and domain logic.
 *          Manages the directory structure under ~/.config/gutterTab (or platform standard QStandardPaths::AppDataLocation):
 *          - Profiles registry: `<AppDataLocation>/profiles.json`
 *          - Profile directories: `<AppDataLocation>/profiles/<profileId>/`
 *            - `config.json`: Gutter geometry and edge settings
 *            - `notes.db`: Profile-specific SQLite database
 *            - `attachments/`: Profile-specific media and file attachments
 *          Also manages auto-launch profile mechanics, allowing one profile to be automatically booted without
 *          showing the profile picker dialog.
 * @note Thread safety: This class is not thread-safe and must be invoked exclusively from the main Qt event loop thread.
 *       Uses Meyers' Singleton pattern for lazy initialization and safe destruction. File I/O operations are synchronous
 *       using Qt's QFile and QJsonDocument.
 */
class ConfigManager {
public:
    /**
     * @brief Retrieves the singleton instance of ConfigManager.
     * @return Reference to the static ConfigManager instance.
     * @note Thread-safe static initialization in C++11 and later (Meyers' Singleton).
     */
    static ConfigManager& instance();
    
    // Profile Registry Methods

    /**
     * @brief Reads and parses all registered user profiles from profiles.json.
     * @return A QVector<ProfileInfo> containing metadata for all configured profiles; returns empty vector if registry does not exist or fails to parse.
     * @note Handles missing registry file or malformed JSON gracefully by returning an empty list.
     */
    static QVector<ProfileInfo> listProfiles();

    /**
     * @brief Creates a new user profile with a unique UUID, default configuration, and saves it to the registry.
     * @param displayName User-visible name for the new profile.
     * @param accentColor Visual theme accent color.
     * @return True if profile creation and default config save succeeded.
     * @note Automatically generates a brace-less UUIDv4, appends to profiles.json, and writes initial default config.json.
     */
    static bool createProfile(const QString& displayName, const QColor& accentColor);

    /**
     * @brief Deletes a profile from the registry and removes its entire directory hierarchy.
     * @param profileId The unique UUID identifier of the profile to remove.
     * @return True upon completion.
     * @note If the deleted profile was configured as the auto-launch profile, the auto-launch setting is cleared.
     *       Deletes the profile folder (<AppDataLocation>/profiles/<profileId>) recursively via QDir::removeRecursively().
     */
    static bool deleteProfile(const QString& profileId);

    /**
     * @brief Renames an existing profile in the registry.
     * @param profileId UUID of the target profile.
     * @param newDisplayName New human-readable display name.
     * @return True if the profile was found and updated, false if profileId was not found.
     */
    static bool renameProfile(const QString& profileId, const QString& newDisplayName);

    /**
     * @brief Updates the accent color for an existing profile.
     * @param profileId UUID of the target profile.
     * @param newColor New accent color.
     * @return True if the profile was found and updated, false if profileId was not found.
     */
    static bool updateProfileColor(const QString& profileId, const QColor& newColor);

    /**
     * @brief Computes the absolute filesystem path to a profile's config.json.
     * @param profileId UUID of the profile.
     * @return Fully qualified path string (<AppDataLocation>/profiles/<profileId>/config.json).
     */
    static QString getProfileConfigPath(const QString& profileId);

    /**
     * @brief Computes the absolute filesystem path to a profile's notes.db SQLite database.
     * @param profileId UUID of the profile.
     * @return Fully qualified path string (<AppDataLocation>/profiles/<profileId>/notes.db).
     */
    static QString getProfileDbPath(const QString& profileId);

    /**
     * @brief Computes the absolute path to a profile's attachments directory, ensuring it exists on disk.
     * @param profileId UUID of the profile.
     * @return Fully qualified directory path string (<AppDataLocation>/profiles/<profileId>/attachments).
     * @note Creates the directory on disk via QDir::mkpath() if it does not already exist.
     */
    static QString getProfileAttachmentsPath(const QString& profileId);

    /**
     * @brief Returns the ID of the currently loaded profile.
     * @return Const reference to the active profile UUID string, or empty string if no profile is active.
     */
    const QString& activeProfileId() const { return m_activeProfileId; }

    /**
     * @brief Computes the absolute filesystem path to the global profiles.json registry file.
     * @return Fully qualified path string under QStandardPaths::AppDataLocation.
     */
    static QString getProfilesRegistryPath();

    /**
     * @brief Reads the auto-launch profile ID from the profiles registry.
     * @return Profile UUID string if set, or empty string if no auto-launch profile is configured or registry is unreadable.
     */
    static QString getAutoLaunchProfile();

    /**
     * @brief Sets or clears the designated auto-launch profile in profiles.json.
     * @param profileId Profile UUID to auto-launch, or empty string to disable auto-launch.
     */
    static void setAutoLaunchProfile(const QString& profileId);

    // Profile-specific Config

    /**
     * @brief Loads profile-specific layout and geometry settings from config.json.
     * @param profileId UUID of the profile whose config should be loaded.
     * @return True if the config file was found and parsed; false if missing or unreadable (default config is saved to disk as fallback).
     * @note Edge setting defaults to Right if omitted or invalid. Fallback automatically generates and writes default settings to disk.
     */
    bool load(const QString& profileId);

    /**
     * @brief Serializes the current active configuration to the profile's config.json.
     * @param profileId UUID of the profile to write.
     * @return True if write succeeded, false if directory creation or file opening failed.
     * @note Ensures parent directories exist via QDir::mkpath() prior to opening file for writing.
     */
    bool save(const QString& profileId);
    
    /**
     * @brief Gets read-only reference to current in-memory geometry and edge configuration.
     * @return Const reference to Config struct.
     */
    const Config& config() const { return m_config; }

    /**
     * @brief Gets mutable reference to current in-memory geometry and edge configuration.
     * @return Mutable reference to Config struct.
     */
    Config& mutableConfig() { return m_config; }

private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    Config m_config;
    QString m_activeProfileId;
};

} // namespace infrastructure
