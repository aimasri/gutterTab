/**
 * @file SleekDialogs.h
 * @brief Custom dark-themed input dialogs, color pickers, and context menus.
 */

#pragma once

#include <QColor>
#include <QDialog>
#include <QMenu>
#include <QString>
#include <QVector>

class QLineEdit;
class QSlider;
class QLabel;
class QPushButton;

/**
 * @brief Retrieves the curated dark palette used for tab and profile color selections.
 * 
 * @return Immutable reference to a QVector of hex color strings.
 * 
 * @details Returns a static palette of 24 carefully chosen hex colors arranged in a 4-row by
 *          6-column gradient matrix (Blues, Greens, Reds, Oranges/Ambers), ranging from deep
 *          shadow tones to vibrant highlight accents optimized for dark UI contrast.
 * 
 * @note Thread-safe static initialization; should be called on the GUI thread when generating swatches.
 */
[[nodiscard]] const QVector<QString>& getSleekDarkPalette();

/**
 * @brief Dark-themed, frameless context menu with customized styling.
 * 
 * @details SleekContextMenu subclasses `QMenu` to render a polished popup menu with translucent
 *          backgrounds, rounded corners, subtle dark borders, and custom padding on actions.
 *          It eliminates default OS menu shadows to ensure seamless blending with gutterTab's
 *          minimalist interface.
 * 
 * @note Operates on Qt's main GUI thread. Dismisses automatically when clicking outside or
 *       pressing Escape, per standard `QMenu` popup semantics.
 */
class SleekContextMenu : public QMenu {
    Q_OBJECT
public:
    /**
     * @brief Constructs a SleekContextMenu.
     * @param parent Optional parent widget in the Qt object hierarchy. Defaults to nullptr.
     */
    explicit SleekContextMenu(QWidget* parent = nullptr);
};

/**
 * @brief Frameless dark-mode modal dialog for single-line text inputs (e.g. note/profile renaming).
 * 
 * @details Provides a centered input card with a title, descriptive sub-label, stylized text input
 *          field, and Cancel / Save action buttons. It supports immediate keyboard workflow:
 *          pressing Enter or Return accepts and saves, while pressing Escape rejects and cancels.
 * 
 * @note Modal execution via `exec()` blocks caller flow until dismissed.
 */
class SleekInputDialog : public QDialog {
    Q_OBJECT
public:
    /**
     * @brief Constructs a SleekInputDialog.
     * 
     * @param title Header title displayed at the top of the card.
     * @param labelText Optional descriptive sub-label displayed above the text input.
     * @param initialValue Initial text pre-populating the text field (auto-selected for quick overwriting).
     * @param parent Optional parent widget in the Qt object hierarchy. Defaults to nullptr.
     */
    explicit SleekInputDialog(
        const QString& title,
        const QString& labelText,
        const QString& initialValue = QString(),
        QWidget* parent = nullptr
    );

    /**
     * @brief Returns the trimmed text string currently entered in the line edit.
     * @return Trimmed QString value, or empty QString if line edit is uninitialized.
     */
    [[nodiscard]] QString value() const;

protected:
    /**
     * @brief Intercepts key presses to provide intuitive Return/Enter acceptance and Escape dismissal.
     * @param event Key event details.
     */
    void keyPressEvent(QKeyEvent* event) override;

private:
    QLineEdit* m_lineEdit = nullptr; ///< Owned single-line input field.
};

/**
 * @brief Frameless dark-mode modal dialog for selecting colors via preset swatches or hex input.
 * 
 * @details SleekColorDialog presents a grid of 24 curated swatches (from `getSleekDarkPalette()`),
 *          a regex-validated hexadecimal text entry field, a dynamic live preview swatch bar, and
 *          Cancel / Apply action buttons. Clicking any preset swatch immediately syncs the hex field
 *          and preview; manually typing a valid 6-character hex code dynamically updates the preview.
 * 
 * @note Operates modally on the GUI thread. If an invalid initial color is provided, it falls back
 *       gracefully to `#2563eb`.
 */
class SleekColorDialog : public QDialog {
    Q_OBJECT
public:
    /**
     * @brief Constructs a SleekColorDialog.
     * 
     * @param initialColor Pre-selected color. If invalid, defaults to `#2563eb`.
     * @param parent Optional parent widget in the Qt object hierarchy. Defaults to nullptr.
     */
    explicit SleekColorDialog(const QColor& initialColor, QWidget* parent = nullptr);

    /**
     * @brief Returns the final selected QColor upon dialog acceptance.
     * @return The chosen QColor instance.
     */
    [[nodiscard]] QColor selectedColor() const;

private slots:
    /**
     * @brief Slot invoked when a palette swatch push button is clicked.
     * @param color The QColor corresponding to the clicked swatch button.
     */
    void onSwatchClicked(const QColor& color);

    /**
     * @brief Slot invoked when text is typed or edited in the hex line edit.
     * @param text The raw text string from the line edit.
     */
    void onHexEdited(const QString& text);

    /**
     * @brief Updates the background color of the live preview widget to match m_color.
     */
    void updatePreview();

private:
    QColor m_color;                     ///< The currently selected color state.
    QLineEdit* m_hexEdit = nullptr;      ///< Owned hex input line edit with regex validation.
    QWidget* m_previewSwatch = nullptr;  ///< Owned widget rendering the live color preview bar.
};

