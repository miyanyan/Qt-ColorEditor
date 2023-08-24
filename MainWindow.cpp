#include "MainWindow.h"

#include "QColorEditor/QColorEditor.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto widget = new ColorWheel(this);
    this->setCentralWidget(widget);

    //auto cc = new colorcombination::Complementary(this);
    //auto cc = new colorcombination::Monochromatic(this);
    //auto cc = new colorcombination::Analogous(this);
    //auto cc = new colorcombination::Triadic(this);
    auto cc = new colorcombo::Tetradic(this);
    cc->setFactor(0.8);
    widget->setColorCombination(cc);
}

MainWindow::~MainWindow() 
{
}
