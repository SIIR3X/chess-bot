#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_ChessBot.h"

class ChessBot : public QMainWindow
{
    Q_OBJECT

public:
    ChessBot(QWidget *parent = nullptr);
    ~ChessBot();

private:
    Ui::ChessBotClass ui;
};

