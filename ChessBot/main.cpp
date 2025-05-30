#include "ChessBot.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    ChessBot window;
    window.show();
    return app.exec();
}
