#pragma once

#include <QWidget>
#include <QFrame>
#include <QPushButton>
#include <QListWidget>
#include <QGridLayout>
#include "../domain/TabController.h"

namespace presentation {

/**
 * @brief Floating dialog overlay presenting a styled card for managing and opening folder shortcuts.
 */
class FoldersOverlay : public QWidget {
    Q_OBJECT
public:
    explicit FoldersOverlay(domain::TabController* controller, QWidget* parent = nullptr);
    ~FoldersOverlay() override;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onStateChanged(domain::TabController::State state);
    void onFoldersLoaded();
    void onCloseClicked();
    void onAddFolderClicked();
    
    // Context menu handlers
    void onEditShortcut(int id);
    void onDeleteShortcut(int id);

private:
    domain::TabController* m_controller;
    
    QFrame* m_cardFrame;
    QPushButton* m_closeButton;
    QPushButton* m_addBtn;
    
    // Grid layout to hold shortcut items
    QWidget* m_scrollAreaWidget;
    QGridLayout* m_gridLayout;
    
    void buildGrid();
    QPushButton* createToolbarBtn(const QString& text);
};

} // namespace presentation
