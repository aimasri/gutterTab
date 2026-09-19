#include <QApplication>
#include <QTextEdit>
#include <QDebug>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QTextEdit edit;
    edit.setMarkdown("# Hello");
    qDebug() << "Markdown:" << edit.toMarkdown();
    edit.append("World");
    qDebug() << "Markdown 2:" << edit.toMarkdown();
    return 0;
}
