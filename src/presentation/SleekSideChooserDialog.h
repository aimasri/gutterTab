/**
 * @file SleekSideChooserDialog.h
 * @brief Modal dialog prompting for screen docking edge selection (Left vs Right).
 */

#pragma once

#include <QDialog>
#include <QString>
#include "../infrastructure/ConfigManager.h"

/**
 * @brief Frameless dark-mode modal dialog prompting the user to choose a screen dock edge.
 * 
 * @details Used during profile creation in `ProfilePickerWindow::onAddProfileClicked` to select
 *          whether the gutter tabs for the profile should be anchored to the left or right edge
 *          of the display (`infrastructure::Config::Edge`). Presents a compact 300x150 dark card
 *          with interactive "Left" and "Right" action buttons.
 * 
 * @note Must be instantiated and executed on the Qt main GUI thread. Default selection is `Edge::Right`.
 */
class SleekSideChooserDialog : public QDialog {
    Q_OBJECT
public:
    /**
     * @brief Constructs a SleekSideChooserDialog.
     * 
     * @param parent Optional parent widget in the Qt object hierarchy. Defaults to nullptr.
     */
    explicit SleekSideChooserDialog(QWidget* parent = nullptr);

    /**
     * @brief Returns the screen edge chosen by the user.
     * @return The selected `infrastructure::Config::Edge` enumeration value.
     */
    [[nodiscard]] infrastructure::Config::Edge selectedEdge() const { return m_selectedEdge; }

protected:
    /**
     * @brief Handles Escape key press to cancel and reject the dialog.
     * @param event Key event details.
     */
    void keyPressEvent(QKeyEvent* event) override;

private:
    infrastructure::Config::Edge m_selectedEdge; ///< Stored chosen screen edge (Left or Right).
};

