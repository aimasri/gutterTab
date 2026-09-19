#pragma once

#include <QWidget>
#include <QPropertyAnimation>
#include "../domain/Note.h"

namespace presentation {

/**
 * @brief Interactive edge tab widget representing an individual note in the gutter strip.
 *
 * @details GutterTab implements a physical notebook-style index tab anchored to the screen border.
 * Key architectural features include:
 * - **QPropertyAnimation Width Interpolation:** Exposes the `currentWidth` Qt property, allowing smooth width
 *   transitions via `QPropertyAnimation` with `QEasingCurve::OutCubic` between rest (2px), hover (26px),
 *   and peek (30px+) widths. Preempts ongoing animations on state changes to guarantee fluid responsiveness.
 * - **Custom Paint Coordinate Transformations:** During `paintEvent`, translates the coordinate origin to the
 *   center of the widget (`width()/2, height()/2`) and rotates the coordinate space -90 degrees counter-clockwise.
 *   This renders the note title vertically upwards along the screen edge without requiring specialized vertical font assets.
 * - **Font Metric Measurements & Snippet Rendering:** Measures title text using `QFontMetrics::horizontalAdvance`
 *   and dynamically lays out an excerpt preview snippet (up to 3 non-empty lines) with word-wrapping when the tab
 *   is in peek expansion mode.
 * - **Drag-and-Drop & Context Menu Triggers:** Initiates dragging upon exceeding `QApplication::startDragDistance()`
 *   with live pixmap drag feedback, and exposes a rich context menu (`SleekContextMenu`) emitting fine-grained signals
 *   for note creation, renaming, color picking, and deletion.
 *
 * @note This widget must be manipulated exclusively on the main Qt GUI thread. Animation state and QPainter transformations
 * are tightly coupled to the active Qt rendering context.
 */
class GutterTab : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int currentWidth READ currentWidth WRITE setCurrentWidth)

public:
    /**
     * @brief Constructs a GutterTab widget for a specific note.
     *
     * @param note Domain model containing note metadata (id, title, color, content).
     * @param parent Optional parent widget (typically GutterStrip).
     */
    explicit GutterTab(const domain::Note& note, QWidget* parent = nullptr);

    /**
     * @brief Destructor for GutterTab.
     */
    ~GutterTab() override;

    /**
     * @brief Gets the current interactive animated width of the tab.
     * @return Current tab width in pixels.
     */
    int currentWidth() const { return m_currentWidth; }

    /**
     * @brief Sets the current animated width and updates the widget geometry and paint canvas.
     *
     * @details Invoked by `QPropertyAnimation` during width interpolation. If the width changed,
     * updates `setFixedWidth` and schedules a repaint via `update()`.
     *
     * @param width Target width in pixels.
     */
    void setCurrentWidth(int width);

    /**
     * @brief Returns the unique database identifier of the associated note.
     * @return Integer note ID.
     */
    int noteId() const { return m_note.id; }

    /**
     * @brief Synchronizes the tab with an updated note model and requests a repaint.
     *
     * @param note Updated Note domain model.
     */
    void updateNote(const domain::Note& note);
    
    /**
     * @brief Smoothly animates the tab's width to a target pixel value.
     *
     * @details Preempts any currently running animation on `currentWidth` to avoid conflicting
     * interpolation targets, sets the new end value, and launches the `QPropertyAnimation`.
     *
     * @param targetWidth Target width in pixels (e.g., rest, hover, or peek width).
     */
    void animateToWidth(int targetWidth);

protected:
    /**
     * @brief Paints the tab background, rotated vertical title, and preview snippet.
     *
     * @details Execution steps:
     * 1. Renders rounded rectangle background using note's assigned hex color.
     * 2. If tab width exceeds resting width, executes coordinate translation to center and rotates
     *    by -90 degrees to render the title vertically with antialiased typography.
     * 3. If tab width reaches peek threshold, renders up to 3 lines of note body text with word wrap.
     *
     * @param event The QPaintEvent parameters.
     */
    void paintEvent(QPaintEvent* event) override;

    /**
     * @brief Handles mouse enter event into the tab.
     *
     * @details Emits `hovered()` to signal parent `GutterStrip`.
     *
     * @param event The QEnterEvent parameters.
     */
    void enterEvent(QEnterEvent* event) override;

    /**
     * @brief Handles mouse leave event from the tab.
     *
     * @details Emits `unhovered()` to signal parent `GutterStrip`.
     *
     * @param event Base QEvent parameters.
     */
    void leaveEvent(QEvent* event) override;

    /**
     * @brief Handles mouse press events to detect click or prepare drag operation.
     *
     * @details On left button press, records initial click position `m_dragStartPosition`
     * and emits `clicked(m_note.id)` to trigger note opening.
     *
     * @param event The QMouseEvent parameters.
     */
    void mousePressEvent(QMouseEvent* event) override;

    /**
     * @brief Detects drag threshold and initiates QDrag reordering session.
     *
     * @details Checks if mouse move distance exceeds `QApplication::startDragDistance()`.
     * If exceeded, constructs a `QDrag` with MIME type `"application/x-guttertab-id"`, renders
     * a semi-transparent pixmap snapshot of the tab as the drag cursor visual, and executes `Qt::MoveAction`.
     *
     * @param event The QMouseEvent parameters.
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    /**
     * @brief Displays the contextual popup menu for tab operations.
     *
     * @details Emits `menuOpened()` (suspending strip collapse timers), displays `SleekContextMenu`
     * with actions (Add New Note, Rename Note, Change Color, Delete, Close), emits `menuClosed()`,
     * and fires the corresponding action signal.
     *
     * @param event The QContextMenuEvent containing trigger coordinates.
     */
    void contextMenuEvent(QContextMenuEvent* event) override;

signals:
    /** @brief Emitted when the cursor enters the tab. */
    void hovered();

    /** @brief Emitted when the cursor departs the tab. */
    void unhovered();

    /**
     * @brief Emitted when the tab is clicked to open the note editor.
     * @param noteId ID of the clicked note.
     */
    void clicked(int noteId);

    /** @brief Emitted when "Add New Note" is triggered from the context menu. */
    void addRequested();

    /**
     * @brief Emitted when "Rename Note" is triggered from the context menu.
     * @param noteId ID of the note to rename.
     */
    void renameRequested(int noteId);

    /**
     * @brief Emitted when "Change Color" is triggered from the context menu.
     * @param id ID of the note to recolor.
     */
    void changeColorRequested(int id);

    /**
     * @brief Emitted when "Delete" is triggered from the context menu.
     * @param id ID of the note to delete.
     */
    void deleteRequested(int id);

    /** @brief Emitted when "Close gutterTab" application exit is requested. */
    void quitRequested();

    /** @brief Emitted before the context menu is opened, notifying parent to inhibit collapse. */
    void menuOpened();

    /** @brief Emitted after the context menu is closed, allowing parent to resume collapse timer. */
    void menuClosed();

private:
    domain::Note m_note;                ///< Local copy of the note domain model.
    int m_currentWidth;                 ///< Current interpolated width in pixels.
    QPoint m_dragStartPosition;         ///< Coordinates where left-mouse press initiated.
    QPropertyAnimation* m_animation;    ///< Width interpolation animation driver.
    
    /**
     * @brief Extracts up to 3 non-empty lines from note content for peek snippet rendering.
     * @return Trimmed multi-line preview text.
     */
    QString getSnippet() const;
};

} // namespace presentation
