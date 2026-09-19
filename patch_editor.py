import re

cpp_path = 'src/presentation/NoteEditorOverlay.cpp'
with open(cpp_path, 'r') as f:
    content = f.read()

# Replace Constructor
start_idx = content.find('NoteEditorOverlay::NoteEditorOverlay')
end_idx = content.find('// Step 6: Connect signals to toolbar slots')

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
    mainLayout->addWidget(card);
    
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
    
    auto* btnH1 = createToolbarBtn("H1");
    auto* btnH2 = createToolbarBtn("H2");
    auto* btnH3 = createToolbarBtn("H3");
    
    auto* btnBullet = createToolbarBtn("•");
    auto* btnNum = createToolbarBtn("1.");
    
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
    
    """

if start_idx != -1 and end_idx != -1:
    content = content[:start_idx] + new_constructor + content[end_idx:]
    with open(cpp_path, 'w') as f:
        f.write(content)
else:
    print("Failed to replace constructor")
