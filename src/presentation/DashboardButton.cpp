#include "DashboardButton.h"
#include "../infrastructure/ConfigManager.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

namespace presentation {

DashboardButton::DashboardButton(QWidget* parent) : QWidget(parent) {
    auto& config = infrastructure::ConfigManager::instance().config();
    setFixedWidth(config.gutterRestWidth);
    setFixedHeight(50); // Shorter than regular tabs
    
    m_widthAnim = new QPropertyAnimation(this, "currentWidth", this);
    m_widthAnim->setDuration(150);
    m_widthAnim->setEasingCurve(QEasingCurve::OutCubic);
}

void DashboardButton::animateToWidth(int targetWidth) {
    if (m_widthAnim->endValue().toInt() == targetWidth) return;
    m_widthAnim->stop();
    m_widthAnim->setStartValue(width());
    m_widthAnim->setEndValue(targetWidth);
    m_widthAnim->start();
}

void DashboardButton::enterEvent(QEnterEvent* event) {
    Q_UNUSED(event);
    emit hovered();
}

void DashboardButton::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    emit unhovered();
}

void DashboardButton::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
}

void DashboardButton::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    auto& config = infrastructure::ConfigManager::instance().config();
    bool isRightEdge = (config.edge == infrastructure::Config::Edge::Right);
    
    QPainterPath path;
    int r = 8; // Corner radius
    if (isRightEdge) {
        path.moveTo(width(), 0);
        path.lineTo(r, 0);
        path.arcTo(0, 0, 2*r, 2*r, 90, 90);
        path.lineTo(0, height() - r);
        path.arcTo(0, height() - 2*r, 2*r, 2*r, 180, 90);
        path.lineTo(width(), height());
    } else {
        path.moveTo(0, 0);
        path.lineTo(width() - r, 0);
        path.arcTo(width() - 2*r, 0, 2*r, 2*r, 90, -90);
        path.lineTo(width(), height() - r);
        path.arcTo(width() - 2*r, height() - 2*r, 2*r, 2*r, 0, -90);
        path.lineTo(0, height());
    }
    
    painter.fillPath(path, QColor("#1f2029")); // Dark background
    
    // Draw simple bento box icon grid in the center
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#A0A0A0"));
    
    int cx = width() / 2;
    int cy = height() / 2;
    
    int size = 6;
    int gap = 2;
    
    // A 2x2 grid of small squares as the bento icon
    painter.drawRoundedRect(cx - size - gap, cy - size - gap, size, size, 2, 2);
    painter.drawRoundedRect(cx + gap, cy - size - gap, size, size, 2, 2);
    painter.drawRoundedRect(cx - size - gap, cy + gap, size, size, 2, 2);
    painter.drawRoundedRect(cx + gap, cy + gap, size, size, 2, 2);
}

} // namespace presentation
