#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QFont font = app.font();
    font.setPointSize(15);
    app.setFont(font);

    app.setStyleSheet(QStringLiteral(
        "QMainWindow {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #E8F5E9, stop:0.5 #F1F8E9, stop:1 #E8F5E9);"
        "}"

        "QLineEdit {"
        "  border: 22px solid #C8E6C9;"
        "  border-radius: 30px;"
        "  padding: 34px 36px;"
        "  font-size: 38px;"
        "  background-color: white;"
        "}"
        "QLineEdit:focus { border-color: #4CAF50; background-color: #F1F8E9; }"

        "QTextEdit {"
        "  border: 22px solid #C8E6C9;"
        "  border-radius: 30px;"
        "  background-color: #FAFFFA;"
        "  font-size: 35px;"
        "  padding: 30px;"
        "}"

        "QLabel { font-size: 36px; color: #2E3B2E; }"

        "QGroupBox {"
        "  font-weight: bold;"
        "  font-size: 65px;"
        "  color: #1B5E20;"
        "  border: 1px solid #C8E6C9;"
        "  border-radius: 34px;"
        "  margin-top: 38px;"
        "  padding-top: 42px;"
        "  background-color: rgba(255,255,255,0.95);"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 36px;"
        "  padding: 0 30px;"
        "  color: #2E7D32;"
        "}"

        "QSpinBox {"
        "  border: 22px solid #C8E6C9;"
        "  border-radius: 30px;"
        "  padding: 32px 34px;"
        "  font-size: 38px;"
        "  background-color: white;"
        "}"
        "QSpinBox:focus { border-color: #4CAF50; }"

        "QProgressBar {"
        "  border: none;"
        "  border-radius: 30px;"
        "  background-color: #E0E8E0;"
        "  text-align: center;"
        "  min-height: 46px;"
        "  font-size: 34px;"
        "  color: #2E3B2E;"
        "}"
        "QProgressBar::chunk {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #66BB6A, stop:1 #43A047);"
        "  border-radius: 30px;"
        "}"

        "QScrollArea { border: none; background-color: transparent; }"
    ));

    MainWindow w;
    w.show();

    return app.exec();
}
