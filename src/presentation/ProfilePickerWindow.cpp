/**
 * @file ProfilePickerWindow.cpp
 * @brief Implementation of ProfilePickerWindow and ProfileCard presentation components.
 */

#include "ProfilePickerWindow.h"
#include "../infrastructure/ConfigManager.h"
#include "SleekDialogs.h"
#include "SleekSideChooserDialog.h"

#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QPainter>
#include <QMouseEvent>
#include <QMessageBox>
#include <QMenu>
#include <QStyle>
#include <QKeyEvent>

namespace {

/**
 * @brief Visual tile widget displaying a single profile entry in the picker grid.
 * 
 * @details ProfileCard renders an interactive card containing a procedurally painted
 *          circular avatar with the profile's accent color and display name initial.
 *          It provides hover feedback via dark-mode CSS borders and intercepts mouse
 *          events to emit either left-click selection or right-click context menu requests.
 * 
 * @note Internal helper component scoped to the ProfilePickerWindow translation unit.
 */
class ProfileCard : public QFrame {
    Q_OBJECT
public:
    /**
     * @brief Constructs a ProfileCard widget.
     * @param info Metadata containing profile identifier, name, and accent color.
     * @param parent Optional parent widget.
     */
    ProfileCard(const infrastructure::ProfileInfo& info, QWidget* parent = nullptr)
        : QFrame(parent), m_info(info) {
        setFixedSize(140, 180);
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(
            "ProfileCard {"
            "  background-color: #18181b;"
            "  border: 1px solid #27272a;"
            "  border-radius: 8px;"
            "}"
            "ProfileCard:hover {"
            "  border: 1px solid #6366f1;"
            "  background-color: #27272a;"
            "}"
        );

        auto* layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignCenter);
        layout->setSpacing(12);

        // Step 1: Render avatar circle with profile accent color and initial letter
        auto* avatarLabel = new QLabel(this);
        avatarLabel->setFixedSize(64, 64);
        
        QPixmap avatar(64, 64);
        avatar.fill(Qt::transparent);
        QPainter p(&avatar);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(info.accentColor);
        p.setPen(Qt::NoPen);
        p.drawEllipse(0, 0, 64, 64);
        
        p.setPen(Qt::white);
        QFont font = p.font();
        font.setPointSize(24);
        font.setBold(true);
        p.setFont(font);
        QString initial = info.displayName.isEmpty() ? "?" : info.displayName.left(1).toUpper();
        p.drawText(avatar.rect(), Qt::AlignCenter, initial);
        p.end();
        
        avatarLabel->setPixmap(avatar);
        layout->addWidget(avatarLabel, 0, Qt::AlignCenter);

        // Step 2: Render profile display name
        auto* nameLabel = new QLabel(info.displayName, this);
        nameLabel->setStyleSheet("color: white; font-weight: bold; font-size: 14px;");
        nameLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(nameLabel, 0, Qt::AlignCenter);
    }

    /**
     * @brief Returns immutable reference to the profile metadata represented by this card.
     * @return ProfileInfo struct.
     */
    const infrastructure::ProfileInfo& info() const { return m_info; }

protected:
    /**
     * @brief Handles mouse release to trigger selection on left-click or context menu on right-click.
     * @param event Mouse event details.
     */
    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            emit clicked(m_info.id);
        } else if (event->button() == Qt::RightButton) {
            emit rightClicked(m_info.id, event->globalPosition().toPoint());
        }
    }
    
signals:
    /**
     * @brief Emitted when the card is left-clicked.
     * @param id Profile identifier.
     */
    void clicked(const QString& id);

    /**
     * @brief Emitted when the card is right-clicked.
     * @param id Profile identifier.
     * @param pos Global screen coordinates for context menu anchoring.
     */
    void rightClicked(const QString& id, const QPoint& pos);
    
private:
    infrastructure::ProfileInfo m_info; ///< Metadata for this profile.
};

} // namespace

/**
 * @brief Constructs the ProfilePickerWindow dialog and populates profile tiles.
 * @param parent Optional parent widget.
 */
ProfilePickerWindow::ProfilePickerWindow(QWidget* parent)
    : QWidget(parent) {
    
    // Step 1: Set frameless and stay-on-top flags for sleek floating appearance
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(700, 500);

    // Step 2: Build visual skeleton and populate cards from profile registry
    buildUI();
    rebuildProfileCards();
}

/**
 * @brief Assembles the window frames, header typography, cards container, and footer controls.
 */
void ProfilePickerWindow::buildUI() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    // Step 1: Dark outer card frame
    auto* backgroundFrame = new QFrame(this);
    backgroundFrame->setStyleSheet(
        "QFrame#bgFrame {"
        "  background-color: #09090b;"
        "  border: 1px solid #27272a;"
        "  border-radius: 12px;"
        "}"
    );
    backgroundFrame->setObjectName("bgFrame");
    rootLayout->addWidget(backgroundFrame);

    auto* mainLayout = new QVBoxLayout(backgroundFrame);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    mainLayout->setSpacing(20);

    // Step 2: Header layout
    auto* headerLayout = new QHBoxLayout();
    auto* logoLabel = new QLabel(this);
    headerLayout->addWidget(logoLabel);
    
    auto* titleLabel = new QLabel("Which Profile?", this);
    titleLabel->setStyleSheet("color: white; font-size: 24px; font-weight: bold;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    mainLayout->addLayout(headerLayout);
    mainLayout->addSpacing(10);

    // Step 3: Frameless window close button ("✕") positioned in top-right corner
    auto* closeBtn = new QPushButton("✕", backgroundFrame);
    closeBtn->setFixedSize(32, 32);
    closeBtn->move(700 - 32 - 12, 12); // Top right corner offset
    closeBtn->setStyleSheet(
        "QPushButton { background: transparent; color: #a1a1aa; font-size: 16px; border: none; font-weight: bold; }"
        "QPushButton:hover { color: white; background: #ef4444; border-radius: 16px; }"
    );
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);

    // Step 4: Central profile cards grid container
    m_cardsContainer = new QWidget(this);
    m_cardsLayout = new QGridLayout(m_cardsContainer);
    m_cardsLayout->setSpacing(20);
    m_cardsLayout->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_cardsContainer, 1);

    // Step 5: Footer bar with auto-launch checkbox and "+ Add Profile" action
    auto* bottomLayout = new QHBoxLayout();
    m_autoLaunchCheck = new QCheckBox("Always use selected profile on startup", this);
    m_autoLaunchCheck->setStyleSheet(
        "QCheckBox { color: #a1a1aa; font-size: 13px; }"
        "QCheckBox::indicator { width: 18px; height: 18px; border-radius: 4px; border: 1px solid #3f3f46; background: #18181b; }"
        "QCheckBox::indicator:checked { image: url(data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='%236366f1' stroke-width='4' stroke-linecap='round' stroke-linejoin='round'><path d='M18 6 6 18'/><path d='m6 6 12 12'/></svg>); }"
    );
    bottomLayout->addWidget(m_autoLaunchCheck);
    bottomLayout->addStretch();
    
    auto* addProfileBtn = new QPushButton("+ Add Profile", this);
    addProfileBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: transparent;"
        "  color: #6366f1;"
        "  border: 1px solid #6366f1;"
        "  border-radius: 6px;"
        "  padding: 8px 16px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(99, 102, 241, 0.1);"
        "}"
    );
    connect(addProfileBtn, &QPushButton::clicked, this, &ProfilePickerWindow::onAddProfileClicked);
    bottomLayout->addWidget(addProfileBtn);

    mainLayout->addLayout(bottomLayout);
}

/**
 * @brief Refreshes the grid of profile cards by querying ConfigManager.
 */
void ProfilePickerWindow::rebuildProfileCards() {
    // Step 1: Clear existing cards from grid layout
    QLayoutItem* item;
    while ((item = m_cardsLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    // Step 2: Fetch all profiles from registry
    auto profiles = infrastructure::ConfigManager::listProfiles();
    int row = 0;
    int col = 0;
    const int maxCols = 3;

    // Step 3: Populate cards in a 3-column grid
    for (const auto& p : profiles) {
        QWidget* card = createProfileCard(p);
        m_cardsLayout->addWidget(card, row, col);
        
        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }
}

/**
 * @brief Instantiates a ProfileCard and wires selection and context menu events.
 * @param profile Metadata for the profile.
 * @return Pointer to the configured ProfileCard widget.
 */
QWidget* ProfilePickerWindow::createProfileCard(const infrastructure::ProfileInfo& profile) {
    auto* card = new ProfileCard(profile, this);
    
    // Step 1: Connect left-click to profile activation
    connect(card, &ProfileCard::clicked, this, &ProfilePickerWindow::onProfileCardClicked);
    
    // Step 2: Connect right-click to custom context menu
    connect(card, &ProfileCard::rightClicked, this, [this](const QString& id, const QPoint& pos) {
        SleekContextMenu menu(this);
        
        auto* renameAction = menu.addAction("Rename Profile...");
        auto* colorAction = menu.addAction("Change Color...");
        menu.addSeparator();
        auto* deleteAction = menu.addAction("Delete Profile");
        
        // Step 3: Guardrail: Prevent deleting the only remaining profile
        if (infrastructure::ConfigManager::listProfiles().size() <= 1) {
            deleteAction->setEnabled(false);
        }

        QAction* selected = menu.exec(pos);
        if (selected == renameAction) {
            onEditProfileName(id);
        } else if (selected == colorAction) {
            onEditProfileColor(id);
        } else if (selected == deleteAction) {
            onDeleteProfile(id);
        }
    });

    return card;
}

/**
 * @brief Handles card click by recording selection, persisting auto-launch choice, and emitting signal.
 * @param profileId Identifier of the selected profile.
 */
void ProfilePickerWindow::onProfileCardClicked(const QString& profileId) {
    m_selected = true;
    if (m_autoLaunchCheck->isChecked()) {
        infrastructure::ConfigManager::setAutoLaunchProfile(profileId);
    }
    emit profileSelected(profileId);
}

/**
 * @brief Orchestrates profile creation through sequential dialog prompts.
 */
void ProfilePickerWindow::onAddProfileClicked() {
    // Step 1: Request profile display name
    SleekInputDialog nameDialog("Add Profile", "Profile Name:", "", this);
    if (nameDialog.exec() == QDialog::Accepted) {
        QString name = nameDialog.value().trimmed();
        if (!name.isEmpty()) {
            // Step 2: Request profile accent color
            SleekColorDialog colorDialog(QColor("#4285F4"), this);
            if (colorDialog.exec() == QDialog::Accepted) {
                // Step 3: Request screen edge dock side (Left or Right)
                SleekSideChooserDialog sideDialog(this);
                if (sideDialog.exec() == QDialog::Accepted) {
                    // Step 4: Persist new profile in registry
                    infrastructure::ConfigManager::createProfile(name, colorDialog.selectedColor());
                    
                    auto profiles = infrastructure::ConfigManager::listProfiles();
                    QString newId = profiles.last().id;
                    
                    // Step 5: Save edge preference to the new profile's config
                    infrastructure::ConfigManager::instance().mutableConfig().edge = sideDialog.selectedEdge();
                    infrastructure::ConfigManager::instance().save(newId);
                    
                    // Step 6: Rebuild card grid to include the newly created profile
                    rebuildProfileCards();
                }
            }
        }
    }
}

/**
 * @brief Prompts for confirmation and deletes the specified profile directory.
 * @param profileId Identifier of the profile to delete.
 */
void ProfilePickerWindow::onDeleteProfile(const QString& profileId) {
    // Step 1: Present confirmation message box with dark theme styling
    QMessageBox msgBox(this);
    msgBox.setStyleSheet("QMessageBox { background-color: #18181b; color: white; } QLabel { color: white; } QPushButton { background-color: #27272a; color: white; padding: 6px 12px; border: 1px solid #3f3f46; border-radius: 4px; } QPushButton:hover { background-color: #3f3f46; }");
    msgBox.setText(QString("Are you sure you want to delete profile '%1'?\nThis will delete all its decks and settings permanently.").arg(profileId));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    if (msgBox.exec() == QMessageBox::Yes) {
        // Step 2: Remove profile files via ConfigManager and refresh UI
        infrastructure::ConfigManager::deleteProfile(profileId);
        rebuildProfileCards();
    }
}

/**
 * @brief Prompts user to rename a profile and persists changes.
 * @param profileId Identifier of the profile to rename.
 */
void ProfilePickerWindow::onEditProfileName(const QString& profileId) {
    // Step 1: Find current display name
    auto profiles = infrastructure::ConfigManager::listProfiles();
    QString currentName;
    for (const auto& p : profiles) {
        if (p.id == profileId) currentName = p.displayName;
    }
    
    // Step 2: Prompt for new name with SleekInputDialog
    SleekInputDialog dialog("Rename Profile", "New Name:", currentName, this);
    dialog.adjustSize();
    dialog.move(geometry().center() - dialog.rect().center());
    if (dialog.exec() == QDialog::Accepted) {
        QString name = dialog.value();
        if (!name.isEmpty() && name != currentName) {
            // Step 3: Persist updated name and refresh cards
            infrastructure::ConfigManager::renameProfile(profileId, name);
            rebuildProfileCards();
        }
    }
}

/**
 * @brief Prompts user to change a profile accent color and persists changes.
 * @param profileId Identifier of the profile to recolor.
 */
void ProfilePickerWindow::onEditProfileColor(const QString& profileId) {
    // Step 1: Find current accent color
    auto profiles = infrastructure::ConfigManager::listProfiles();
    QColor currentColor;
    for (const auto& p : profiles) {
        if (p.id == profileId) currentColor = p.accentColor;
    }
    
    // Step 2: Prompt for new color with SleekColorDialog
    SleekColorDialog dialog(currentColor, this);
    dialog.adjustSize();
    dialog.move(geometry().center() - dialog.rect().center());
    if (dialog.exec() == QDialog::Accepted) {
        // Step 3: Persist updated accent color and refresh cards
        infrastructure::ConfigManager::updateProfileColor(profileId, dialog.selectedColor());
        rebuildProfileCards();
    }
}

/**
 * @brief Emits closedWithoutSelection if the window closes without picking a profile.
 * @param event Close event details.
 */
void ProfilePickerWindow::closeEvent(QCloseEvent* event) {
    if (!m_selected) {
        emit closedWithoutSelection();
    }
    QWidget::closeEvent(event);
}

/**
 * @brief Handles Escape key press to close the window.
 * @param event Key event details.
 */
void ProfilePickerWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        close();
    } else {
        QWidget::keyPressEvent(event);
    }
}

/**
 * @brief Captures mouse click offset to initiate frameless window dragging.
 * @param event Mouse event details.
 */
void ProfilePickerWindow::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

/**
 * @brief Updates window position as the user drags with the mouse.
 * @param event Mouse event details.
 */
void ProfilePickerWindow::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

/**
 * @brief Centers the dialog on the active display monitor when shown.
 * @param event Show event details.
 */
void ProfilePickerWindow::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        move(screenGeometry.center() - rect().center());
    }
}

#include "ProfilePickerWindow.moc"

