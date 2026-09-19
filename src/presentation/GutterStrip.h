#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QTimer>
#include "../domain/TabController.h"
#include "GutterTab.h"
#include "DashboardButton.h"

namespace presentation {

/**
 * @brief Container widget managing the vertical stack of notebook-style gutter tabs on the screen edge.
 *
 * @details GutterStrip acts as the primary navigational strip anchored along the screen edge (left or right).
 * Key architectural design aspects include:
 * - **Vertical Stacking & Margins:** Uses a `QVBoxLayout` aligned to the configured edge. It applies an 80px
 *   dead-zone margin at the top and bottom (`screenHeight - 160`) so tabs never overlap system panels, status
 *   bars (e.g., tint2, polybar), or desktop screen corners. Tab heights scale dynamically up to 180px.
 * - **Binder Overlap Spacing:** Inserts negative spacer items (`QSpacerItem(0, -25, ...)`) between consecutive
 *   tabs to create an authentic physical binder / index-tab overlapping aesthetic.
 * - **Z-Ordering Cascade:** Implements `updateZOrder()` which calls `raise()` from top to bottom, ensuring
 *   each upper tab casts its edge over the tab beneath it without visual clipping or inverted seams.
 * - **Hover Collapse Debounce:** Manages a single-shot timer (`m_collapseTimer`, 150ms default, 750ms on dialog
 *   dismissals) to gracefully collapse the strip to an 8px sliver when the cursor leaves, preventing accidental
 *   collapses during fast cursor transit between tabs.
 * - **Drag-and-Drop Reordering:** Serves as the drop target for reordering tabs via Qt's drag-and-drop system
 *   using MIME type `"application/x-guttertab-id"`. Calculates the target insertion index from cursor Y
 *   coordinates relative to tab centers and commits changes via `TabController::reorderNote()`.
 *
 * @note Must only be accessed from the main GUI thread. When modal dialogs (rename, color picker) or context
 * menus are active, the collapse timer is inhibited via `m_dialogOpen` to prevent jarring UI state changes.
 */
class GutterStrip : public QWidget {
    Q_OBJECT
signals:
    /**
     * @brief Emitted whenever the strip's interactive width expands or contracts.
     *
     * @details Notifies the parent `OverlayWindow` to resize its underlying X11 geometry to match
     * the strip's current bounds, maintaining desktop click-through outside the interactive zone.
     */
    void triggerAreaChanged();

public:
    /**
     * @brief Constructs the GutterStrip container.
     *
     * @param controller Non-null pointer to domain TabController managing note models and states.
     * @param parent Optional parent widget (typically the OverlayWindow).
     */
    explicit GutterStrip(domain::TabController* controller, QWidget* parent = nullptr);

    /**
     * @brief Destructor for GutterStrip.
     */
    ~GutterStrip() override;

    /**
     * @brief Rebuilds or updates in-place the collection of GutterTab child widgets.
     *
     * @details Evaluates whether tabs can be updated in-place (if note count and IDs match existing tabs)
     * to avoid unnecessary widget destruction and layout thrashing. If structural re-ordering or item count
     * changes occurred, clears the layout, calculates available vertical space excluding the 80px top/bottom
     * dead-zones, instantiates new `GutterTab` instances with negative spacing spacers (-25px), establishes
     * context menu connections, and cascades Z-ordering.
     */
    void updateTabs();

    /**
     * @brief Re-applies the cascading Z-order across all child tab widgets.
     *
     * @details Iterates over `m_tabs` calling `raise()` sequentially to ensure higher-indexed tabs
     * overlap properly according to the binder aesthetic.
     */
    void updateZOrder();

protected:
    /**
     * @brief Handles mouse enter events into the gutter strip area.
     *
     * @details Cancels pending collapse timers, expands the strip width to peek width, enables drag-and-drop
     * acceptance, informs `OverlayWindow` via `triggerAreaChanged()`, and transitions the controller to `PEEKING`
     * if not currently `OPEN`.
     *
     * @param event Mouse enter event parameters.
     */
    void enterEvent(QEnterEvent* event) override;

    /**
     * @brief Handles mouse leave events when the cursor exits the gutter strip.
     *
     * @details Starts the 150ms collapse timer to debounce accidental mouse slips, provided no modal dialog
     * or context menu is currently active (`!m_dialogOpen`).
     *
     * @param event Base QEvent parameters.
     */
    void leaveEvent(QEvent* event) override;

    /**
     * @brief Validates prospective drag-and-drop operations entering the strip.
     *
     * @details Inspects the drag payload for format `"application/x-guttertab-id"`. If present, accepts
     * the proposed action.
     *
     * @param event The QDragEnterEvent containing MIME data.
     */
    void dragEnterEvent(QDragEnterEvent* event) override;

    /**
     * @brief Tracks movement during an ongoing tab drag-and-drop operation.
     *
     * @details Confirms format validity while the user moves the tab over potential drop slots.
     *
     * @param event The QDragMoveEvent parameters.
     */
    void dragMoveEvent(QDragMoveEvent* event) override;

    /**
     * @brief Handles the drop event to complete note tab reordering.
     *
     * @details Extracts the dragged note ID from MIME data, computes the target insertion index by comparing
     * the drop Y coordinate against each tab's vertical midpoint, and invokes `TabController::reorderNote()`.
     *
     * @param event The QDropEvent parameters.
     */
    void dropEvent(QDropEvent* event) override;

private slots:
    /**
     * @brief Responds to changes in the active note selection.
     *
     * @details Adjusts tab widths:
     * - If noteId is -1 (deselected), either animates tabs to hover width (if cursor is still inside) or
     *   to rest width and shrinks strip to 8px.
     * - If noteId is valid, expands the selected tab to peek width and others to rest width.
     *
     * @param noteId ID of the newly activated note, or -1 if none is active.
     */
    void onActiveNoteChanged(int noteId);

    /**
     * @brief Handles hover notification from an individual tab.
     *
     * @details In OPEN state, expands non-active tabs to hover width on hover. In non-OPEN state, stops the
     * collapse timer, transitions controller to PEEKING, expands the hovered tab to peek width, and expands
     * peer tabs to hover width.
     *
     * @param hoveredTab Pointer to the hovered GutterTab.
     */
    void onTabHovered(GutterTab* hoveredTab);

    /**
     * @brief Handles unhover notification from an individual tab.
     *
     * @details Restores tab widths and restarts the collapse timer if outside the active state.
     *
     * @param unhoveredTab Pointer to the unhovered GutterTab.
     */
    void onTabUnhovered(GutterTab* unhoveredTab);

    /**
     * @brief Slot triggered upon expiration of the hover collapse timer.
     *
     * @details Verifies whether the cursor is truly outside the widget boundary via global cursor mapping.
     * If outside, sets controller state to IDLE, animates all tabs to rest width, collapses strip width to 8px,
     * and emits `triggerAreaChanged()`.
     */
    void collapseTimerFired();
    
    /**
     * @brief Displays the sleek modal dialog to rename a note.
     *
     * @details Blocks the collapse timer during dialog execution, updates the note title in `DatabaseManager`
     * upon confirmation, and triggers `TabController::loadNotes()`.
     *
     * @param noteId ID of the note to rename.
     */
    void promptRename(int noteId);

    /**
     * @brief Displays the sleek modal color picker dialog to customize a note's tab color.
     *
     * @details Blocks the collapse timer during dialog execution, saves the selected hex color in `DatabaseManager`
     * upon confirmation, and triggers `TabController::loadNotes()`.
     *
     * @param noteId ID of the note to recolor.
     */
    void promptColor(int noteId);

private:
    domain::TabController* m_controller; ///< Non-owning pointer to the domain TabController.
    QVBoxLayout* m_layout;               ///< Layout managing the vertical stacking of tabs and spacers.
    DashboardButton* m_dashboardBtn;
    QVector<GutterTab*> m_tabs;          ///< Ordered list of active child GutterTab widgets.
    
    QTimer* m_collapseTimer;             ///< Single-shot debounce timer for collapsing the strip on mouse leave.
    bool m_dialogOpen = false;           ///< Flag indicating if a modal dialog or context menu is active.
};

} // namespace presentation
