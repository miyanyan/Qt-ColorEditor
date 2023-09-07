#include "MainWindow.h"

#include "ColorEditor/ColorEditor.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto btn = new ColorButton(this);
    btn->setColor(Qt::blue);
    setCentralWidget(btn);

    connect(btn,&ColorButton::clicked, this, [this, btn](){
        auto color = ColorEditor::getColor(btn->color(), this, "");
        btn->setColor(color);
    });
}

MainWindow::~MainWindow() 
{
}
