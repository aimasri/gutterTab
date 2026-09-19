#pragma once

#include <QWidget>
#include <QVector>
#include "BentoCard.h"
#include "../domain/TabController.h"

namespace presentation {

class BentoDashboard : public QWidget {
    Q_OBJECT
public:
    explicit BentoDashboard(domain::TabController* controller, QWidget* parent = nullptr);
    
    void showDashboard(const QRect& originRect);
    void hideDashboard();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    void buildCards();
    void computeMasonryLayout();
    
    domain::TabController* m_controller;
    QVector<BentoCard*> m_cards;
    QRect m_originRect;
    bool m_active = false;
};

} // namespace presentation
