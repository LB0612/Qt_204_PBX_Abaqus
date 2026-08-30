#include "mainwindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication a(argc, argv);

    a.setStyleSheet(QStringLiteral(R"(
QScrollBar:vertical {
    background: #f2f2f2;
    width: 14px;
    margin: 0px;
    border: none;
}

QScrollBar::handle:vertical {
    background: #8c8c8c;
    min-height: 45px;
    margin: 2px;
    border-radius: 5px;
}

QScrollBar::handle:vertical:hover {
    background: #595959;
}

QScrollBar::handle:vertical:pressed {
    background: #333333;
}

QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical {
    height: 0px;
}

QScrollBar::add-page:vertical,
QScrollBar::sub-page:vertical {
    background: transparent;
}

QScrollBar:horizontal {
    background: #f2f2f2;
    height: 14px;
    margin: 0px;
    border: none;
}

QScrollBar::handle:horizontal {
    background: #8c8c8c;
    min-width: 45px;
    margin: 2px;
    border-radius: 5px;
}

QScrollBar::handle:horizontal:hover {
    background: #595959;
}

QScrollBar::handle:horizontal:pressed {
    background: #333333;
}

QScrollBar::add-line:horizontal,
QScrollBar::sub-line:horizontal {
    width: 0px;
}

QScrollBar::add-page:horizontal,
QScrollBar::sub-page:horizontal {
    background: transparent;
}
)"));

    MainWindow w;
    w.showMaximized();
    return a.exec();
}
