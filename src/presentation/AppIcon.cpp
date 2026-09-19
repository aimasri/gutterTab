#include "AppIcon.h"

#include <QColor>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRectF>

namespace presentation {

QPixmap AppIcon::createIconPixmap(int size) {
    if (size <= 0) size = 64;

    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    double scale = size / 256.0;
    p.scale(scale, scale);

    // 1. Dark sleek obsidian squircle container
    QRectF containerRect(10.0, 10.0, 236.0, 236.0);
    QPainterPath containerPath;
    containerPath.addRoundedRect(containerRect, 56.0, 56.0);

    QLinearGradient bgGrad(0.0, 0.0, 256.0, 256.0);
    bgGrad.setColorAt(0.0, QColor(0x18, 0x19, 0x24));
    bgGrad.setColorAt(1.0, QColor(0x0b, 0x0c, 0x12));
    p.fillPath(containerPath, bgGrad);

    // Subtle edge rim border
    p.strokePath(containerPath, QPen(QColor(255, 255, 255, 28), 2.5));

    // Helper for adding a styled rectangle
    auto drawRect = [&](double x, double y, double w, double h, double r, const QColor& c1, const QColor& c2, bool vertical) {
        QPainterPath path;
        path.addRoundedRect(QRectF(x, y, w, h), r, r);
        QLinearGradient grad(0, 0, vertical ? 0 : 256, vertical ? 256 : 256);
        if (!vertical) {
            grad.setStart(x, y);
            grad.setFinalStop(x + w, y + h);
        } else {
            grad.setStart(x, y);
            grad.setFinalStop(x, y + h);
        }
        grad.setColorAt(0.0, c1);
        grad.setColorAt(1.0, c2);
        p.fillPath(path, grad);
    };

    drawRect(36, 36, 162, 64, 8, QColor("#38e2fc"), QColor("#06b6d4"), false);
    drawRect(36, 108, 77, 50, 7, QColor("#818cf8"), QColor("#6366f1"), false);
    drawRect(121, 108, 77, 50, 7, QColor("#f472b6"), QColor("#ec4899"), false);
    drawRect(36, 166, 58, 54, 7, QColor("#fdba74"), QColor("#fb923c"), false);
    drawRect(102, 166, 96, 54, 7, QColor("#34d399"), QColor("#10b981"), false);
    drawRect(206, 36, 14, 184, 5, QColor("#38bdf8"), QColor("#ec4899"), true);

    p.end();
    return pix;
}

QIcon AppIcon::createAppIcon() {
    QIcon icon;
    const int sizes[] = {16, 24, 32, 48, 64, 128, 256};
    for (int sz : sizes) {
        icon.addPixmap(createIconPixmap(sz));
    }
    return icon;
}

} // namespace presentation
