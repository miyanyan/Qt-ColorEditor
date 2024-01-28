# QColorEditor

A qt based color editor, and also provides some color widgets:
* ColorWheel, a color wheel to select color
* GradientSlider, a gradient slider with gradient color
* ColorSpinHSlider, a horizontal color slider with label and spinbox
* ColorButton, a color button to show color, drag and drop color
* ColorPalette, a color palette to show a list of colors, drag and drop color
* ColorPreview, a color preview to show the current and previous color
* ColorComboWidget, a widget to switch color combinations
* ColorLineEdit, a color lineedit to show color name
* ColorPicker, a color picker to pick screen color

## Gallery
* srgb switch
![](./images/srgb.gif)

* select color by color wheel
![](./images/colorwheel.gif)

* select color by color text
![](./images/colortext.gif)

* select color by color picker(not in srgb)
![](./images/colorpicker.gif)

* select color by color slider
![](./images/colorslider.gif)

* select color by color palette
![](./images/colorpalette-select.gif)

* save/remove color in color palette(permanent)
![](./images/colorpalette-saveremove.gif)

* previous/current color change
![](./images/color-precur.gif)

* color combination
![](./images/colorcombination.gif)

## How to use
copy `ColorWidgets` forder(only contains 2 files: `ColorEditor.h` and `ColorEditor.cpp`) to your project, remenber add to your build system.

then you can use like:
```
#include "ColorWidgets/ColorEditor.h"
// ...
// call here, you can find this in MainWindow.cpp
auto btn = new ColorButton(this);
btn->setColor(Qt::blue);
setCentralWidget(btn);

connect(btn, &ColorButton::clicked, this, [this, btn](){
    auto color = ColorEditor::getColor(btn->color(), this, "");
    btn->setColor(color);
});
```