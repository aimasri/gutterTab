/**
 * @file SleekSideChooserDialog.cpp
 * @brief Implementation of the screen docking edge chooser dialog.
 */

#include "SleekSideChooserDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QKeyEvent>

/**
 * @brief Constructs the SleekSideChooserDialog with Left and Right choice buttons.
 * @param parent Optional parent widget in the Qt object tree.
 */
SleekSideChooserDialog::SleekSideChooserDialog(QWidget* parent)
    : QDialog(parent, Qt::FramelessWindowHint | Qt::Dialog), m_selectedEdge(infrastructure::Config::Edge::Right)
{
    // Step 1: Configure modal frameless dialog attributes and fixed dimensions
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(300, 150);
    
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(16);

    // Step 2: Add header title and prompt text
    auto* titleLabel = new QLabel("Screen Edge", this);
    titleLabel->setStyleSheet("color: white; font-size: 16px; font-weight: bold;");
    layout->addWidget(titleLabel);

    auto* promptLabel = new QLabel("Which side of the screen would you like to dock this profile to?", this);
    promptLabel->setStyleSheet("color: #CCCCCC; font-size: 14px;");
    promptLabel->setWordWrap(true);
    layout->addWidget(promptLabel);
    
    // Step 3: Create horizontal action button row for Left and Right choices
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    
    auto* leftBtn = new QPushButton("Left", this);
    auto* rightBtn = new QPushButton("Right", this);
    
    QString btnStyle = 
        "QPushButton {"
        "  background-color: #333333;"
        "  color: white;"
        "  border: 1px solid #444444;"
        "  border-radius: 6px;"
        "  padding: 8px 16px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #444444;"
        "  border-color: #555555;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #222222;"
        "}";
        
    leftBtn->setStyleSheet(btnStyle);
    rightBtn->setStyleSheet(btnStyle);
    
    btnLayout->addWidget(leftBtn);
    btnLayout->addWidget(rightBtn);
    layout->addLayout(btnLayout);

    // Step 4: Wire button signals to record edge choice and accept dialog
    connect(leftBtn, &QPushButton::clicked, this, [this]() {
        m_selectedEdge = infrastructure::Config::Edge::Left;
        accept();
    });
    
    connect(rightBtn, &QPushButton::clicked, this, [this]() {
        m_selectedEdge = infrastructure::Config::Edge::Right;
        accept();
    });

    // Step 5: Apply dark-mode container frame styling
    setStyleSheet(
        "QDialog {"
        "  background-color: #1E1E1E;"
        "  border: 1px solid #333333;"
        "  border-radius: 8px;"
        "}"
    );
}

/**
 * @brief Intercepts Escape key to reject the dialog without applying changes.
 * @param event Key event details.
 */
void SleekSideChooserDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
    } else {
        QDialog::keyPressEvent(event);
    }
}

