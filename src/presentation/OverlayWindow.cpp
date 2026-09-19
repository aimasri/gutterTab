#include <QTimer>
#include "OverlayWindow.h"
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <QDebug>

#include "GutterStrip.h"
#include "NoteEditorOverlay.h"
#include "AppIcon.h"
#include "SleekDialogs.h"
#include "../infrastructure/ConfigManager.h"

namespace presentation {

/**
 * @brief Constructs the OverlayWindow presentation root.
 *
 * @details Configures top-level window flags, transparency attributes, initializes child widgets
 * (GutterStrip, NoteEditorOverlay), wires domain state synchronization, and initiates hotkey polling.
 *
 * @param controller Non-null pointer to the domain TabController.
 * @param parent Optional parent widget (nullptr for top-level desktop surfaces).
 */
OverlayWindow::OverlayWindow(domain::TabController* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller) {
    
    // Step 1: Configure X11 window manager flags.
    // - Qt::SplashScreen prevents window manager taskbar and pager registration (crucial in Openbox).
    // - Qt::WindowStaysOnTopHint ensures overlay visibility above normal application windows.
    // - Qt::FramelessWindowHint removes title bars and window decorations.
    setWindowFlags(Qt::SplashScreen | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
                   
    // Step 2: Configure transparent visual attributes for X11 ARGB compositing.
    // - Qt::WA_TranslucentBackground requests an ARGB 32-bit visual instead of the default 24-bit visual.
    // - Qt::WA_NoSystemBackground prevents the window manager from filling the window backing pixmap with an opaque brush.
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    
    // Step 3: Instantiate child components within the presentation layer.
    m_gutterStrip = new GutterStrip(controller, this);
    connect(m_gutterStrip, &GutterStrip::triggerAreaChanged, this, [this]() { updateGutterGeometry(); });
    m_dashboardOverlay = new BentoDashboard(controller, this);
    m_editorOverlay = new NoteEditorOverlay(controller, nullptr);
    m_foldersOverlay = new FoldersOverlay(controller, nullptr);
    
    // Step 7: Wire domain state machine emissions to overlay geometry re-evaluation
    connect(m_controller, &domain::TabController::stateChanged,
            this, &OverlayWindow::onStateChanged);
            
    // Step 8: Setup system tray icon
    setupTrayIcon();
            
    // Step 5: Start a periodic 50ms timer to poll modifier chords (Ctrl+Shift).
    m_hotkeyTimer = new QTimer(this);
    connect(m_hotkeyTimer, &QTimer::timeout, this, &OverlayWindow::checkHotkey);
    m_hotkeyTimer->start(50);

    // Step 6: Initialize root geometry against primary screen boundaries.
    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        setGeometry(screen->geometry());
    }
    
    // Step 7: Apply edge layout and input constraints.
    updateGutterGeometry();
}

/**
 * @brief Destructor for OverlayWindow.
 */
OverlayWindow::~OverlayWindow() {
}

/**
 * @brief Updates the X11 input bounding mask for desktop click-through.
 *
 * @details Traditional X11 transparent overlays achieve click-through via the XShape extension:
 * `XShapeCombineRectangles(..., ShapeInput, ...)` or `xcb_shape_rectangles(conn, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_INPUT, ...)`.
 * In GutterTab, instead of incurring expensive rasterized 1-bit bitmask generation and X11 round-trips,
 * click-through is achieved architecturally by dynamically sizing the X11 window geometry to tightly
 * bound only the strip during IDLE and PEEKING states. `clearMask()` clears any active input mask,
 * letting the window's physical geometry dictate event routing.
 */
void OverlayWindow::updateInputMask() {
    clearMask(); // No masks! We use actual geometry now.
}

/**
 * @brief Paints the overlay window surface and background dimming.
 *
 * @details Handles compositing and background clearing:
 * - If state is OPEN: renders a 150/255 alpha black veil (`QColor(0, 0, 0, 150)`) across the screen.
 * - If state is IDLE/PEEKING: executes `CompositionMode_Clear` to clear destination alpha to zero,
 *   preventing residual artifacts on composited X11 desktops.
 *
 * @param event The QPaintEvent parameters (unused, full repaint executed).
 */
void OverlayWindow::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    
    qDebug() << "PAINT EVENT! State:" << (int)m_controller->currentState() << "Rect:" << rect() << "Mask empty:" << mask().isEmpty();
    
    if (m_controller->currentState() == domain::TabController::State::OPEN ||
        m_controller->currentState() == domain::TabController::State::FOLDERS) {
        // Step 1: In OPEN/FOLDERS state, render semi-transparent backdrop for focus dimming.
        painter.fillRect(rect(), QColor(0, 0, 0, 150));
    } else if (m_controller->currentState() == domain::TabController::State::DASHBOARD) {
        // Dashboard has its own background dimming in BentoDashboard widget
        // But we shouldn't clear alpha here.
    } else {
        // Step 2: In IDLE/PEEKING state, clear alpha channel to maintain transparency.
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(rect(), Qt::transparent);
    }
}

/**
 * @brief Handles window show events to trigger geometry re-alignment.
 *
 * @param event The QShowEvent parameters passed to base widget handler.
 */
void OverlayWindow::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    updateGutterGeometry();
}

/**
 * @brief Recalculates and applies window and strip geometry based on configuration and controller state.
 *
 * @details Translates screen coordinates to local layout coordinates:
 * - Edge case 1: Hotkey active -> Collapses window to (0, 0, 0, 0) to yield all pointer/keyboard events.
 * - Edge case 2: Null screen pointer -> Safely aborts if primary screen is disconnected or unavailable.
 * - State OPEN: Fullscreen geometry covers the primary monitor (`screenRect`), strip sits at edge (0 or width-w).
 * - State IDLE / PEEKING: Window geometry shrinks to strip dimensions on configured edge, ensuring underlying
 *   desktop windows receive clicks everywhere else without XShape mask overhead.
 */
void OverlayWindow::updateGutterGeometry() {
    // Step 1: If hotkey chord is held down, hide the window surface entirely.
    if (m_isHotkeyDown) {
        setGeometry(0, 0, 0, 0);
        return;
    }

    // Step 2: Fetch configuration and screen parameters.
    auto& config = infrastructure::ConfigManager::instance().config();
    int w = m_gutterStrip->width();
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    QRect screenRect = screen->geometry();

    // Step 3: Compute coordinate spaces based on active state.
    if (m_controller->currentState() == domain::TabController::State::OPEN || 
        m_controller->currentState() == domain::TabController::State::DASHBOARD ||
        m_controller->currentState() == domain::TabController::State::FOLDERS) {
        // OPEN/DASHBOARD/FOLDERS state: Span full primary screen for backdrop dimming and modal click interception.
        setGeometry(screenRect);
        if (config.edge == infrastructure::Config::Edge::Left) {
            m_gutterStrip->setGeometry(0, 0, w, height());
        } else {
            m_gutterStrip->setGeometry(width() - w, 0, w, height());
        }
        if (m_dashboardOverlay) {
            m_dashboardOverlay->setGeometry(0, 0, width(), height());
        }
    } else {
        // IDLE/PEEKING state: Fit window geometry strictly around the gutter strip.
        // The remainder of the desktop is completely uncovered, providing native click-through.
        if (config.edge == infrastructure::Config::Edge::Left) {
            setGeometry(screenRect.x(), screenRect.y(), w, screenRect.height());
            m_gutterStrip->setGeometry(0, 0, w, height());
        } else {
            setGeometry(screenRect.x() + screenRect.width() - w, screenRect.y(), w, screenRect.height());
            m_gutterStrip->setGeometry(0, 0, w, height());
        }
    }
}

/**
 * @brief Handles window resize events.
 *
 * @param event The QResizeEvent containing dimension changes.
 */
void OverlayWindow::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateGutterGeometry();
}

/**
 * @brief Handles mouse press events on the overlay backdrop.
 *
 * @details When in OPEN state, any mouse press falling on the dimming backdrop outside
 * child controls automatically triggers dismissal of the active note.
 *
 * @param event The QMouseEvent containing click coordinates and button flags.
 */
void OverlayWindow::mousePressEvent(QMouseEvent* event) {
    if (m_controller->currentState() == domain::TabController::State::OPEN) {
        // If clicking the dim background, close the note
        m_controller->closeNote();
    } else if (m_controller->currentState() == domain::TabController::State::DASHBOARD) {
        m_controller->toggleDashboard();
    } else if (m_controller->currentState() == domain::TabController::State::FOLDERS) {
        m_controller->toggleFolders();
    }
    QWidget::mousePressEvent(event);
}

/**
 * @brief Slot responding to domain state changes from TabController.
 *
 * @details Schedules an asynchronous geometry recalculation via a single-shot timer
 * (50ms) to prevent re-entrant layout storms during rapid transitions, and flags
 * the window for repainting.
 *
 * @param state The newly transitioned domain::TabController::State.
 */
void OverlayWindow::onStateChanged(domain::TabController::State state) {
    if (state == domain::TabController::State::DASHBOARD) {
        QRect origin = m_gutterStrip->geometry();
        origin.setHeight(50);
        m_dashboardOverlay->showDashboard(origin);
    } else {
        if (m_dashboardOverlay && m_dashboardOverlay->isVisible()) {
            m_dashboardOverlay->hideDashboard();
        }
    }

    // Defer geometry calculation to avoid re-entrancy during state updates
    QTimer::singleShot(50, this, [this]() {
        updateGutterGeometry();
    });
    update(); // Trigger repaint (for background dimming)
}

/**
 * @brief Polls the keyboard modifier state to toggle temporary concealment.
 *
 * @details Queries `QApplication::queryKeyboardModifiers()`. When Ctrl+Shift is detected,
 * the window temporarily collapses geometry to (0, 0, 0, 0) and conceals the active note editor,
 * allowing instant desktop access. Upon release, normal geometry and editor visibility are restored.
 * 
 * @note This polling design avoids X11 passive grab conflicts (`XGrabKey`) with Openbox keybindings.
 */
void OverlayWindow::checkHotkey() {
    Qt::KeyboardModifiers mods = QApplication::queryKeyboardModifiers();
    bool isDown = ((mods & Qt::ControlModifier) && (mods & Qt::ShiftModifier));
    
    if (isDown != m_isHotkeyDown) {
        m_isHotkeyDown = isDown;
        updateGutterGeometry();
        
        if (m_controller->currentState() == domain::TabController::State::OPEN && m_editorOverlay) {
            if (isDown) {
                m_editorOverlay->hide();
            } else {
                m_editorOverlay->show();
            }
        }
        
        if (m_controller->currentState() == domain::TabController::State::FOLDERS && m_foldersOverlay) {
            if (isDown) {
                m_foldersOverlay->hide();
            } else {
                m_foldersOverlay->show();
            }
        }
    }
}
void OverlayWindow::setupTrayIcon() {
    m_trayIcon = new QSystemTrayIcon(AppIcon::createAppIcon(), this);
    m_trayIcon->setToolTip("gutterTab Daemon");
    
    auto* menu = new SleekContextMenu();
    
    auto* showAction = menu->addAction("Toggle Dashboard");
    connect(showAction, &QAction::triggered, [this]() {
        m_controller->toggleDashboard();
    });
    
    auto* quitAction = menu->addAction("Quit gutterTab");
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    
    m_trayIcon->setContextMenu(menu);
    m_trayIcon->show();
}

} // namespace presentation
