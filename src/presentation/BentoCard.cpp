#include "BentoCard.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QFile>

namespace presentation {

BentoCard::BentoCard(const domain::Note& note, QWidget* parent) 
    : QWidget(parent), m_note(note) 
{
    setMouseTracking(true);
    
    // Extract first image from markdown
    QRegularExpression regex("!\\[.*?\\]\\((.*?)\\)");
    auto match = regex.match(m_note.content);
    if (match.hasMatch()) {
        QString path = match.captured(1);
        if (path.startsWith("file://")) {
            path = path.mid(7); // Remove file://
        }
        if (QFile::exists(path)) {
            m_attachmentPath = path;
            QPixmap fullPix(path);
            if (!fullPix.isNull()) {
                m_thumbnail = fullPix.scaled(800, 800, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            }
        }
    }
    
    m_geometryAnim = new QPropertyAnimation(this, "geometry", this);
    m_geometryAnim->setDuration(400);
    m_geometryAnim->setEasingCurve(QEasingCurve::OutExpo);
}

void BentoCard::setTargetGeometry(const QRect& rect) {
    m_targetGeometry = rect;
}

void BentoCard::animateToTarget() {
    m_geometryAnim->stop();
    m_geometryAnim->setStartValue(geometry());
    m_geometryAnim->setEndValue(m_targetGeometry);
    m_geometryAnim->start();
}

void BentoCard::animateToOrigin(const QRect& origin) {
    m_geometryAnim->stop();
    m_geometryAnim->setStartValue(geometry());
    m_geometryAnim->setEndValue(origin);
    m_geometryAnim->start();
}

void BentoCard::enterEvent(QEnterEvent* event) {
    Q_UNUSED(event);
    m_hovered = true;
    update();
}

void BentoCard::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    m_hovered = false;
    update();
}

void BentoCard::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_note.id);
    }
}

void BentoCard::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    int r = 16;
    path.addRoundedRect(rect(), r, r);
    
    // Draw background
    QColor baseColor(m_note.color);
    if (m_hovered) {
        baseColor = baseColor.lighter(110);
    }
    painter.fillPath(path, baseColor);
    
    painter.setClipPath(path);

    // If there's an image, draw it as a faint background or top half
    if (!m_thumbnail.isNull()) {
        QPainterPath imgPath;
        imgPath.addRoundedRect(rect().adjusted(4, 4, -4, -4), r-4, r-4);
        painter.setClipPath(imgPath);
        
        // Draw image covering the whole rect, with some opacity
        painter.setOpacity(0.3);
        
        int xOffset = (rect().width() - m_thumbnail.width()) / 2;
        int yOffset = (rect().height() - m_thumbnail.height()) / 2;
        painter.drawPixmap(xOffset, yOffset, m_thumbnail);
        
        painter.setOpacity(1.0);
    }
    
    painter.setClipPath(path);

    // Draw Title
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(16);
    font.setBold(true);
    painter.setFont(font);
    
    QRect textRect = rect().adjusted(20, 20, -20, -20);
    painter.drawText(textRect, Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap, m_note.title);
    
    // Draw Text Preview
    font.setPointSize(10);
    font.setBold(false);
    painter.setFont(font);
    
    // Strip markdown formatting simple attempt
    QString plainText = m_note.content;
    plainText.replace(QRegularExpression("[#*`>\\[\\]]"), "");
    plainText.replace(QRegularExpression("\\(.*\\)"), ""); // remove links
    
    QRect previewRect = textRect.adjusted(0, 30, 0, 0);
    painter.setPen(QColor(255, 255, 255, 180));
    painter.drawText(previewRect, Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap, plainText);
}

} // namespace presentation
