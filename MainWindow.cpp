#include "MainWindow.h"

#include "ColorEditor/ColorEditor.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto btn = new ColorButton(this);
    btn->setColor(Qt::blue);
    setCentralWidget(btn);

    connect(btn,&ColorButton::clicked, this, [this, btn](){
        auto editor = new ColorEditor(btn->color(), this);
        editor->show();
    });
}

MainWindow::~MainWindow() 
{
}
