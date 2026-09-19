#pragma once

#include <QWidget>
#include <QPropertyAnimation>
#include "../domain/Note.h"

namespace presentation {

class BentoCard : public QWidget {
    Q_OBJECT
public:
    explicit BentoCard(const domain::Note& note, QWidget* parent = nullptr);

    void setTargetGeometry(const QRect& rect);
    const QRect& targetGeometry() const { return m_targetGeometry; }
    void animateToTarget();
    void animateToOrigin(const QRect& origin);
    
    int noteId() const { return m_note.id; }

signals:
    void clicked(int noteId);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    domain::Note m_note;
    QRect m_targetGeometry;
    QPropertyAnimation* m_geometryAnim;
    bool m_hovered = false;
    
    // Extracted attachment path, if any
    QString m_attachmentPath;
    QPixmap m_thumbnail;
};

} // namespace presentation
