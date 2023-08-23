#include "MainWindow.h"

#include "QColorEditor/QColorEditor.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto widget = new QColorWheel(this);
    this->setCentralWidget(widget);
}

MainWindow::~MainWindow() 
{
}
