#include "mainview.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    MainView v;
    v.show();
    
    return a.exec();
}
