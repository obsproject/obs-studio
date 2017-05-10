#include "caboutdlg.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CAboutDlg w;
    w.show();

    return a.exec();
}
