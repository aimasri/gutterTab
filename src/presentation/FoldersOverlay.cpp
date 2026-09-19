#include "FoldersOverlay.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGuiApplication>
#include <QScreen>
#include <QScrollArea>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>
#include "SleekDialogs.h"

namespace presentation {

FoldersOverlay::FoldersOverlay(domain::TabController* controller, QWidget* parent)
    : QWidget(nullptr, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint), m_controller(controller) {
    
    setFixedSize(600, 500);
    setAttribute(Qt::WA_TranslucentBackground);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    m_cardFrame = new QFrame(this);
    m_cardFrame->setObjectName("sleekCard");
    m_cardFrame->setAttribute(Qt::WA_StyledBackground, true);
    
    QVBoxLayout* cardLayout = new QVBoxLayout(m_cardFrame);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(12);
    
    // Top Bar
    QHBoxLayout* topBarLayout = new QHBoxLayout();
    
    QLabel* titleLbl = new QLabel("Folder Shortcuts", this);
    titleLbl->setStyleSheet("color: #e4e4e7; font-size: 18px; font-weight: bold; font-family: sans-serif;");
    topBarLayout->addWidget(titleLbl);
    topBarLayout->addStretch();
    
    m_addBtn = createToolbarBtn("+");
    m_addBtn->setObjectName("addBtn");
    m_addBtn->setToolTip("Add Folder");
    
    m_closeButton = createToolbarBtn("✕");
    m_closeButton->setObjectName("cancelBtn");
    m_closeButton->setToolTip("Close");
    
    topBarLayout->addWidget(m_addBtn);
    topBarLayout->addWidget(m_closeButton);
    cardLayout->addLayout(topBarLayout);
    
    // Scroll Area
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    
    m_scrollAreaWidget = new QWidget();
    m_scrollAreaWidget->setStyleSheet("background: transparent;");
    m_gridLayout = new QGridLayout(m_scrollAreaWidget);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setSpacing(16);
    
    // Push everything to the top
    m_gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    
    scrollArea->setWidget(m_scrollAreaWidget);
    cardLayout->addWidget(scrollArea);
    mainLayout->addWidget(m_cardFrame);
    
    // Connect actions
    connect(m_closeButton, &QPushButton::clicked, this, &FoldersOverlay::onCloseClicked);
    connect(m_addBtn, &QPushButton::clicked, this, &FoldersOverlay::onAddFolderClicked);
    
    connect(m_controller, &domain::TabController::stateChanged, this, &FoldersOverlay::onStateChanged);
    connect(m_controller, &domain::TabController::foldersLoaded, this, &FoldersOverlay::onFoldersLoaded);
    
    installEventFilter(this);
    
    QString globalStyle =
        "QFrame#sleekCard {"
        "    background-color: rgba(24, 24, 27, 0.98);"
        "    border: 1px solid #3f3f46;"
        "    border-radius: 10px;"
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
        "}"
        "QPushButton#cancelBtn {"
        "    background-color: transparent; color: #a1a1aa; border: none; font-weight: bold; font-size: 14px;"
        "}"
        "QPushButton#cancelBtn:hover { color: white; background-color: #ef4444; border-radius: 15px; }";
    
    m_cardFrame->setStyleSheet(globalStyle);
    hide();
}

FoldersOverlay::~FoldersOverlay() {}

QPushButton* FoldersOverlay::createToolbarBtn(const QString& text) {
    auto* btn = new QPushButton(text, this);
    btn->setProperty("isToolbarBtn", "true");
    btn->setFixedSize(30, 30);
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

void FoldersOverlay::mousePressEvent(QMouseEvent* event) {
    event->accept();
}

bool FoldersOverlay::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            if (m_controller->currentState() == domain::TabController::State::FOLDERS) {
                m_controller->toggleFolders();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void FoldersOverlay::onCloseClicked() {
    m_controller->toggleFolders();
}

void FoldersOverlay::onStateChanged(domain::TabController::State state) {
    if (state == domain::TabController::State::FOLDERS) {
        if (QScreen* screen = QGuiApplication::primaryScreen()) {
            move(screen->geometry().center() - rect().center());
        }
        m_controller->loadFolders();
        show();
        activateWindow();
    } else {
        hide();
    }
}

void FoldersOverlay::onFoldersLoaded() {
    buildGrid();
}

void FoldersOverlay::buildGrid() {
    // Clear layout
    QLayoutItem* child;
    while ((child = m_gridLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->hide();
            child->widget()->deleteLater();
        }
        delete child;
    }
    
    const auto& folders = m_controller->folders();
    int cols = 4;
    for (int i = 0; i < folders.size(); ++i) {
        const auto& f = folders[i];
        
        QPushButton* folderBtn = new QPushButton(m_scrollAreaWidget);
        folderBtn->setFixedSize(120, 100);
        folderBtn->setCursor(Qt::PointingHandCursor);
        
        // Setup Icon and text
        QString style = "QPushButton { background: #27272a; border: 1px solid #3f3f46; border-radius: 8px; color: #e4e4e7; font-family: sans-serif; font-size: 12px; } "
                        "QPushButton:hover { background: #3f3f46; border: 1px solid #52525b; }";
        folderBtn->setStyleSheet(style);
        
        QVBoxLayout* btnLayout = new QVBoxLayout(folderBtn);
        btnLayout->setAlignment(Qt::AlignCenter);
        
        QLabel* iconLbl = new QLabel(folderBtn);
        iconLbl->setAlignment(Qt::AlignCenter);
        if (!f.iconPath.isEmpty() && QFile::exists(f.iconPath)) {
            QPixmap pix(f.iconPath);
            iconLbl->setPixmap(pix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            // Text fallback icon
            iconLbl->setText("📁");
            iconLbl->setStyleSheet("font-size: 32px; background: transparent; border: none;");
        }
        
        QLabel* nameLbl = new QLabel(f.name, folderBtn);
        nameLbl->setAlignment(Qt::AlignCenter);
        nameLbl->setStyleSheet("background: transparent; border: none; color: #e4e4e7;");
        nameLbl->setWordWrap(true);
        
        btnLayout->addWidget(iconLbl);
        btnLayout->addWidget(nameLbl);
        
        int row = i / cols;
        int col = i % cols;
        m_gridLayout->addWidget(folderBtn, row, col);
        
        connect(folderBtn, &QPushButton::clicked, [f]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(f.path));
        });
        
        // Custom Context Menu
        folderBtn->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(folderBtn, &QWidget::customContextMenuRequested, [this, folderBtn, f](const QPoint& pos) {
            SleekContextMenu* menu = new SleekContextMenu(folderBtn);
            
            QAction* editAction = menu->addAction("Edit Shortcut");
            connect(editAction, &QAction::triggered, [this, f]() {
                onEditShortcut(f.id);
            });
            
            QAction* delAction = menu->addAction("Delete Shortcut");
            connect(delAction, &QAction::triggered, [this, f]() {
                onDeleteShortcut(f.id);
            });
            
            menu->exec(folderBtn->mapToGlobal(pos));
        });
    }
}

void FoldersOverlay::onAddFolderClicked() {
    SleekInputDialog nameDialog("New Folder Shortcut", "Enter shortcut name:", "", this);
    if (nameDialog.exec() != QDialog::Accepted || nameDialog.value().trimmed().isEmpty()) return;
    QString name = nameDialog.value().trimmed();
    
    QString path = QFileDialog::getExistingDirectory(this, "Select Directory", QDir::homePath());
    if (path.isEmpty()) return;
    
    QString iconPath = QFileDialog::getOpenFileName(this, "Select Custom Icon (Optional)", QDir::homePath(), "Images (*.png *.xpm *.jpg *.svg *.bmp)");
    
    m_controller->createFolderShortcut(name, path, iconPath);
}

void FoldersOverlay::onEditShortcut(int id) {
    domain::FolderShortcut target;
    bool found = false;
    for (const auto& f : m_controller->folders()) {
        if (f.id == id) {
            target = f;
            found = true;
            break;
        }
    }
    if (!found) return;
    
    SleekInputDialog nameDialog("Edit Shortcut", "Name:", target.name, this);
    if (nameDialog.exec() != QDialog::Accepted || nameDialog.value().trimmed().isEmpty()) return;
    QString name = nameDialog.value().trimmed();
    
    QString path = QFileDialog::getExistingDirectory(this, "Select Directory", target.path);
    if (path.isEmpty()) return; // Assume cancel
    
    QString iconPath = QFileDialog::getOpenFileName(this, "Select Custom Icon", target.iconPath.isEmpty() ? QDir::homePath() : QFileInfo(target.iconPath).absolutePath(), "Images (*.png *.xpm *.jpg *.svg *.bmp)");
    // Empty iconPath is valid if they cleared it, but getOpenFileName returns empty on cancel. Let's just ask if they want to keep the old one if empty?
    // Actually getOpenFileName gives "" on cancel. We might overwrite a valid icon.
    // Instead of forcing icon picking, let's keep it simple: if empty, keep old. To clear, they can delete and recreate.
    if (iconPath.isEmpty()) {
        iconPath = target.iconPath;
    }
    
    m_controller->updateFolderShortcut(id, name, path, iconPath);
}

void FoldersOverlay::onDeleteShortcut(int id) {
    m_controller->deleteFolderShortcut(id);
}

} // namespace presentation
