#include "BentoDashboard.h"
#include <QPainter>
#include <QTimer>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include "../infrastructure/ConfigManager.h"

namespace presentation {

BentoDashboard::BentoDashboard(domain::TabController* controller, QWidget* parent) 
    : QWidget(parent), m_controller(controller) 
{
    hide();
}

void BentoDashboard::buildCards() {
    for (auto card : m_cards) {
        card->hide();
        card->deleteLater();
    }
    m_cards.clear();

    const auto& notes = m_controller->notes();
    for (const auto& note : notes) {
        BentoCard* card = new BentoCard(note, this);
        connect(card, &BentoCard::clicked, this, [this, card](int noteId) {
            // Note selected! 
            // "the selected note transitions to the center with markdown ability, and the rest fly back to the edge."
            // Actually, TabController::openNote(noteId) transitions to OPEN state.
            // OverlayWindow will listen to this state change and ask BentoDashboard to hide gracefully.
            m_controller->openNote(noteId);
        });
        m_cards.append(card);
    }
}

void BentoDashboard::computeMasonryLayout() {
    int margin = 100;
    int gap = 16;
    int cols = 4;
    
    int availWidth = width() - 2 * margin;
    int unitW = (availWidth - (cols - 1) * gap) / cols;
    int unitH = unitW; // Squares by default
    
    // Very simple layout packer
    QVector<QVector<bool>> grid(20, QVector<bool>(cols, false));
    
    for (int i = 0; i < m_cards.size(); ++i) {
        BentoCard* card = m_cards[i];
        
        // Determine spanning
        int spanW = 1;
        int spanH = 1;
        
        // Simple heuristics for card sizing
        QString plainText = m_controller->notes()[i].content;
        bool hasAttachment = plainText.contains(QRegularExpression("!\\[.*?\\]\\((.*?)\\)"));
        
        if (hasAttachment) {
            spanW = 2;
            spanH = 2;
        } else if (plainText.length() > 300) {
            spanW = 2;
            spanH = 1;
        } else if (i % 5 == 0 && cols >= 2) {
            spanW = 1;
            spanH = 2;
        }
        
        // Find first free slot
        bool placed = false;
        for (int r = 0; r < grid.size() && !placed; ++r) {
            for (int c = 0; c <= cols - spanW && !placed; ++c) {
                // Check if space is free
                bool free = true;
                for (int dr = 0; dr < spanH; ++dr) {
                    for (int dc = 0; dc < spanW; ++dc) {
                        if (r + dr >= grid.size() || grid[r + dr][c + dc]) {
                            free = false;
                            break;
                        }
                    }
                    if (!free) break;
                }
                
                if (free) {
                    for (int dr = 0; dr < spanH; ++dr) {
                        for (int dc = 0; dc < spanW; ++dc) {
                            grid[r + dr][c + dc] = true;
                        }
                    }
                    
                    int x = margin + c * (unitW + gap);
                    int y = margin + r * (unitH + gap);
                    int w = spanW * unitW + (spanW - 1) * gap;
                    int h = spanH * unitH + (spanH - 1) * gap;
                    
                    card->setTargetGeometry(QRect(x, y, w, h));
                    placed = true;
                }
            }
        }
    }
}

void BentoDashboard::showDashboard(const QRect& originRect) {
    m_originRect = originRect;
    m_active = true;
    show();
    raise(); // Bring to front
    
    buildCards();
    computeMasonryLayout();
    
    for (auto card : m_cards) {
        card->setGeometry(m_originRect); // Start at edge
        card->show();
        card->animateToTarget();
    }
}

void BentoDashboard::hideDashboard() {
    m_active = false;
    for (auto card : m_cards) {
        card->animateToOrigin(m_originRect);
    }
    
    // Delay hide until animations finish
    QTimer::singleShot(400, this, [this]() {
        if (!m_active) {
            hide();
            for (auto card : m_cards) {
                card->hide();
                card->deleteLater();
            }
            m_cards.clear();
        }
    });
}

void BentoDashboard::mousePressEvent(QMouseEvent* event) {
    // Clicking the background dismisses the dashboard
    if (event->button() == Qt::LeftButton) {
        m_controller->toggleDashboard();
    }
}

void BentoDashboard::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    // Draw translucent background scrim to obscure desktop
    painter.fillRect(rect(), QColor(11, 12, 18, 180));
}

} // namespace presentation
