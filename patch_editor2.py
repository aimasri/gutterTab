import re

cpp_path = 'src/presentation/NoteEditorOverlay.cpp'
with open(cpp_path, 'r') as f:
    content = f.read()

# 1. Update Constructor
start_idx = content.find('NoteEditorOverlay::NoteEditorOverlay')
end_idx = content.find('// Step 5: Instantiate and configure embedded MarkdownEditor')

new_constructor = """NoteEditorOverlay::NoteEditorOverlay(domain::TabController* controller, QWidget* parent)
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
    
    topBarLayout->addStretch();
    
    m_copyButton = new QPushButton(card);
    m_copyButton->setObjectName("copyBtn");
    m_copyButton->setToolTip("Copy Markdown");
    m_copyButton->setFixedSize(30, 30);
    m_copyButton->setCursor(Qt::PointingHandCursor);
    
    m_closeButton = new QPushButton("✕", card);
    m_closeButton->setObjectName("cancelBtn");
    m_closeButton->setFixedSize(30, 30);
    m_closeButton->setCursor(Qt::PointingHandCursor);
    
    topBarLayout->addWidget(m_copyButton);
    topBarLayout->addSpacing(4);
    topBarLayout->addWidget(m_closeButton);
    
    cardLayout->addLayout(topBarLayout);
    
    """

if start_idx != -1 and end_idx != -1:
    content = content[:start_idx] + new_constructor + content[end_idx:]

# 2. Update createToolbarBtn
btn_start = content.find('QPushButton* NoteEditorOverlay::createToolbarBtn')
btn_end = content.find('}', btn_start) + 1
new_btn = """QPushButton* NoteEditorOverlay::createToolbarBtn(const QString& text) {
    auto* btn = new QPushButton(text, this);
    btn->setProperty("isToolbarBtn", true);
    btn->setFixedSize(30, 30);
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}"""
if btn_start != -1:
    content = content[:btn_start] + new_btn + content[btn_end:]

# 3. Update onActiveNoteChanged style string
style_start = content.find('            // Step 4: Compute semi-transparent tint')
style_end = content.find('            // Step 5: Center the card')
new_style = """            // Step 4: Compute semi-transparent tint based on the note's tab accent color
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
                "QPushButton[isToolbarBtn=\\"true\\"] {"
                "    background-color: #27272a;"
                "    color: #d4d4d8;"
                "    border: 1px solid #3f3f46;"
                "    border-radius: 4px;"
                "    font-size: 14px;"
                "}"
                "QPushButton[isToolbarBtn=\\"true\\"]:hover {"
                "    background-color: #3f3f46;"
                "    color: white;"
                "    border: 1px solid %2;"
                "}"
                "QPushButton#btnBold { font-weight: bold; }"
                "QPushButton#btnItalic { font-style: italic; }"
                "QPushButton#btnUnder { text-decoration: underline; }"
                "QPushButton#btnStrike { text-decoration: line-through; }"
                "QPushButton#copyBtn {"
                "    background-color: transparent; border: none; border-radius: 4px;"
                "    image: url(\\"data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='%23ffffff' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><rect x='9' y='9' width='13' height='13' rx='2' ry='2'></rect><path d='M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1'></path></svg>\\");"
                "}"
                "QPushButton#copyBtn:hover {"
                "    background-color: #3f3f46;"
                "}"
                "QPushButton#cancelBtn {"
                "    background-color: transparent; color: #a1a1aa; border: none; font-weight: bold; font-size: 14px;"
                "}"
                "QPushButton#cancelBtn:hover { color: white; background-color: #ef4444; border-radius: 15px; }"
            ).arg(bgColor, noteOpt->color);
            
            QFrame* card = findChild<QFrame*>("sleekCard");
            if (card) {
                card->setStyleSheet(globalStyle);
            }
            
"""
if style_start != -1:
    content = content[:style_start] + new_style + content[style_end:]

with open(cpp_path, 'w') as f:
    f.write(content)
