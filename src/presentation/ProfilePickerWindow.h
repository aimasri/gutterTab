/**
 * @file ProfilePickerWindow.h
 * @brief Profile selection, creation, and management interface window.
 */

#pragma once

#include <QWidget>
#include <QString>
#include <QVector>

class QCheckBox;
class QVBoxLayout;
class QGridLayout;
namespace infrastructure { struct ProfileInfo; }

/**
 * @brief Frameless dialog window displaying a grid of user profiles for selection and configuration.
 * 
 * @details ProfilePickerWindow serves as the startup gate or profile switcher interface in gutterTab.
 *          Because it uses `Qt::FramelessWindowHint` for a polished modern dark aesthetic, it implements
 *          manual mouse-drag movement across the desktop. Profiles are retrieved dynamically from
 *          `infrastructure::ConfigManager::listProfiles()` and presented as visual cards in a grid layout.
 *          Each profile card features a dynamically generated circular avatar with the profile's accent
 *          color and initials. Users can select an active profile, mark it to auto-launch on startup,
 *          create new profiles via a multi-step guided dialog flow, or right-click existing cards to
 *          rename, recolor, or delete profiles.
 * 
 * @note Must be executed on the Qt main GUI thread. When invoked before the main application event
 *       loop in `main.cpp`, it runs its own local event loop cycle or responds via `profileSelected`
 *       and `closedWithoutSelection` signals.
 */
class ProfilePickerWindow : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Constructs the ProfilePickerWindow widget.
     * 
     * @param parent Optional parent widget in the Qt object hierarchy. Defaults to nullptr.
     * 
     * @details Sets frameless and stay-on-top window flags, enables translucent backgrounds,
     *          fixes dimensions to 700x500, constructs the dark-mode layout, and populates
     *          the profile card grid.
     */
    explicit ProfilePickerWindow(QWidget* parent = nullptr);

signals:
    /**
     * @brief Emitted when the user selects a profile to launch.
     * @param profileId Unique directory identifier of the selected profile.
     */
    void profileSelected(const QString& profileId);

    /**
     * @brief Emitted when the window is closed without making a profile selection.
     */
    void closedWithoutSelection();

protected:
    /**
     * @brief Intercepts keyboard events to allow closing via the Escape key.
     * @param event The key event.
     */
    void keyPressEvent(QKeyEvent* event) override;

    /**
     * @brief Handles window close requests to signal cancellation if no profile was picked.
     * @param event The close event.
     */
    void closeEvent(QCloseEvent* event) override;

    /**
     * @brief Captures mouse press position to initialize frameless window dragging.
     * @param event Mouse event details.
     */
    void mousePressEvent(QMouseEvent* event) override;

    /**
     * @brief Moves the frameless window relative to mouse drag delta.
     * @param event Mouse move event details.
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    /**
     * @brief Centers the window on the active monitor when presented.
     * @param event Show event details.
     */
    void showEvent(QShowEvent* event) override;

private slots:
    /**
     * @brief Slot invoked when a profile card is left-clicked.
     * @param profileId Unique identifier of the clicked profile.
     * 
     * @details Marks selection state, updates startup auto-launch configuration if the
     *          checkbox is enabled, and emits `profileSelected`.
     */
    void onProfileCardClicked(const QString& profileId);

    /**
     * @brief Slot invoked when "+ Add Profile" button is clicked.
     * 
     * @details Launches a sequential guided creation flow:
     *          1. `SleekInputDialog` for profile name.
     *          2. `SleekColorDialog` for profile accent color.
     *          3. `SleekSideChooserDialog` for docking screen edge (Left/Right).
     *          Persists the new profile and rebuilds the card grid.
     */
    void onAddProfileClicked();

    /**
     * @brief Slot invoked from card context menu to delete a profile.
     * @param profileId Identifier of the profile to remove.
     * 
     * @details Prompts confirmation via QMessageBox, deletes profile directory via
     *          `ConfigManager::deleteProfile`, and refreshes the card grid.
     * 
     * @note Deletion is disabled in the context menu if only one profile exists.
     */
    void onDeleteProfile(const QString& profileId);

    /**
     * @brief Slot invoked from card context menu to rename a profile.
     * @param profileId Identifier of the profile to rename.
     * 
     * @details Opens `SleekInputDialog` pre-filled with current display name, updates
     *          name via `ConfigManager::renameProfile`, and refreshes the card grid.
     */
    void onEditProfileName(const QString& profileId);

    /**
     * @brief Slot invoked from card context menu to modify a profile's accent color.
     * @param profileId Identifier of the profile to recolor.
     * 
     * @details Opens `SleekColorDialog` pre-filled with current accent color, updates
     *          color via `ConfigManager::updateProfileColor`, and refreshes the card grid.
     */
    void onEditProfileColor(const QString& profileId);

private:
    /**
     * @brief Scaffolds the outer frameless dark-mode shell, headers, close button, and footer controls.
     */
    void buildUI();

    /**
     * @brief Clears existing profile card widgets and repopulates the 3-column grid from ConfigManager.
     */
    void rebuildProfileCards();

    /**
     * @brief Instantiates and connects an individual ProfileCard widget for the given profile.
     * @param profile Struct containing profile ID, display name, and accent color.
     * @return Newly created QWidget (ProfileCard) ready for grid insertion.
     */
    QWidget* createProfileCard(const infrastructure::ProfileInfo& profile);

    /**
     * @brief Applies application theme styling to child components (reserved for theming extensions).
     */
    void applyTheme();

    QWidget* m_cardsContainer = nullptr;  ///< Container widget hosting the cards grid.
    QGridLayout* m_cardsLayout = nullptr;  ///< Grid layout managing card placement (up to 3 columns).
    QCheckBox* m_autoLaunchCheck = nullptr; ///< Checkbox for remembering profile on startup.
    bool m_selected = false;               ///< Flag indicating if a profile was successfully chosen.
    QPoint m_dragPosition;                 ///< Stored mouse offset used for frameless window dragging.
};

