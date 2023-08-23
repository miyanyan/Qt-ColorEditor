#include "MainWindow.h"

#include "QColorWheel.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto widget = new QColorWheel(this);
    this->setCentralWidget(widget);
}

MainWindow::~MainWindow() 
{
}
