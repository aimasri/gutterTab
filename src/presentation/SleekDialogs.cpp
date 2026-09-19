/**
 * @file SleekDialogs.cpp
 * @brief Implementation of sleek dark-mode dialogs, color pickers, and context menus.
 */

#include "SleekDialogs.h"

#include <QBoxLayout>
#include <QFrame>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

namespace {

/**
 * @brief Stylesheet rules for SleekInputDialog and SleekColorDialog cards and input fields.
 */
const char* const DIALOG_STYLE = R"(
    #dialogCard {
        background-color: #18181b;
        border: 1px solid #3f3f46;
        border-radius: 10px;
    }
    QLabel {
        color: #f4f4f5;
        font-family: sans-serif;
    }
    QLabel#titleLabel {
        font-size: 15px;
        font-weight: bold;
        color: #ffffff;
    }
    QLabel#subLabel {
        font-size: 12px;
        color: #a1a1aa;
    }
    QLineEdit {
        background-color: #09090b;
        border: 1px solid #27272a;
        border-radius: 6px;
        color: #f4f4f5;
        padding: 8px 12px;
        font-size: 13px;
        selection-background-color: #6366f1;
    }
    QLineEdit:focus {
        border: 1px solid #6366f1;
    }
    QPushButton#cancelBtn {
        background-color: #27272a;
        color: #d4d4d8;
        border: none;
        border-radius: 6px;
        padding: 7px 16px;
        font-size: 12px;
        font-weight: 600;
    }
    QPushButton#cancelBtn:hover {
        background-color: #3f3f46;
        color: #ffffff;
    }
    QPushButton#actionBtn {
        background-color: #6366f1;
        color: #ffffff;
        border: none;
        border-radius: 6px;
        padding: 7px 20px;
        font-size: 12px;
        font-weight: 600;
    }
    QPushButton#actionBtn:hover {
        background-color: #4f46e5;
    }
)";

/**
 * @brief Stylesheet rules for SleekContextMenu items and popup border container.
 */
const char* const MENU_STYLE = R"(
    QMenu {
        background-color: #18181b;
        border: 1px solid #3f3f46;
        border-radius: 8px;
        padding: 6px;
    }
    QMenu::item {
        background-color: transparent;
        color: #f4f4f5;
        padding: 7px 18px 7px 10px;
        margin: 2px 0px;
        border-radius: 5px;
        font-family: sans-serif;
        font-size: 13px;
        font-weight: 500;
    }
    QMenu::item:selected {
        background-color: #27272a;
        color: #ffffff;
    }
    QMenu::separator {
        height: 1px;
        background-color: #27272a;
        margin: 5px 6px;
    }
)";
} // namespace

/**
 * @brief Returns the curated 24-color dark-mode palette.
 */
const QVector<QString>& getSleekDarkPalette() {
    static const QVector<QString> s_palette = {
        // Row 1: Blues (Deep -> Mid -> Bright Accents)
        QStringLiteral("#172554"), QStringLiteral("#1e3a5f"), QStringLiteral("#284b63"),
        QStringLiteral("#2563eb"), QStringLiteral("#38bdf8"), QStringLiteral("#06b6d4"),

        // Row 2: Greens (Deep -> Mid -> Bright Accents)
        QStringLiteral("#14532d"), QStringLiteral("#134e4a"), QStringLiteral("#0f766e"),
        QStringLiteral("#16a34a"), QStringLiteral("#10b981"), QStringLiteral("#4ade80"),

        // Row 3: Reds (Deep -> Mid -> Bright Accents)
        QStringLiteral("#5c1d24"), QStringLiteral("#7f1d1d"), QStringLiteral("#991b1b"),
        QStringLiteral("#dc2626"), QStringLiteral("#f43f5e"), QStringLiteral("#fb7185"),

        // Row 4: Oranges & Ambers (Deep -> Mid -> Bright Accents)
        QStringLiteral("#7c2d12"), QStringLiteral("#9a3412"), QStringLiteral("#c2410c"),
        QStringLiteral("#ea580c"), QStringLiteral("#fb923c"), QStringLiteral("#ffb74d")
    };
    return s_palette;
}

/**
 * @brief Constructs a SleekContextMenu with frameless popup flags and dark styling.
 * @param parent Optional parent widget.
 */
SleekContextMenu::SleekContextMenu(QWidget* parent)
    : QMenu(parent) {
    // Step 1: Configure popup window flags to suppress window manager decorations and shadows
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    
    // Step 2: Apply custom dark menu stylesheet
    setStyleSheet(MENU_STYLE);
}

/**
 * @brief Constructs the SleekInputDialog dialog card and controls.
 * @param title Header title.
 * @param labelText Optional sub-label description.
 * @param initialValue Initial text in input field.
 * @param parent Optional parent widget.
 */
SleekInputDialog::SleekInputDialog(
    const QString& title,
    const QString& labelText,
    const QString& initialValue,
    QWidget* parent
) : QDialog(parent) {
    // Step 1: Set modal window flags, translucent background, fixed width, and stylesheet
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(380);
    setStyleSheet(DIALOG_STYLE);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    // Step 2: Create central card frame
    auto* card = new QFrame(this);
    card->setObjectName("dialogCard");
    rootLayout->addWidget(card);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(14);

    // Step 3: Add title and optional descriptive sub-label
    auto* titleLbl = new QLabel(title, card);
    titleLbl->setObjectName("titleLabel");
    cardLayout->addWidget(titleLbl);

    if (!labelText.isEmpty()) {
        auto* subLbl = new QLabel(labelText, card);
        subLbl->setObjectName("subLabel");
        cardLayout->addWidget(subLbl);
    }

    // Step 4: Add input line edit with pre-selected text
    m_lineEdit = new QLineEdit(initialValue, card);
    m_lineEdit->selectAll();
    cardLayout->addWidget(m_lineEdit);

    // Step 5: Add Cancel and Save action buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->addStretch(1);

    auto* cancelBtn = new QPushButton("Cancel", card);
    cancelBtn->setObjectName("cancelBtn");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    auto* saveBtn = new QPushButton("Save", card);
    saveBtn->setObjectName("actionBtn");
    connect(saveBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(saveBtn);

    cardLayout->addLayout(btnLayout);
    m_lineEdit->setFocus();
}

/**
 * @brief Returns the trimmed text string currently entered in the line edit.
 */
QString SleekInputDialog::value() const {
    return m_lineEdit ? m_lineEdit->text().trimmed() : QString();
}

/**
 * @brief Handles Enter/Return to accept and Escape to reject the input dialog.
 * @param event Key event details.
 */
void SleekInputDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        accept();
    } else {
        QDialog::keyPressEvent(event);
    }
}

/**
 * @brief Constructs the SleekColorDialog with palette swatch buttons, hex input, and live preview.
 * @param initialColor Pre-selected color.
 * @param parent Optional parent widget.
 */
SleekColorDialog::SleekColorDialog(const QColor& initialColor, QWidget* parent)
    : QDialog(parent),
      m_color(initialColor.isValid() ? initialColor : QColor("#2563eb")) {

    // Step 1: Configure modal frameless dialog attributes
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(340);
    setStyleSheet(DIALOG_STYLE);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    // Step 2: Build card container
    auto* card = new QFrame(this);
    card->setObjectName("dialogCard");
    rootLayout->addWidget(card);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(14);

    auto* titleLbl = new QLabel("Color", card);
    titleLbl->setObjectName("titleLabel");
    cardLayout->addWidget(titleLbl);

    // Step 3: Populate 6-column swatch palette grid
    auto* gridLayout = new QGridLayout();
    gridLayout->setSpacing(8);
    gridLayout->setAlignment(Qt::AlignCenter);

    const auto& palette = getSleekDarkPalette();
    for (int i = 0; i < palette.size(); ++i) {
        QColor c(palette[i]);
        auto* swatchBtn = new QPushButton(card);
        swatchBtn->setFixedSize(28, 28);
        swatchBtn->setCursor(Qt::PointingHandCursor);
        swatchBtn->setStyleSheet(QString(
            "QPushButton {"
            "    background-color: %1;"
            "    border: 1px solid rgba(255, 255, 255, 0.2);"
            "    border-radius: 5px;"
            "}"
            "QPushButton:hover {"
            "    border: 2px solid #ffffff;"
            "}"
        ).arg(palette[i]));
        connect(swatchBtn, &QPushButton::clicked, this, [this, c]() {
            onSwatchClicked(c);
        });
        gridLayout->addWidget(swatchBtn, i / 6, i % 6);
    }
    cardLayout->addLayout(gridLayout);

    // Step 4: Hex input line edit with regex validator
    auto* hexLayout = new QHBoxLayout();
    auto* hexLabel = new QLabel("Hex:", card);
    hexLabel->setObjectName("subLabel");
    hexLayout->addWidget(hexLabel);

    m_hexEdit = new QLineEdit(m_color.name(QColor::HexRgb).toUpper(), card);
    QRegularExpression hexRegex("^#?[0-9A-Fa-f]{6}$");
    m_hexEdit->setValidator(new QRegularExpressionValidator(hexRegex, m_hexEdit));
    connect(m_hexEdit, &QLineEdit::textChanged, this, &SleekColorDialog::onHexEdited);
    hexLayout->addWidget(m_hexEdit);
    cardLayout->addLayout(hexLayout);

    // Step 5: Live preview swatch widget
    m_previewSwatch = new QWidget(card);
    m_previewSwatch->setFixedHeight(40);
    m_previewSwatch->setStyleSheet("border-radius: 6px;");
    cardLayout->addWidget(m_previewSwatch);

    updatePreview();

    // Step 6: Dialog action buttons (Cancel / Apply)
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->addStretch(1);

    auto* cancelBtn = new QPushButton("Cancel", card);
    cancelBtn->setObjectName("cancelBtn");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    auto* applyBtn = new QPushButton("Apply", card);
    applyBtn->setObjectName("actionBtn");
    connect(applyBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(applyBtn);

    cardLayout->addLayout(btnLayout);
}

/**
 * @brief Updates selected color and syncs hex field when a palette swatch is clicked.
 * @param color Color associated with the clicked swatch.
 */
void SleekColorDialog::onSwatchClicked(const QColor& color) {
    m_color = color;
    if (m_hexEdit) {
        m_hexEdit->setText(m_color.name(QColor::HexRgb).toUpper());
    }
    updatePreview();
}

/**
 * @brief Validates typed hex text, updates color state, and refreshes the preview.
 * @param text Typed hex color string.
 */
void SleekColorDialog::onHexEdited(const QString& text) {
    QString hex = text.trimmed();
    // Step 1: Prepend '#' if user typed raw 6 hex digits without prefix
    if (!hex.startsWith('#')) {
        hex.prepend('#');
    }
    // Step 2: Validate color string and update state
    if (QColor::isValidColorName(hex)) {
        m_color = QColor(hex);
        updatePreview();
    }
}

/**
 * @brief Updates the live preview swatch widget with the current color's stylesheet.
 */
void SleekColorDialog::updatePreview() {
    if (m_previewSwatch) {
        m_previewSwatch->setStyleSheet(QString(
            "background-color: %1; border: 1px solid rgba(255, 255, 255, 0.2); border-radius: 6px;"
        ).arg(m_color.name(QColor::HexArgb)));
    }
}

/**
 * @brief Returns the chosen color.
 */
QColor SleekColorDialog::selectedColor() const {
    return m_color;
}

