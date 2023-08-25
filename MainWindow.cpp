#include "MainWindow.h"

#include "ColorEditor/ColorEditor.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    //auto widget = new ColorWheel(this);
    //this->setCentralWidget(widget);

    ////auto cc = new colorcombination::Complementary(this);
    ////auto cc = new colorcombination::Monochromatic(this);
    ////auto cc = new colorcombination::Analogous(this);
    ////auto cc = new colorcombination::Triadic(this);
    //auto cc = new colorcombo::Tetradic(this);
    //cc->setFactor(0.8);
    //widget->setColorCombination(cc);

    auto widget = new ColorPalette(10, this);
    for (int g = 0; g < 4; ++g)
        for (int r = 0; r < 4; ++r)
            for (int b = 0; b < 3; ++b)
                widget->addColor(QColor(r * 255 / 3, g * 255 / 3, b * 255 / 2));
    this->setCentralWidget(widget);

    //auto widget = new ColorPreview(Qt::red, this);
    //widget->setCurrentColor(Qt::blue);
    //setCentralWidget(widget);
}

MainWindow::~MainWindow() 
{
}
