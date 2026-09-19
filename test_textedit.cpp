#include <QApplication>
#include <QTextEdit>
#include <QDebug>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QTextEdit ed;
    ed.insertPlainText("Hello!");
    qDebug() << ed.toHtml().trimmed().startsWith("<");
    qDebug() << ed.toHtml();
    return 0;
}
