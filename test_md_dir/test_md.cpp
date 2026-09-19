#include <QApplication>
#include <QPushButton>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QPushButton btn("Test");
    btn.setProperty("isToolbarBtn", true);
    btn.setStyleSheet("QPushButton[isToolbarBtn=\"true\"] { background-color: red; }");
    btn.show();
    
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();
    
    return 0;
}
