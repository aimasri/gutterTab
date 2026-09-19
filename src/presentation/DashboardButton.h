#pragma once

#include <QWidget>
#include <QPropertyAnimation>
#include <QPaintEvent>

namespace presentation {

/**
 * @brief Top-docked button to trigger the Bento Dashboard.
 */
class DashboardButton : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int currentWidth READ width WRITE setFixedWidth)
public:
    explicit DashboardButton(QWidget* parent = nullptr);
    void animateToWidth(int targetWidth);

signals:
    void hovered();
    void unhovered();
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QPropertyAnimation* m_widthAnim;
};

} // namespace presentation
