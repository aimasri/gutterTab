#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include "GutterStrip.h"
#include "../infrastructure/ConfigManager.h"
#include <QEvent>

#include <QInputDialog>
#include <QColorDialog>
#include <QGuiApplication>
#include <QScreen>
#include "../infrastructure/DatabaseManager.h"
#include "SleekDialogs.h"

namespace presentation {

/**
 * @brief Constructs the GutterStrip container widget.
 *
 * @details Initializes vertical layout without default margins/spacing, sets edge alignment,
 * configures the hover collapse timer (150ms single-shot), sets initial 8px resting width,
 * and establishes signal subscriptions with domain::TabController.
 *
 * @param controller Non-null pointer to domain TabController.
 * @param parent Optional parent widget (typically OverlayWindow).
 */
GutterStrip::GutterStrip(domain::TabController* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller) {
    
    // Step 1: Configure vertical box layout. Margins and spacing are set to 0
    // so negative spacer items can accurately control binder overlap offsets.
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0); // Spacing 0 allows explicit negative spacer insertion for binder overlap
    
    // Step 2: Align layout vertically centered and pinned against configured screen edge.
    auto& config = infrastructure::ConfigManager::instance().config();
    if (config.edge == infrastructure::Config::Edge::Right) {
        m_layout->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    } else {
        m_layout->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    }
    
    // Step 3: Establish initial resting strip width (8px narrow trigger line).
    setFixedWidth(8);
    
    // Step 4: Configure the hover collapse debounce timer.
    m_collapseTimer = new QTimer(this);
    m_collapseTimer->setSingleShot(true);
    m_collapseTimer->setInterval(150);
    connect(m_collapseTimer, &QTimer::timeout, this, &GutterStrip::collapseTimerFired);
    
    // Step 5: Subscribe to domain controller updates for note reloading and activation changes.
    connect(m_controller, &domain::TabController::notesLoaded, this, &GutterStrip::updateTabs);
    connect(m_controller, &domain::TabController::activeNoteChanged, this, &GutterStrip::onActiveNoteChanged);
}

/**
 * @brief Destructor for GutterStrip.
 */
GutterStrip::~GutterStrip() {
}

/**
 * @brief Rebuilds or updates in-place the collection of GutterTab child widgets.
 *
 * @details Execution sequence:
 * 1. Fast path: Checks if existing tab count and IDs match the incoming note list.
 *    If identical, performs an in-place model update (`m_tabs[i]->updateNote(...)`),
 *    bypassing expensive layout destruction and widget reconstruction.
 * 2. Layout tear-down: If structural changes occurred, removes all layout items,
 *    hides child widgets, schedules their deletion via `deleteLater()`, and clears `m_tabs`.
 * 3. Geometry & Dead-zone calculation:
 *    - Queries primary screen height.
 *    - Applies an 80px screen dead-zone margin at top and bottom (`availableHeight = screenHeight - 160`)
 *      to ensure tabs never obscure OS panels, desktop taskbars (tint2/polybar), or corner hotspots.
 *    - Computes tab height clamped to a maximum of 180px (`tabHeight = min(180, availableHeight / noteCount)`).
 * 4. Tab instantiation & Binder overlap spacing:
 *    - For each note index > 0, inserts a negative spacer (`QSpacerItem(0, -25, ...)`), producing
 *      the physical binder / index-tab overlapping aesthetic.
 *    - Instantiates a `GutterTab` widget, setting initial width based on current controller state.
 * 5. Signal Wiring: Connects hover, click, context menu actions, and modal open/closed notifications.
 * 6. Layout insertion & Z-order update: Adds widget to layout with edge alignment and refreshes Z-order.
 */
void GutterStrip::updateTabs() {
    const auto& notes = m_controller->notes();
    
    // Step 1: Check if in-place update is possible to preserve widget instances and avoid layout churn.
    bool canUpdateInPlace = (notes.size() == m_tabs.size());
    if (canUpdateInPlace) {
        for (int i = 0; i < notes.size(); ++i) {
            if (notes[i].id != m_tabs[i]->noteId()) {
                canUpdateInPlace = false;
                break;
            }
        }
    }
    
    if (canUpdateInPlace) {
        for (int i = 0; i < notes.size(); ++i) {
            m_tabs[i]->updateNote(notes[i]);
        }
        return;
    }

    // Step 2: Remove and schedule deletion for all existing layout items and widgets.
    QLayoutItem *item;
    while ((item = m_layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->hide();
            item->widget()->deleteLater();
        }
        delete item;
    }
    m_dashboardBtn = nullptr;
    m_tabs.clear();
    
    if (notes.isEmpty()) return;
    
    // Step 3: Compute vertical dimensions respecting the 80px top/bottom screen dead-zones.
    int screenHeight = QGuiApplication::primaryScreen()->geometry().height();
    int availableHeight = screenHeight - 160; // 80px dead-zone margin top/bottom prevents OS panel overlap
    int tabHeight = std::min(180, availableHeight / (int)notes.size());
    
    auto& config = infrastructure::ConfigManager::instance().config();
    int activeId = m_controller->activeNoteId();
    bool isOpen = (m_controller->currentState() == domain::TabController::State::OPEN);
    bool isPeeking = (m_controller->currentState() == domain::TabController::State::PEEKING);
    
    
    // Add Dashboard button at the top
    m_dashboardBtn = new DashboardButton(this);
    m_layout->addWidget(m_dashboardBtn);
    connect(m_dashboardBtn, &DashboardButton::clicked, m_controller, &domain::TabController::toggleDashboard);
    connect(m_dashboardBtn, &DashboardButton::hovered, this, [this]() {
        auto& config = infrastructure::ConfigManager::instance().config();
        m_collapseTimer->stop();
        if (m_controller->currentState() != domain::TabController::State::OPEN) {
            m_controller->setState(domain::TabController::State::PEEKING);
            m_dashboardBtn->animateToWidth(config.gutterPeekWidth);
            for (auto tab : m_tabs) {
                tab->animateToWidth(config.gutterHoverWidth);
            }
        }
    });
    connect(m_dashboardBtn, &DashboardButton::unhovered, this, [this]() {
        auto& config = infrastructure::ConfigManager::instance().config();
        if (m_controller->currentState() != domain::TabController::State::OPEN) {
            if (!m_dialogOpen) m_collapseTimer->start();
        }
    });
    
    // Add negative spacer before first tab
    m_layout->addSpacerItem(new QSpacerItem(0, -10, QSizePolicy::Fixed, QSizePolicy::Fixed));

// Step 4: Instantiate and configure each tab widget.
    for (int i = 0; i < notes.size(); ++i) {
        const auto& note = notes[i];
        
        // Binder overlap: insert negative vertical spacer (-25px) between consecutive tabs
        if (i > 0) {
            m_layout->addSpacerItem(new QSpacerItem(0, -25, QSizePolicy::Fixed, QSizePolicy::Fixed));
        }
        
        GutterTab* tab = new GutterTab(note, this);
        tab->setFixedHeight(tabHeight);
        
        // Step 5: Determine initial tab width according to active domain state.
        if (isOpen) {
            if (note.id == activeId) {
                tab->setCurrentWidth(config.gutterPeekWidth);
            } else {
                tab->setCurrentWidth(config.gutterRestWidth);
            }
        } else if (isPeeking) {
            tab->setCurrentWidth(config.gutterHoverWidth);
        }
        
        // Step 6: Connect tab interaction signals.
        connect(tab, &GutterTab::hovered, [this, tab]() { onTabHovered(tab); });
        connect(tab, &GutterTab::unhovered, [this, tab]() { onTabUnhovered(tab); });
        connect(tab, &GutterTab::clicked, m_controller, &domain::TabController::openNote);
        
        // Context menu action dispatching
        connect(tab, &GutterTab::addRequested, m_controller, &domain::TabController::createNewNote);
        connect(tab, &GutterTab::renameRequested, this, &GutterStrip::promptRename);
        connect(tab, &GutterTab::changeColorRequested, this, &GutterStrip::promptColor);
        connect(tab, &GutterTab::deleteRequested, m_controller, &domain::TabController::deleteNote);
        connect(tab, &GutterTab::quitRequested, QCoreApplication::instance(), &QCoreApplication::quit);
        
        // Menu blocking: Inhibit collapse timer while context menu is open
        connect(tab, &GutterTab::menuOpened, this, [this]() {
            m_dialogOpen = true;
            m_collapseTimer->stop();
        });
        connect(tab, &GutterTab::menuClosed, this, [this]() {
            m_dialogOpen = false;
            if (m_controller->currentState() != domain::TabController::State::OPEN) {
                if (!rect().contains(mapFromGlobal(QCursor::pos()))) {
                    m_collapseTimer->start(750); // Grace period after context menu close
                }
            }
        });
        
        // Step 7: Add widget to layout and track reference.
        Qt::Alignment align = (config.edge == infrastructure::Config::Edge::Right) 
                              ? Qt::AlignRight 
                              : Qt::AlignLeft;
        m_layout->addWidget(tab, 0, align);
        m_tabs.push_back(tab);
        tab->show();
    }
    
    // Step 8: Cascade Z-ordering to reflect proper stacking overlap.
    updateZOrder();
}

/**
 * @brief Displays modal dialog to rename a note title.
 *
 * @details Suspends the collapse timer via `m_dialogOpen = true`, executes `SleekInputDialog`,
 * writes the updated title to `DatabaseManager`, and requests notes reload. Upon completion,
 * if the cursor is outside the strip, starts the collapse timer with a 750ms grace period.
 *
 * @param noteId ID of the note to be renamed.
 */
void GutterStrip::promptRename(int noteId) {
    m_dialogOpen = true;
    m_collapseTimer->stop();
    
    auto note = infrastructure::DatabaseManager::instance().getNote(noteId);
    if (note) {
        SleekInputDialog dialog("Rename Note", "New Title:", note->title, window());
        if (dialog.exec() == QDialog::Accepted) {
            QString newTitle = dialog.value();
            if (!newTitle.isEmpty()) {
                note->title = newTitle;
                infrastructure::DatabaseManager::instance().saveNote(*note);
                m_controller->loadNotes();
            }
        }
    }
    
    m_dialogOpen = false;
    if (m_controller->currentState() != domain::TabController::State::OPEN) {
        if (!rect().contains(mapFromGlobal(QCursor::pos()))) {
            m_collapseTimer->start(750); // Extended grace period following dialog dismissal
        }
    }
}

/**
 * @brief Displays modal dialog to update note tab color.
 *
 * @details Suspends the collapse timer, executes `SleekColorDialog`, persists the hex color
 * string in `DatabaseManager`, and reloads notes. Restores collapse timer if cursor has departed.
 *
 * @param noteId ID of the note to recolor.
 */
void GutterStrip::promptColor(int noteId) {
    m_dialogOpen = true;
    m_collapseTimer->stop();
    
    auto note = infrastructure::DatabaseManager::instance().getNote(noteId);
    if (note) {
        SleekColorDialog dialog(QColor(note->color), window());
        if (dialog.exec() == QDialog::Accepted) {
            note->color = dialog.selectedColor().name(QColor::HexRgb);
            infrastructure::DatabaseManager::instance().saveNote(*note);
            m_controller->loadNotes();
        }
    }
    
    m_dialogOpen = false;
    if (m_controller->currentState() != domain::TabController::State::OPEN) {
        if (!rect().contains(mapFromGlobal(QCursor::pos()))) {
            m_collapseTimer->start(750); // Extended grace period following dialog dismissal
        }
    }
}

/**
 * @brief Responds to changes in active note selection.
 *
 * @details Execution steps:
 * - If noteId == -1 (deselected / closed):
 *   - If cursor is still over strip: keeps state as PEEKING, animates tabs to hover width.
 *   - If cursor is outside: transitions state to IDLE, animates tabs to rest width, shrinks
 *     strip to 8px, and emits `triggerAreaChanged()`.
 * - If noteId is valid:
 *   - Animates active tab to peek width and all peer tabs to rest width.
 *   - Re-applies Z-order cascade so the active tab displays cleanly.
 *
 * @param noteId Active note ID, or -1 if no note is active.
 */
void GutterStrip::onActiveNoteChanged(int noteId) {
    auto& config = infrastructure::ConfigManager::instance().config();
    if (noteId == -1) {
        if (rect().contains(mapFromGlobal(QCursor::pos()))) {
            m_controller->setState(domain::TabController::State::PEEKING);
            if (m_dashboardBtn) m_dashboardBtn->animateToWidth(config.gutterHoverWidth);
            for (auto tab : m_tabs) {
                tab->animateToWidth(config.gutterHoverWidth);
            }
            setFixedWidth(config.gutterPeekWidth);
            setAcceptDrops(true);
            emit triggerAreaChanged();
        } else {
            m_controller->setState(domain::TabController::State::IDLE);
            if (m_dashboardBtn) m_dashboardBtn->animateToWidth(config.gutterRestWidth);
            for (auto tab : m_tabs) {
                tab->animateToWidth(config.gutterRestWidth);
            }
            setFixedWidth(8);
            emit triggerAreaChanged();
        }
    } else {
        for (auto tab : m_tabs) {
            if (tab->noteId() == noteId) {
                tab->animateToWidth(config.gutterPeekWidth);
            } else {
                tab->animateToWidth(config.gutterRestWidth);
            }
        }
        updateZOrder();
    }
}

/**
 * @brief Handles cursor entering the gutter strip boundary.
 *
 * @details Cancels pending collapse timers, widens strip to `gutterPeekWidth`, enables drag-drop
 * acceptance, signals `OverlayWindow` via `triggerAreaChanged()`, and if not in OPEN state, sets
 * controller state to PEEKING and animates tabs to hover width.
 *
 * @param event The QEnterEvent parameters.
 */
void GutterStrip::enterEvent(QEnterEvent* event) {
    Q_UNUSED(event);
    m_collapseTimer->stop();
    auto& config = infrastructure::ConfigManager::instance().config();
    setFixedWidth(config.gutterPeekWidth);
    setAcceptDrops(true);
    emit triggerAreaChanged();
    
    if (m_controller->currentState() != domain::TabController::State::OPEN) {
        m_controller->setState(domain::TabController::State::PEEKING);
        if (m_dashboardBtn) m_dashboardBtn->animateToWidth(config.gutterHoverWidth);
        for (auto tab : m_tabs) {
            tab->animateToWidth(config.gutterHoverWidth);
        }
    }
}

/**
 * @brief Handles cursor leaving the gutter strip boundary.
 *
 * @details If no dialog/menu is open and state is not OPEN, starts the collapse debounce timer.
 *
 * @param event Base QEvent parameters.
 */
void GutterStrip::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    if (m_dialogOpen) return;
    if (m_controller->currentState() != domain::TabController::State::OPEN) {
        m_collapseTimer->start();
    }
}

/**
 * @brief Handles hover event notification from an individual tab.
 *
 * @details In OPEN state, expands non-active tab to hover width. In non-OPEN state, stops collapse timer,
 * sets state to PEEKING, widens hovered tab to peek width, and peer tabs to hover width.
 *
 * @param hoveredTab Pointer to the hovered GutterTab.
 */
void GutterStrip::onTabHovered(GutterTab* hoveredTab) {
    auto& config = infrastructure::ConfigManager::instance().config();
    if (m_controller->currentState() == domain::TabController::State::OPEN) {
        if (hoveredTab->noteId() != m_controller->activeNoteId()) {
            hoveredTab->animateToWidth(config.gutterHoverWidth);
        }
    } else {
        m_collapseTimer->stop();
        m_controller->setState(domain::TabController::State::PEEKING);
        if (m_dashboardBtn) m_dashboardBtn->animateToWidth(config.gutterHoverWidth);
        for (auto tab : m_tabs) {
            if (tab == hoveredTab) {
                tab->animateToWidth(config.gutterPeekWidth);
            } else {
                tab->animateToWidth(config.gutterHoverWidth);
            }
        }
    }
}

/**
 * @brief Handles unhover notification from an individual tab.
 *
 * @details Restores tab to rest width if in OPEN state, or restarts collapse timer if outside OPEN state.
 *
 * @param unhoveredTab Pointer to the unhovered GutterTab.
 */
void GutterStrip::onTabUnhovered(GutterTab* unhoveredTab) {
    auto& config = infrastructure::ConfigManager::instance().config();
    if (m_controller->currentState() == domain::TabController::State::OPEN) {
        if (unhoveredTab->noteId() != m_controller->activeNoteId()) {
            unhoveredTab->animateToWidth(config.gutterRestWidth);
        }
    } else {
        if (!m_dialogOpen) m_collapseTimer->start();
    }
    updateZOrder();
}

/**
 * @brief Slot executing upon collapse timer expiration.
 *
 * @details Inspects global cursor coordinates to confirm departure. If outside the strip rect,
 * reverts controller state to IDLE, animates tabs to rest width, resets strip width to 8px,
 * and notifies OverlayWindow via `triggerAreaChanged()`.
 */
void GutterStrip::collapseTimerFired() {
    if (m_dialogOpen) return;
    if (m_controller->currentState() == domain::TabController::State::OPEN) return;
    
    if (!rect().contains(mapFromGlobal(QCursor::pos()))) {
        m_controller->setState(domain::TabController::State::IDLE);
        auto& config = infrastructure::ConfigManager::instance().config();
        if (m_dashboardBtn) m_dashboardBtn->animateToWidth(config.gutterRestWidth);
        for (auto tab : m_tabs) {
            tab->animateToWidth(config.gutterRestWidth);
        }
        setFixedWidth(8);
        emit triggerAreaChanged();
    }
}

/**
 * @brief Cascades Z-order across all tabs in sequence.
 *
 * @details Calls `raise()` sequentially on `m_tabs`. This ensures top-to-bottom overlap
 * where tabs lower in the list render consistently relative to upper tabs.
 */
void GutterStrip::updateZOrder() {
    // Cascade from bottom to top
    for (int i = 0; i < m_tabs.size(); ++i) {
        m_tabs[i]->raise();
    }
}

/**
 * @brief Validates prospective drag-and-drop operations entering the strip.
 *
 * @param event The QDragEnterEvent parameters.
 */
void GutterStrip::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasFormat("application/x-guttertab-id")) {
        event->acceptProposedAction();
    }
}

/**
 * @brief Tracks movement during an ongoing tab drag-and-drop operation.
 *
 * @param event The QDragMoveEvent parameters.
 */
void GutterStrip::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasFormat("application/x-guttertab-id")) {
        event->acceptProposedAction();
    }
}

/**
 * @brief Handles drop event to perform note reordering.
 *
 * @details Extracts dragged note ID from MIME format `"application/x-guttertab-id"`,
 * determines insertion index by checking if drop Y coordinate is above each tab's vertical
 * center point, delegates reorder to `TabController::reorderNote()`, and accepts drop.
 *
 * @param event The QDropEvent parameters.
 */
void GutterStrip::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasFormat("application/x-guttertab-id")) {
        // Step 1: Extract dragged note identifier from custom MIME payload.
        int draggedId = event->mimeData()->data("application/x-guttertab-id").toInt();
        
        // Step 2: Compute target insertion index by evaluating cursor Y against tab midpoints.
        int dropY = event->position().y();
        int targetIndex = m_tabs.size();
        
        for (int i = 0; i < m_tabs.size(); ++i) {
            if (dropY < m_tabs[i]->y() + (m_tabs[i]->height() / 2)) {
                targetIndex = i;
                break;
            }
        }
        
        // Step 3: Dispatch reorder command to domain controller.
        m_controller->reorderNote(draggedId, targetIndex);
        event->acceptProposedAction();
    }
}

} // namespace presentation
