#pragma once

#include <QWidget>
#include <QRegion>
#include <QSystemTrayIcon>
#include "../domain/TabController.h"
#include "BentoDashboard.h"

namespace presentation {

class GutterStrip;
class NoteEditorOverlay;

/**
 * @brief Top-level transparent overlay window managing screen edge tabs and desktop pass-through.
 *
 * @details OverlayWindow provides the presentation anchor for the gutterTab application on X11 desktops.
 * In X11 window management, transparent overlays require a careful balance between rendering fidelity
 * and pointer event routing (desktop click-through). Historically, full-screen transparent windows in X11
 * rely on the XShape extension (specifically `xcb_shape_rectangles` or `XShapeCombineRectangles` with
 * `ShapeInput`) to punch input holes into the 1-bit input mask, routing clicks to underlying client windows.
 * 
 * To maximize performance and eliminate X11 server round-trip latency and compositor race conditions,
 * OverlayWindow adopts a dynamic geometry allocation pattern:
 * - In the IDLE or PEEKING state, the window's physical X11 geometry is tightly clamped to the active gutter
 *   strip on the screen edge (left or right), allowing 100% native click-through on the rest of the desktop
 *   without requiring costly bitmask rasterization.
 * - In the OPEN state, OverlayWindow expands to the full primary screen geometry to render the semi-transparent
 *   dimming backdrop (`QColor(0, 0, 0, 150)`) and intercept background mouse clicks to dismiss the active note.
 *
 * It hosts child components including the GutterStrip (vertical tab stack) and NoteEditorOverlay (markdown editor),
 * mediating coordinates between the screen boundary and Qt's local coordinate space.
 *
 * @note This class must only be instantiated and accessed on the main GUI thread. It relies on Qt's object tree
 * ownership model. Thread safety is not guaranteed across threads.
 */
class OverlayWindow : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Constructs the OverlayWindow presentation root.
     *
     * @details Sets up frameless splash window flags (`Qt::SplashScreen | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint`),
     * enables translucent and unbuffered system backgrounds (`Qt::WA_TranslucentBackground`, `Qt::WA_NoSystemBackground`),
     * instantiates child presentation widgets (GutterStrip, NoteEditorOverlay), and initializes the hotkey polling timer.
     *
     * @param controller Pointer to the domain TabController governing application state. Must not be null.
     * @param parent Optional parent widget (typically nullptr for top-level desktop windows).
     */
    explicit OverlayWindow(domain::TabController* controller, QWidget* parent = nullptr);

    /**
     * @brief Destructor for OverlayWindow.
     *
     * @details Cleans up allocated resources. Child QObject instances parented to this window are automatically
     * reclaimed via Qt's object hierarchy.
     */
    ~OverlayWindow() override;

    /**
     * @brief Updates the X11 input bounding mask for desktop click-through.
     *
     * @details Synchronizes the input shape mask using `QWidget::clearMask()` or `QWidget::setMask()`. Under X11,
     * this corresponds to applying an XShape 1-bit input mask via `xcb_shape_rectangles` or `XShapeCombineRectangles`
     * (ShapeInput). When using dynamic geometry bounding, `clearMask()` ensures the active strip geometry remains
     * fully interactive.
     *
     * @note When dynamic geometry sizing is active, the window geometry itself defines the interactive surface,
     * minimizing compositor overhead.
     */
    void updateInputMask();

    /**
     * @brief Periodic polling handler to inspect global modifier hotkey states.
     *
     * @details Polls `QApplication::queryKeyboardModifiers()` at 50ms intervals to detect Ctrl+Shift chords.
     * If the hotkey chord is detected, the overlay temporarily shrinks its geometry to (0, 0, 0, 0) and hides
     * the editor overlay, releasing desktop interaction to underlying windows without requiring low-level
     * X11 passive grab conflicts (`XGrabKey`).
     *
     * @note Edge case: Avoids X11 key grab contention with window managers (e.g., Openbox/tint2) by using polling.
     */
    void checkHotkey();

    /**
     * @brief Recalculates and applies window and strip geometry based on configuration and controller state.
     *
     * @details Maps coordinates between global screen coordinates (`QScreen::geometry()`) and local overlay
     * boundaries. Depending on `domain::TabController::State` (IDLE/PEEKING vs. OPEN) and the configured edge
     * (`Config::Edge::Left` vs. `Config::Edge::Right`), adjusts the window's `setGeometry` and positions
     * `GutterStrip` flush against the screen margin.
     *
     * @note Handles multi-monitor scenarios by querying `QGuiApplication::primaryScreen()`. If the screen is null,
     * the update safely aborts to prevent crashes.
     */
    void updateGutterGeometry();

protected:
    /**
     * @brief Paints the overlay window surface and background dimming.
     *
     * @details Invoked during Qt repaint cycles. If the controller state is `OPEN`, fills the canvas with a
     * semi-transparent black brush (`QColor(0, 0, 0, 150)`) for focus backdrop dimming. In non-OPEN states,
     * uses `QPainter::CompositionMode_Clear` to preserve absolute transparency over the desktop.
     *
     * @param event The paint event parameters supplied by the Qt paint engine.
     */
    void paintEvent(QPaintEvent* event) override;

    /**
     * @brief Handles window show events to trigger geometry re-alignment.
     *
     * @details Re-synchronizes strip geometry against the primary screen as soon as the X11 surface is mapped.
     *
     * @param event The QShowEvent parameters.
     */
    void showEvent(QShowEvent* event) override;

    /**
     * @brief Handles window resize events to ensure correct child layout positioning.
     *
     * @details Re-evaluates child strip geometries whenever the overlay dimensions are altered by the window manager.
     *
     * @param event The QResizeEvent containing old and new dimensions.
     */
    void resizeEvent(QResizeEvent* event) override;

    /**
     * @brief Handles mouse press events on the overlay surface.
     *
     * @details When the overlay is in the `OPEN` state, a click outside of the editor or gutter strip strikes
     * this backdrop, automatically triggering `m_controller->closeNote()` to dismiss the note editor.
     *
     * @param event The QMouseEvent containing click coordinates and button states.
     */
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    /**
     * @brief Responds to domain state changes emitted by TabController.
     *
     * @details Triggers an asynchronous geometry re-evaluation via `QTimer::singleShot(50, ...)` to prevent
     * re-entrancy and layout thrashing, and requests a repaint with `update()` to redraw or clear the dimming mask.
     *
     * @param state The newly transitioned domain::TabController::State.
     */
    void onStateChanged(domain::TabController::State state);
    void setupTrayIcon();

private:
    domain::TabController* m_controller; ///< Non-owning pointer to the domain TabController.
    
    GutterStrip* m_gutterStrip;          ///< Child widget rendering the stack of note tabs.
    NoteEditorOverlay* m_editorOverlay;
    BentoDashboard* m_dashboardOverlay;  ///< Child overlay rendering the active note markdown editor.
    QTimer* m_hotkeyTimer;               ///< 50ms polling timer for keyboard modifiers.
    bool m_isHotkeyDown = false;         ///< Cached hotkey modifier status (Ctrl+Shift).
    QSystemTrayIcon* m_trayIcon;         ///< System tray icon for gutterTab.
    
    // Defines the area that can receive mouse clicks
    QRegion m_solidRegion;               ///< Cached region mask for XShape input bounding.
};

} // namespace presentation
