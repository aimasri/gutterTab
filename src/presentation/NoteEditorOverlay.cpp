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
    
    // Step 3 & 4: Build combined top action bar (formatting + close/copy)
    QHBoxLayout* topBarLayout = new QHBoxLayout();
    topBarLayout->setSpacing(6);
    
    auto* btnBold = createToolbarBtn("B"); btnBold->setObjectName("btnBold");
    auto* btnItalic = createToolbarBtn("I"); btnItalic->setObjectName("btnItalic");
    auto* btnUnder = createToolbarBtn("U"); btnUnder->setObjectName("btnUnder");
    auto* btnStrike = createToolbarBtn("S"); btnStrike->setObjectName("btnStrike");
    
    auto* btnH1 = createToolbarBtn("H1"); btnH1->setObjectName("btnH1");
    auto* btnH2 = createToolbarBtn("H2"); btnH2->setObjectName("btnH2");
    auto* btnH3 = createToolbarBtn("H3"); btnH3->setObjectName("btnH3");
    
    auto* btnBullet = createToolbarBtn("•"); btnBullet->setObjectName("btnBullet");
    auto* btnNum = createToolbarBtn("1."); btnNum->setObjectName("btnNum");
    auto* btnSpace = createToolbarBtn("Space"); btnSpace->setObjectName("btnSpace");
    btnSpace->setToolTip("Insert Hard Line Break");
    
    topBarLayout->addWidget(btnBold);
    topBarLayout->addWidget(btnItalic);
    topBarLayout->addWidget(btnUnder);
    topBarLayout->addWidget(btnStrike);
    topBarLayout->addSpacing(12);
    topBarLayout->addWidget(btnH1);
    topBarLayout->addWidget(btnH2);
    topBarLayout->addWidget(btnH3);
    topBarLayout->addSpacing(12);
    topBarLayout->addWidget(btnBullet);
    topBarLayout->addWidget(btnNum);
    topBarLayout->addSpacing(12);
    topBarLayout->addWidget(btnSpace);
    
    topBarLayout->addStretch();
    
    m_copyButton = new QPushButton("Copy", card);
    m_copyButton->setProperty("isToolbarBtn", "true");
    m_copyButton->setToolTip("Copy Markdown");
    m_copyButton->setCursor(Qt::PointingHandCursor);
    
    m_closeButton = new QPushButton("✕", card);
    m_closeButton->setProperty("isToolbarBtn", "true");
    m_closeButton->setObjectName("cancelBtn"); // Keep this to make hover red
    m_closeButton->setFixedSize(30, 30);
    m_closeButton->setCursor(Qt::PointingHandCursor);
    
    topBarLayout->addWidget(m_copyButton);
    topBarLayout->addSpacing(4);
    topBarLayout->addWidget(m_closeButton);
    
    cardLayout->addLayout(topBarLayout);
    
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
    connect(btnSpace, &QPushButton::clicked, this, &NoteEditorOverlay::onFormatSpace);
    
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
    btn->setProperty("isToolbarBtn", "true");
    btn->setFixedSize(30, 30);
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
            // Mix dark background #18181b (24, 24, 27) with the accent color heavily
            int r = (24 * 6 + c.red()) / 7;
            int g = (24 * 6 + c.green()) / 7;
            int b = (27 * 6 + c.blue()) / 7;
            QString bgColor = QString("rgba(%1, %2, %3, 0.98)").arg(r).arg(g).arg(b);
            
            QString globalStyle = QString(
                "QFrame#sleekCard {"
                "    background-color: %1;"
                "    border: 1px solid %2;"
                "    border-radius: 10px;"
                "}"
                "QTextEdit {"
                "    background: #18181b;"
                "    color: #e4e4e7;"
                "    font-family: sans-serif;"
                "    font-size: 16px;"
                "    border: 1px solid #3f3f46;"
                "    border-radius: 6px;"
                "    padding: 12px;"
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
                "QPushButton[isToolbarBtn=\"true\"] {"
                "    background-color: #27272a;"
                "    color: #d4d4d8;"
                "    border: 1px solid #3f3f46;"
                "    border-radius: 4px;"
                "    font-size: 14px;"
                "}"
                "QPushButton[isToolbarBtn=\"true\"]:hover {"
                "    background-color: #3f3f46;"
                "    color: white;"
                "    border: 1px solid %2;"
                "}"
                "QPushButton#btnBold { font-weight: bold; }"
                "QPushButton#btnItalic { font-style: italic; }"
                "QPushButton#btnUnder { text-decoration: underline; }"
                "QPushButton#btnStrike { text-decoration: line-through; }"
                "QPushButton#cancelBtn {"
                "    background-color: transparent; color: #a1a1aa; border: none; font-weight: bold; font-size: 14px;"
                "}"
                "QPushButton#cancelBtn:hover { color: white; background-color: #ef4444; border-radius: 15px; }"
            ).arg(bgColor, noteOpt->color);
            
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
        QString md = m_textEdit->toMarkdown();
        qDebug() << "Saving note ID:" << currentId << "Content Length:" << md.length();
        m_controller->updateNoteContent(currentId, md);
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

/**
 * @brief Inserts a hard HTML break to bypass Markdown empty-line collapsing.
 */
void NoteEditorOverlay::onFormatSpace() {
    m_textEdit->textCursor().insertHtml("&nbsp;<br><br>");
    m_textEdit->setFocus();
}

} // namespace presentation

