/**
 * @file NoteEditorOverlay.cpp
 * @brief Implementation of NoteEditorOverlay presentation component.
 */

#include <QScreen>
#include "NoteEditorOverlay.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextBlockFormat>
#include <QTextListFormat>
#include "../infrastructure/DatabaseManager.h"

namespace presentation {

/**
 * @brief Constructs the NoteEditorOverlay dialog, initializes layouts, toolbars, and signal connections.
 * @param controller Observing pointer to domain TabController.
 * @param parent Optional parent widget.
 */
NoteEditorOverlay::NoteEditorOverlay(domain::TabController* controller, QWidget* parent)
    : QWidget(nullptr, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint), m_controller(controller) {
    
    // Step 1: Configure overlay dimensions and transparency attributes
    setFixedSize(800, 600);
    setAttribute(Qt::WA_TranslucentBackground);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Step 2: Create the sleek visual card frame
    QFrame* card = new QFrame(this);
    card->setObjectName("sleekCard");
    card->setAttribute(Qt::WA_StyledBackground, true);
    
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(12);
    
    // Step 3: Build top action bar with copy-markdown and close buttons
    QHBoxLayout* topLayout = new QHBoxLayout();
    m_copyButton = new QPushButton(card);
    m_copyButton->setObjectName("copyBtn");
    m_copyButton->setToolTip("Copy Markdown");
    m_copyButton->setFixedSize(30, 30);
    m_copyButton->setCursor(Qt::PointingHandCursor);
    
    m_closeButton = new QPushButton("✕", card);
    m_closeButton->setObjectName("cancelBtn");
    m_closeButton->setFixedSize(30, 30);
    m_closeButton->setCursor(Qt::PointingHandCursor);
    
    topLayout->addStretch();
    topLayout->addWidget(m_copyButton);
    topLayout->addSpacing(4);
    topLayout->addWidget(m_closeButton);
    cardLayout->addLayout(topLayout);
    
    // Step 4: Build formatting toolbar buttons
    QHBoxLayout* formatLayout = new QHBoxLayout();
    formatLayout->setSpacing(8);
    auto* btnBold = createToolbarBtn("B"); btnBold->setObjectName("btnBold");
    auto* btnItalic = createToolbarBtn("I"); btnItalic->setObjectName("btnItalic");
    auto* btnUnder = createToolbarBtn("U"); btnUnder->setObjectName("btnUnder");
    auto* btnStrike = createToolbarBtn("S"); btnStrike->setObjectName("btnStrike");
    
    auto* btnH1 = createToolbarBtn("H1"); btnH1->setObjectName("btnH1");
    auto* btnH2 = createToolbarBtn("H2"); btnH2->setObjectName("btnH2");
    auto* btnH3 = createToolbarBtn("H3"); btnH3->setObjectName("btnH3");
    
    auto* btnBullet = createToolbarBtn("•"); btnBullet->setObjectName("btnBullet");
    auto* btnNum = createToolbarBtn("1."); btnNum->setObjectName("btnNum");
    
    formatLayout->addWidget(btnBold);
    formatLayout->addWidget(btnItalic);
    formatLayout->addWidget(btnUnder);
    formatLayout->addWidget(btnStrike);
    formatLayout->addSpacing(10);
    formatLayout->addWidget(btnH1);
    formatLayout->addWidget(btnH2);
    formatLayout->addWidget(btnH3);
    formatLayout->addSpacing(10);
    formatLayout->addWidget(btnBullet);
    formatLayout->addWidget(btnNum);
    formatLayout->addStretch();
    
    cardLayout->addLayout(formatLayout);
    
    // Step 5: Instantiate and configure embedded MarkdownEditor
    m_textEdit = new MarkdownEditor(card);
    cardLayout->addWidget(m_textEdit);
    
    mainLayout->addWidget(card);
    
    // Step 6: Connect formatting button triggers
    connect(btnBold, &QPushButton::clicked, this, &NoteEditorOverlay::onFormatBold);
    connect(btnItalic, &QPushButton::clicked, this, &NoteEditorOverlay::onFormatItalic);
    connect(btnUnder, &QPushButton::clicked, this, &NoteEditorOverlay::onFormatUnderline);
    connect(btnStrike, &QPushButton::clicked, this, &NoteEditorOverlay::onFormatStrike);
    connect(btnH1, &QPushButton::clicked, this, &NoteEditorOverlay::onFormatH1);
    connect(btnH2, &QPushButton::clicked, this, &NoteEditorOverlay::onFormatH2);
    connect(btnH3, &QPushButton::clicked, this, &NoteEditorOverlay::onFormatH3);
    connect(btnBullet, &QPushButton::clicked, this, &NoteEditorOverlay::onFormatBulletList);
    connect(btnNum, &QPushButton::clicked, this, &NoteEditorOverlay::onFormatNumberedList);
    
    // Step 7: Connect window actions and content change monitoring
    connect(m_copyButton, &QPushButton::clicked, this, &NoteEditorOverlay::onCopyToClipboard);
    connect(m_closeButton, &QPushButton::clicked, this, &NoteEditorOverlay::onCloseClicked);
    connect(m_textEdit, &QTextEdit::textChanged, this, &NoteEditorOverlay::onTextChanged);
    
    // Step 8: Observe domain state changes via signals/slots
    connect(m_controller, &domain::TabController::activeNoteChanged, this, &NoteEditorOverlay::onActiveNoteChanged);
    
    // Step 9: Install event filter on editor and overlay to capture Escape key events
    m_textEdit->installEventFilter(this);
    installEventFilter(this);
    
    hide();
}

/**
 * @brief Cleans up overlay resources upon destruction.
 */
NoteEditorOverlay::~NoteEditorOverlay() {}

/**
 * @brief Factory helper to generate styled toolbar push buttons.
 * @param text Button label or symbol.
 * @return Pointer to the newly created QPushButton.
 */
QPushButton* NoteEditorOverlay::createToolbarBtn(const QString& text) {
    auto* btn = new QPushButton(text, this);
    btn->setObjectName("toolbarBtn");
    btn->setFixedSize(36, 36);
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

/**
 * @brief Handles active note state changes by loading note content and updating visual styling.
 * @param noteId ID of newly active note, or -1 if no note is active.
 */
void NoteEditorOverlay::onActiveNoteChanged(int noteId) {
    // Step 1: If noteId is -1, dismiss overlay immediately
    if (noteId == -1) {
        hide();
    } else {
        // Step 2: Fetch note entity from persistent SQLite storage
        auto noteOpt = infrastructure::DatabaseManager::instance().getNote(noteId);
        if (noteOpt) {
            // Step 3: Populate MarkdownEditor while blocking signals to avoid feedback loops
            m_textEdit->blockSignals(true);
            m_textEdit->setMarkdown(noteOpt->content);
            m_textEdit->blockSignals(false);
            
            // Step 4: Compute semi-transparent tint based on the note's tab accent color
            QColor c(noteOpt->color);
            QString bgColor = QString("rgba(%1, %2, %3, 0.15)").arg(c.red()).arg(c.green()).arg(c.blue());
            QString hoverColor = QString("rgba(%1, %2, %3, 0.3)").arg(c.red()).arg(c.green()).arg(c.blue());
            
            QString globalStyle = QString(
                "QFrame#sleekCard {"
                "    background-color: %1;"
                "    border: 2px solid %2;"
                "    border-radius: 12px;"
                "}"
                "QTextEdit {"
                "    background: transparent;"
                "    color: #e4e4e7;"
                "    font-family: sans-serif;"
                "    font-size: 16px;"
                "    border: none;"
                "}"
                "QScrollBar:vertical {"
                "    border: none;"
                "    background: transparent;"
                "    width: 10px;"
                "    margin: 0px 0px 0px 0px;"
                "}"
                "QScrollBar::handle:vertical {"
                "    background: rgba(255, 255, 255, 0.2);"
                "    min-height: 20px;"
                "    border-radius: 5px;"
                "}"
                "QScrollBar::handle:vertical:hover {"
                "    background: rgba(255, 255, 255, 0.4);"
                "}"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
                "    border: none;"
                "    background: none;"
                "    height: 0px;"
                "}"
                "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
                "    background: none;"
                "}"
                "QPushButton#toolbarBtn {"
                "    background-color: transparent;"
                "    color: #d4d4d8;"
                "    border: none;"
                "    border-radius: 4px;"
                "    font-size: 14px;"
                "}"
                "QPushButton#toolbarBtn:hover {"
                "    background-color: %3;"
                "    color: white;"
                "}"
                "QPushButton#btnBold { font-weight: bold; }"
                "QPushButton#btnItalic { font-style: italic; }"
                "QPushButton#btnUnder { text-decoration: underline; }"
                "QPushButton#btnStrike { text-decoration: line-through; }"
                "QPushButton#copyBtn {"
                "    background-color: transparent; border: none; border-radius: 4px;"
                "    image: url(\"data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='%23ffffff' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><rect x='9' y='9' width='13' height='13' rx='2' ry='2'></rect><path d='M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1'></path></svg>\");"
                "}"
                "QPushButton#copyBtn:hover {"
                "    background-color: %3;"
                "}"
                "QPushButton#cancelBtn {"
                "    background-color: transparent; color: #a1a1aa; border: none; font-weight: bold; font-size: 14px;"
                "}"
                "QPushButton#cancelBtn:hover { color: white; background-color: #ef4444; border-radius: 15px; }"
            ).arg(bgColor, noteOpt->color, hoverColor);
            
            QFrame* card = findChild<QFrame*>("sleekCard");
            if (card) {
                card->setStyleSheet(globalStyle);
            }
            
            // Step 5: Center the card on the primary display geometry
            if (QScreen* screen = QGuiApplication::primaryScreen()) {
                move(screen->geometry().center() - rect().center());
            }
            
            // Step 6: Reveal overlay window, activate it, and transfer keyboard focus to editor
            show();
            activateWindow();
            m_textEdit->setFocus();
        }
    }
}

/**
 * @brief Saves updated Markdown content to domain controller when text changes.
 */
void NoteEditorOverlay::onTextChanged() {
    int currentId = m_controller->activeNoteId();
    if (currentId != -1) {
        // Step 1: Extract document as serialized Markdown string and notify controller
        m_controller->updateNoteContent(currentId, m_textEdit->toMarkdown());
    }
}

/**
 * @brief Copies the current note content in Markdown format to the system clipboard.
 */
void NoteEditorOverlay::onCopyToClipboard() {
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(m_textEdit->toMarkdown());
}

/**
 * @brief Closes the current active note via domain controller.
 */
void NoteEditorOverlay::onCloseClicked() {
    m_controller->closeNote();
}

/**
 * @brief Consumes mouse clicks on the overlay frame to prevent click-through dismissal.
 * @param event Mouse event details.
 */
void NoteEditorOverlay::mousePressEvent(QMouseEvent* event) {
    // Accepting the event prevents clicks on the card background from propagating
    // to the underlying full-screen backdrop, ensuring click-outside dismissal
    // only triggers when clicking truly outside this card.
    event->accept();
}

/**
 * @brief Intercepts key presses to provide Escape shortcut dismissal.
 * @param obj Event recipient.
 * @param event Event details.
 * @return True if handled, false otherwise.
 */
bool NoteEditorOverlay::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            // Escape key pressed: Close active note gracefully
            m_controller->closeNote();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

// ============================================================================
// Formatting Action Implementations
// ============================================================================

/**
 * @brief Toggles bold font weight on selected text or cursor position.
 */
void NoteEditorOverlay::onFormatBold() {
    QTextCharFormat fmt;
    fmt.setFontWeight(m_textEdit->fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
    m_textEdit->mergeCurrentCharFormat(fmt);
    m_textEdit->setFocus();
}

/**
 * @brief Toggles italic font styling on selected text or cursor position.
 */
void NoteEditorOverlay::onFormatItalic() {
    QTextCharFormat fmt;
    fmt.setFontItalic(!m_textEdit->fontItalic());
    m_textEdit->mergeCurrentCharFormat(fmt);
    m_textEdit->setFocus();
}

/**
 * @brief Toggles underline font styling on selected text or cursor position.
 */
void NoteEditorOverlay::onFormatUnderline() {
    QTextCharFormat fmt;
    fmt.setFontUnderline(!m_textEdit->fontUnderline());
    m_textEdit->mergeCurrentCharFormat(fmt);
    m_textEdit->setFocus();
}

/**
 * @brief Toggles strike-through font styling on selected text or cursor position.
 */
void NoteEditorOverlay::onFormatStrike() {
    QTextCharFormat fmt;
    fmt.setFontStrikeOut(!m_textEdit->currentCharFormat().fontStrikeOut());
    m_textEdit->mergeCurrentCharFormat(fmt);
    m_textEdit->setFocus();
}

/**
 * @brief Toggles Heading 1 formatting on the current block/paragraph.
 */
void NoteEditorOverlay::onFormatH1() {
    QTextCursor cursor = m_textEdit->textCursor();
    QTextBlockFormat fmt = cursor.blockFormat();
    fmt.setHeadingLevel(fmt.headingLevel() == 1 ? 0 : 1);
    cursor.setBlockFormat(fmt);
    m_textEdit->setFocus();
}

/**
 * @brief Toggles Heading 2 formatting on the current block/paragraph.
 */
void NoteEditorOverlay::onFormatH2() {
    QTextCursor cursor = m_textEdit->textCursor();
    QTextBlockFormat fmt = cursor.blockFormat();
    fmt.setHeadingLevel(fmt.headingLevel() == 2 ? 0 : 2);
    cursor.setBlockFormat(fmt);
    m_textEdit->setFocus();
}

/**
 * @brief Toggles Heading 3 formatting on the current block/paragraph.
 */
void NoteEditorOverlay::onFormatH3() {
    QTextCursor cursor = m_textEdit->textCursor();
    QTextBlockFormat fmt = cursor.blockFormat();
    fmt.setHeadingLevel(fmt.headingLevel() == 3 ? 0 : 3);
    cursor.setBlockFormat(fmt);
    m_textEdit->setFocus();
}

/**
 * @brief Toggles bullet list formatting on the current block.
 */
void NoteEditorOverlay::onFormatBulletList() {
    QTextCursor cursor = m_textEdit->textCursor();
    if (cursor.currentList()) {
        QTextBlockFormat fmt = cursor.blockFormat();
        fmt.setObjectIndex(-1);
        cursor.setBlockFormat(fmt);
    } else {
        cursor.createList(QTextListFormat::ListDisc);
    }
    m_textEdit->setFocus();
}

/**
 * @brief Toggles numbered list formatting on the current block.
 */
void NoteEditorOverlay::onFormatNumberedList() {
    QTextCursor cursor = m_textEdit->textCursor();
    if (cursor.currentList()) {
        QTextBlockFormat fmt = cursor.blockFormat();
        fmt.setObjectIndex(-1);
        cursor.setBlockFormat(fmt);
    } else {
        cursor.createList(QTextListFormat::ListDecimal);
    }
    m_textEdit->setFocus();
}

} // namespace presentation

