#include <QApplication>
#include <QPushButton>
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QPushButton btn("⧉");
    btn.show();
    return app.exec();
}
