#include <QDrag>
#include <QMimeData>
#include <QTextDocument>
#include <QApplication>
#include "GutterTab.h"
#include <QPainter>
#include <QColor>
#include <QMouseEvent>
#include <QApplication>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include "../infrastructure/ConfigManager.h"
#include "SleekDialogs.h"

namespace presentation {

/**
 * @brief Constructs a GutterTab widget.
 *
 * @details Initializes tab width to the configured rest width, creates a `QPropertyAnimation`
 * targeting the `currentWidth` property with a 150ms duration and an `OutCubic` easing curve,
 * providing fluid responsive width animations on user interactions.
 *
 * @param note Note domain model containing metadata and content.
 * @param parent Optional parent widget (typically GutterStrip).
 */
GutterTab::GutterTab(const domain::Note& note, QWidget* parent)
    : QWidget(parent), m_note(note) {
    
    // Step 1: Initialize current width to resting configuration value (e.g. 2px).
    m_currentWidth = infrastructure::ConfigManager::instance().config().gutterRestWidth;
    setFixedWidth(m_currentWidth);
    
    // Step 2: Configure width interpolation animation.
    // - QPropertyAnimation operates on Q_PROPERTY(int currentWidth ...).
    // - QEasingCurve::OutCubic produces an organic deceleration profile.
    m_animation = new QPropertyAnimation(this, "currentWidth", this);
    m_animation->setDuration(150);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
}

/**
 * @brief Destructor for GutterTab.
 */
GutterTab::~GutterTab() {
}

/**
 * @brief Synchronizes the tab with an updated note model and triggers a repaint.
 *
 * @param note Updated Note domain model.
 */
void GutterTab::updateNote(const domain::Note& note) {
    m_note = note;
    update();
}

/**
 * @brief Sets the animated width of the tab widget.
 *
 * @details Invoked during property animation step interpolation. Updates internal width,
 * synchronizes widget fixed width, and requests an asynchronous repaint.
 *
 * @param width New width in pixels.
 */
void GutterTab::setCurrentWidth(int width) {
    if (m_currentWidth != width) {
        m_currentWidth = width;
        setFixedWidth(width);
        update(); // Trigger repaint
    }
}

/**
 * @brief Smoothly animates the tab's width to a target pixel value.
 *
 * @details Preempts any running animation to prevent interpolation conflicts or jitter,
 * sets the new target end value, and launches the animation.
 *
 * @param targetWidth Desired target width in pixels.
 */
void GutterTab::animateToWidth(int targetWidth) {
    // Step 1: Preempt active animation if state is in-flight
    if (m_animation->state() == QAbstractAnimation::Running) {
        m_animation->stop();
    }
    // Step 2: Assign target destination width and execute
    m_animation->setEndValue(targetWidth);
    m_animation->start();
}

/**
 * @brief Paints the tab background, rotated vertical title, and preview snippet.
 *
 * @details Execution sequence:
 * 1. Background fill: Renders an antialiased rounded rectangle (4px radius) using the note's hex color.
 * 2. Title rendering (when width > rest width):
 *    - Saves the QPainter transformation matrix.
 *    - Configures a bold 10pt font and measures title width via `QFontMetrics::horizontalAdvance`.
 *    - Translates the origin to the center of the widget (`width() / 2, height() / 2`).
 *    - Rotates coordinate space by -90 degrees counter-clockwise to render text vertically upwards.
 *    - Lays out text in the transformed rectangle `(-height() / 2, -width() / 2, height(), width())`.
 *    - Restores the original QPainter transformation matrix.
 * 3. Snippet rendering (when width > hover width + 20):
 *    - Computes snippet bounding box starting offset past the vertical title tab margin.
 *    - Renders up to 3 non-empty lines extracted from note content with word wrap and 80% opacity.
 *
 * @param event The QPaintEvent parameters (unused).
 */
void GutterTab::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Step 1: Draw rounded rectangle background using the note's assigned color.
    QColor bgColor(m_note.color);
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 4, 4);
    
    // Step 2: Draw vertical title if tab has expanded beyond resting sliver width.
    auto& config = infrastructure::ConfigManager::instance().config();
    if (m_currentWidth > config.gutterRestWidth) {
        painter.setPen(Qt::white);
        
        // Save painter state prior to coordinate transformation.
        painter.save();
        
        // Configure font and compute metric bounds.
        QFont font = painter.font();
        font.setBold(true);
        font.setPointSize(10);
        painter.setFont(font);
        
        QFontMetrics fm(font);
        int textWidth = fm.horizontalAdvance(m_note.title);
        Q_UNUSED(textWidth);
        
        // Translate origin to widget center and apply -90 deg rotation for vertical text flow.
        painter.translate(width() / 2, height() / 2);
        painter.rotate(-90);
        
        // In the rotated coordinate frame, dimensions are inverted (width <-> height).
        QRect textRect(-height() / 2, -width() / 2, height(), width());
        painter.drawText(textRect, Qt::AlignCenter, m_note.title);
        
        // Restore painter coordinate matrix.
        painter.restore();
        
        // Step 3: Draw peek excerpt snippet if width has expanded sufficiently.
        if (m_currentWidth > config.gutterHoverWidth + 20) {
            QRect snippetRect(30, 10, m_currentWidth - 40, height() - 20);
            
            QFont snippetFont = painter.font();
            snippetFont.setPointSize(9);
            painter.setFont(snippetFont);
            
            painter.setOpacity(0.8);
            painter.drawText(snippetRect, Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap, getSnippet());
        }
    }
}

/**
 * @brief Extracts up to 3 non-empty lines from note content for peek snippet rendering.
 *
 * @details Splits content by newline delimiter, skips empty lines, accumulates up to 3 lines,
 * and returns the trimmed excerpt.
 *
 * @return Formatted multi-line preview snippet.
 */
QString GutterTab::getSnippet() const {
    // Safely extract plain text from HTML or Markdown
    QTextDocument doc;
    if (m_note.content.trimmed().startsWith("<") || m_note.content.contains("<html")) {
        doc.setHtml(m_note.content);
    } else {
        doc.setMarkdown(m_note.content);
    }
    
    // Extract first 3 non-empty lines for snippet
    QString plainText = doc.toPlainText();
    QStringList lines = plainText.split('\n', Qt::SkipEmptyParts);
    QString snippet;
    int count = 0;
    for (const QString& line : lines) {
        snippet += line + "\n";
        count++;
        if (count >= 3) break;
    }
    return snippet.trimmed();
}

/**
 * @brief Handles mouse enter events into the tab surface.
 *
 * @param event The QEnterEvent parameters.
 */
void GutterTab::enterEvent(QEnterEvent* event) {
    Q_UNUSED(event);
    emit hovered();
}

/**
 * @brief Handles mouse leave events from the tab surface.
 *
 * @param event Base QEvent parameters.
 */
void GutterTab::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    emit unhovered();
}

/**
 * @brief Handles mouse press events on the tab.
 *
 * @details On left click, records the start position `m_dragStartPosition` for drag threshold
 * evaluation and emits `clicked(m_note.id)` to request note opening.
 *
 * @param event The QMouseEvent parameters.
 */
void GutterTab::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
        emit clicked(m_note.id);
    }
}

/**
 * @brief Displays the sleek context menu for note actions.
 *
 * @details Execution steps:
 * 1. Emits `menuOpened()` to notify parent container to inhibit the collapse timer.
 * 2. Populates `SleekContextMenu` with actions: Add New Note, Rename Note, Change Color, Delete, Close.
 * 3. Shows the menu synchronously at the cursor position via `menu.exec()`.
 * 4. Emits `menuClosed()` to allow parent container to resume collapse timers.
 * 5. Dispatches appropriate domain signals (`addRequested`, `renameRequested`, etc.) based on action selected.
 *
 * @param event The QContextMenuEvent containing trigger coordinates.
 */
void GutterTab::contextMenuEvent(QContextMenuEvent* event) {
    // Step 1: Signal parent to suspend collapse timer while user navigates context menu.
    emit menuOpened();
    
    // Step 2: Construct custom styled context menu.
    SleekContextMenu menu(this);
    
    QAction* addAction = menu.addAction("Add New Note");
    QAction* renameAction = menu.addAction("Rename Note");
    QAction* colorAction = menu.addAction("Change Color");
    menu.addSeparator();
    QAction* deleteAction = menu.addAction("Delete");
    menu.addSeparator();
    QAction* quitAction = menu.addAction("Close gutterTab");
    
    // Step 3: Block synchronously on modal menu execution.
    QAction* selected = menu.exec(event->globalPos());
    
    // Step 4: Signal parent that context menu has closed.
    emit menuClosed();
    
    // Step 5: Dispatch user selection.
    if (selected == addAction) emit addRequested();
    else if (selected == renameAction) emit renameRequested(m_note.id);
    else if (selected == colorAction) emit changeColorRequested(m_note.id);
    else if (selected == deleteAction) emit deleteRequested(m_note.id);
    else if (selected == quitAction) emit quitRequested();
}

/**
 * @brief Evaluates mouse drag thresholds and initiates tab drag-and-drop.
 *
 * @details Validates left-mouse button state and confirms drag movement distance exceeds
 * `QApplication::startDragDistance()` to avoid triggering drags on slight cursor tremors.
 * Renders a transparent pixmap preview of the tab into the `QDrag` payload and executes `Qt::MoveAction`.
 *
 * @param event The QMouseEvent containing movement coordinates.
 */
void GutterTab::mouseMoveEvent(QMouseEvent* event) {
    // Step 1: Confirm left button is held down.
    if (!(event->buttons() & Qt::LeftButton))
        return;

    // Step 2: Enforce minimum drag threshold to avoid accidental drags.
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance())
        return;

    // Step 3: Construct drag object and attach custom MIME data payload.
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    
    mimeData->setData("application/x-guttertab-id", QByteArray::number(m_note.id));
    drag->setMimeData(mimeData);
    
    // Step 4: Generate a transparent pixmap snapshot of the tab for drag visual feedback.
    QPixmap pixmap(size());
    pixmap.fill(Qt::transparent);
    render(&pixmap);
    drag->setPixmap(pixmap);
    drag->setHotSpot(event->pos());
    
    // Step 5: Execute drag operation blocking until drop or cancellation.
    drag->exec(Qt::MoveAction);
}

} // namespace presentation
