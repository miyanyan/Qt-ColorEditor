#include "ColorEditor.h"

#include <queue>

#include <QApplication>
#include <QDebug>
#include <QDrag>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QVBoxLayout>

//------------------------------------------------------- static color data --------------------------------------------
struct StaticColorEditorData
{
    static constexpr int rowCount = 4;
    static constexpr int colCount = 12;
    bool customSet = false;
    QRgb standardRgb[rowCount * colCount];
    QVector<QRgb> customRgb;

    StaticColorEditorData()
    {
        // standard
        int i = 0;
        for (int g = 0; g < 4; ++g)
            for (int r = 0; r < 4; ++r)
                for (int b = 0; b < 3; ++b) standardRgb[i++] = qRgb(r * 255 / 3, g * 255 / 3, b * 255 / 2);
        // custom
        readSettings();
    }

    void readSettings()
    {
        const QSettings settings(QSettings::UserScope, QStringLiteral("__ColorEditor"));
        int count = settings.value(QLatin1String("customCount")).toInt();
        customRgb.resize(count);
        // if zero, init with standard
        if (count == 0) {
            for (auto color : standardRgb) {
                customRgb.append(color);
            }
            return;
        }
        // otherwise, init with settings
        for (int i = 0; i < count; ++i) {
            const QVariant v = settings.value(QLatin1String("customColors/") + QString::number(i));
            if (v.isValid()) {
                customRgb[i] = v.toUInt();
            }
        }
    }
    void writeSettings()
    {
        const_cast<StaticColorEditorData*>(this)->customSet = false;
        QSettings settings(QSettings::UserScope, QStringLiteral("__ColorEditor"));
        int count = customRgb.size();
        settings.setValue(QLatin1String("customCount"), count);
        for (int i = 0; i < count; ++i) {
            settings.setValue(QLatin1String("customColors/") + QString::number(i), customRgb[i]);
        }
    }
};
Q_GLOBAL_STATIC(StaticColorEditorData, staticColorEditorData)

//--------------------------------------------------------- color wheel ------------------------------------------------
class ColorWheel::Private
{
public:
    int radius = 0;
    QColor selectedColor = QColor(Qt::white);
    QImage colorBuffer;
    colorcombo::ICombination* colorCombination = nullptr;

    void renderWheel(const QRect& rect)
    {
        auto center = rect.center();
        auto size = rect.size();

        radius = std::min(rect.width(), rect.height()) / 2;

        // init buffer
        colorBuffer = QImage(size, QImage::Format_ARGB32);
        colorBuffer.fill(Qt::transparent);

        // create gradient
        QConicalGradient hsvGradient(center, 0);
        for (int deg = 0; deg < 360; deg += 60) {
            hsvGradient.setColorAt(deg / 360.0, QColor::fromHsvF(deg / 360.0, 1.0, selectedColor.valueF()));
        }
        hsvGradient.setColorAt(1.0, QColor::fromHsvF(0.0, 1.0, selectedColor.valueF()));

        QRadialGradient valueGradient(center, radius);
        valueGradient.setColorAt(0.0, QColor::fromHsvF(0.0, 0.0, selectedColor.valueF()));
        valueGradient.setColorAt(1.0, Qt::transparent);

        QPainter painter(&colorBuffer);
        painter.setRenderHint(QPainter::Antialiasing, true);
        // draw color wheel
        painter.setPen(Qt::transparent);
        painter.setBrush(hsvGradient);
        painter.drawEllipse(center, radius, radius);
        painter.setBrush(valueGradient);
        painter.drawEllipse(center, radius, radius);
    }
};

ColorWheel::ColorWheel(QWidget* parent)
    : QWidget(parent)
    , p(new Private)
{
}

void ColorWheel::setColorCombination(colorcombo::ICombination* combination)
{
    p->colorCombination = combination;
    repaint();
}

void ColorWheel::setSelectedColor(const QColor& color)
{
    p->selectedColor = color;
    update();
}

QColor ColorWheel::getSelectedColor() const
{
    return p->selectedColor;
}

QColor ColorWheel::getColor(int x, int y) const
{
    if (p->radius <= 0) return QColor();

    auto line = QLineF(this->rect().center(), QPointF(x, y));
    auto h = line.angle() / 360.0;
    auto s = std::min(1.0, line.length() / p->radius);
    auto v = p->selectedColor.valueF();
    return QColor::fromHsvF(h, s, v);
}

void ColorWheel::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // draw wheel
    painter.drawImage(0, 0, p->colorBuffer);
    // draw selected color circle
    painter.setPen(Qt::black);
    painter.setBrush(Qt::white);
    drawSelector(&painter, p->selectedColor, 4);
    // draw color combination circle
    if (p->colorCombination) {
        auto colors = p->colorCombination->genColors(p->selectedColor);
        for (const auto& color : colors) {
            drawSelector(&painter, color, 3);
        }
        // add selected color, so the user can switch between this
        colors.push_back(p->selectedColor);
        emit combinationColorChanged(colors);
    }
}

void ColorWheel::mousePressEvent(QMouseEvent* e)
{
    processMouseEvent(e);
}

void ColorWheel::mouseMoveEvent(QMouseEvent* e)
{
    processMouseEvent(e);
}

void ColorWheel::resizeEvent(QResizeEvent* e)
{
    p->renderWheel(this->contentsRect());
}

void ColorWheel::processMouseEvent(QMouseEvent* e)
{
    if (e->buttons() & Qt::LeftButton) {
        p->selectedColor = getColor(e->x(), e->y());
        emit colorSelected(p->selectedColor);
        update();
    }
}

void ColorWheel::drawSelector(QPainter* painter, const QColor& color, int radius)
{
    auto line = QLineF::fromPolar(color.hsvSaturationF() * p->radius, color.hsvHueF() * 360.0);
    line.translate(this->rect().center());
    painter->drawEllipse(line.p2(), radius, radius);
}

//-------------------------------------------------- color combination --------------------------------------------
namespace colorcombo
{
ICombination::ICombination(QObject* parent)
    : QObject(parent)
    , m_min(0)
    , m_max(1)
    , m_value(0)
    , m_rangeEnabled(false)
{
}

ICombination::ICombination(double min, double max, double value, bool rangeEnabled, QObject* parent)
    : QObject(parent)
    , m_min(min)
    , m_max(max)
    , m_value(value)
    , m_rangeEnabled(rangeEnabled)
{
}

QString ICombination::name()
{
    return tr("None");
}

QVector<QColor> ICombination::genColors(const QColor& color)
{
    return {};
}

void ICombination::setRange(double min, double max)
{
    m_min = min;
    m_max = max;
}

void ICombination::serValue(double value)
{
    m_value = value;
}

double ICombination::min() const
{
    return m_min;
}

double ICombination::max() const
{
    return m_max;
}

double ICombination::getValue() const
{
    return m_value;
}

bool ICombination::rangeEnabled() const
{
    return m_rangeEnabled;
}

Complementary::Complementary(QObject* parent)
    : ICombination(parent)
{
}

QString Complementary::name()
{
    return tr("Complementary");
}

QVector<QColor> Complementary::genColors(const QColor& color)
{
    return {QColor::fromHsv((color.hsvHue() + 180) % 360, color.hsvSaturation(), color.value())};
}

Monochromatic::Monochromatic(QObject* parent)
    : ICombination(0, 1, 0.5, true, parent)
{
}

QString Monochromatic::name()
{
    return tr("Monochromatic");
}

QVector<QColor> Monochromatic::genColors(const QColor& color)
{
    return {QColor::fromHsvF(color.hsvHueF(), color.hsvSaturationF(), color.valueF() * getValue())};
}

Analogous::Analogous(QObject* parent)
    : ICombination(0, 180, 30, true, parent)
{
}

QString Analogous::name()
{
    return tr("Analogous");
}

QVector<QColor> Analogous::genColors(const QColor& color)
{
    int add = getValue();
    return {QColor::fromHsv((color.hsvHue() + add) % 360, color.hsvSaturation(), color.value()),
            QColor::fromHsv((color.hsvHue() - add + 360) % 360, color.hsvSaturation(), color.value())};
}

Triadic::Triadic(QObject* parent)
    : ICombination(0, 180, 120, true, parent)
{
}

QString Triadic::name()
{
    return tr("Triadic");
}

QVector<QColor> Triadic::genColors(const QColor& color)
{
    int add = getValue();
    return {QColor::fromHsv((color.hsvHue() + add) % 360, color.hsvSaturation(), color.value()),
            QColor::fromHsv((color.hsvHue() - add + 360) % 360, color.hsvSaturation(), color.value())};
}

Tetradic::Tetradic(QObject* parent)
    : ICombination(-90, 90, true, parent)
{
}

QString Tetradic::name()
{
    return tr("Tetradic");
}

QVector<QColor> Tetradic::genColors(const QColor& color)
{
    /*
     * A--------B
     * |        |
     * D--------C
     *
     * A : H, S, V
     * B : H - 90 + factor * 180, S, V
     * C : H + 180, S, V
     * D : H + 90 + factor * 180, S, V
     */
    int add = getValue();
    return {QColor::fromHsv((color.hsvHue() + add + 360) % 360, color.hsvSaturation(), color.value()),
            QColor::fromHsv((color.hsvHue() + 180) % 360, color.hsvSaturation(), color.value()),
            QColor::fromHsv((color.hsvHue() + add + 360) % 360, color.hsvSaturation(), color.value())};
}
} // namespace colorcombo

//--------------------------------------------------- color slider -------------------------------------------
class ColorSlider::Private
{
public:
    QColor startColor;
    QColor stopColor;
};

ColorSlider::ColorSlider(QWidget* parent)
    : QSlider(Qt::Horizontal, parent)
    , p(new Private)
{
    connect(this, &QSlider::valueChanged, this, [this](int value) {
        int minv = this->minimum();
        int maxv = this->maximum();
        float factor = 1.0f * (value - minv) / (maxv - minv);
        int r = p->startColor.red() + factor * (p->stopColor.red() - p->startColor.red());
        int g = p->startColor.green() + factor * (p->stopColor.green() - p->startColor.green());
        int b = p->startColor.blue() + factor * (p->stopColor.blue() - p->startColor.blue());
        emit currentColorChanged(QColor(r, g, b));
    });
}

void ColorSlider::setGradient(const QColor& startColor, const QColor& stopColor)
{
    p->startColor = startColor;
    p->stopColor = stopColor;

    QString ori;
    float x1, y1, x2, y2;
    if (orientation() == Qt::Horizontal) {
        ori = "horizontal";
        x1 = 0;
        y1 = 0.5f;
        x2 = 1;
        y2 = 0.5f;
    }
    else {
        ori = "vertical";
        x1 = 0.5f;
        y1 = 0;
        x2 = 0.5f;
        y2 = 1;
    }

    auto style = QString("QSlider::groove:%1{background:qlineargradient(x1:%2,y1:%3,x2:%4,y2:%5,stop:0 %6,stop:1 %7);}"
                         "QSlider::handle:%1{background:#5C5C5C;border:1px solid;height:4px;width:4px}")
                     .arg(ori)
                     .arg(x1)
                     .arg(y1)
                     .arg(x2)
                     .arg(y2)
                     .arg(startColor.name())
                     .arg(stopColor.name());

    setStyleSheet(style);
}

QColor ColorSlider::startColor() const
{
    return p->startColor;
}

QColor ColorSlider::stopColor() const
{
    return p->stopColor;
}

class ColorSpinHSlider::Private
{
public:
    QSpinBox* spinbox;
    ColorSlider* slider;

    Private(const QString& name, QWidget* parent)
    {
        QLabel* text = new QLabel(name, parent);
        text->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        spinbox = new QSpinBox(parent);
        spinbox->setButtonSymbols(QAbstractSpinBox::NoButtons);
        spinbox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        slider = new ColorSlider(parent);
        spinbox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        auto layout = new QHBoxLayout(parent);
        layout->addWidget(text, 1);
        layout->addWidget(spinbox, 2);
        layout->addWidget(slider, 7);

        connect(slider, &QSlider::valueChanged, spinbox, &QSpinBox::setValue);
        connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged), slider, &ColorSlider::setValue);
    }
};
ColorSpinHSlider::ColorSpinHSlider(const QString& name, QWidget* parent)
    : QWidget(parent)
    , p(new Private(name, this))
{
    connect(p->slider, &ColorSlider::currentColorChanged, this, &ColorSpinHSlider::currentColorChanged);
}

void ColorSpinHSlider::setGradient(const QColor& startColor, const QColor& stopColor)
{
    p->slider->setGradient(startColor, stopColor);
}

void ColorSpinHSlider::setValue(double value)
{
    p->spinbox->setValue(value);
}

void ColorSpinHSlider::setRange(double min, double max)
{
    p->slider->setRange(min, max);
    p->spinbox->setRange(min, max);
}

//--------------------------------------------- color button -------------------------------------------------------
class ColorButton::Private
{
public:
    QPoint pressPos;
    QColor color;
    int bolderWidth = 0;

    void updateStyle(QPushButton* btn)
    {
        auto style = QString("QPushButton{min-width:30px;min-height:30px;background-color:%1;border:%2px solid;}"
                             "QPushButton:pressed{border: 1px solid #ffd700;}")
                         .arg(color.name())
                         .arg(bolderWidth);
        btn->setStyleSheet(style);
    }
};

ColorButton::ColorButton(QWidget* parent)
    : QPushButton(parent)
    , p(new Private)
{
    setAcceptDrops(true);
    connect(this, &QPushButton::clicked, this, [this]() { emit colorClicked(p->color); });
}

void ColorButton::setColor(const QColor& color)
{
    p->color = color;
    p->updateStyle(this);
}

void ColorButton::setBolderWidth(int width)
{
    p->bolderWidth = width;
    p->updateStyle(this);
}

QColor ColorButton::color() const
{
    return p->color;
}

void ColorButton::mousePressEvent(QMouseEvent* e)
{
    p->pressPos = e->pos();
    QPushButton::mousePressEvent(e);
}

void ColorButton::mouseMoveEvent(QMouseEvent* e)
{
    if (e->buttons() & Qt::LeftButton) {
        if ((p->pressPos - e->pos()).manhattanLength() > QApplication::startDragDistance()) {
            QMimeData* mime = new QMimeData;
            mime->setColorData(p->color);
            QPixmap pix(width(), height());
            pix.fill(p->color);
            QDrag* drg = new QDrag(this);
            drg->setMimeData(mime);
            drg->setPixmap(pix);
            drg->exec(Qt::CopyAction);
        }
    }
}

void ColorButton::dragEnterEvent(QDragEnterEvent* e)
{
    if (qvariant_cast<QColor>(e->mimeData()->colorData()).isValid())
        e->accept();
    else
        e->ignore();
}

void ColorButton::dragLeaveEvent(QDragLeaveEvent*)
{
    if (hasFocus()) parentWidget()->setFocus();
}

void ColorButton::dropEvent(QDropEvent* e)
{
    auto color = qvariant_cast<QColor>(e->mimeData()->colorData());
    if (color.isValid()) {
        setColor(color);
        emit colorDroped(color);
        e->accept();
    }
    else {
        e->ignore();
    }
}

//--------------------------------------------- color palette ------------------------------------------------------
class ColorPalette::Private
{
public:
    int columnCount = 0;
    QGridLayout* layout = nullptr;
    QVector<QColor> colors;

    Private(int column, QScrollArea* parent)
    {
        columnCount = column;

        auto scrollWidget = new QWidget(parent);
        layout = new QGridLayout(scrollWidget);
        layout->setAlignment(Qt::AlignTop);
        layout->setSpacing(0);

        parent->setWidget(scrollWidget);
    }

    std::pair<int, int> getLayoutIndex(int index) { return {index / columnCount, index % columnCount}; }

    void updateLayout(int begin, int end)
    {
        for (int i = begin; i < end; ++i) {
            int row = i / columnCount;
            int col = i % columnCount;
            auto btn = qobject_cast<ColorButton*>(layout->itemAtPosition(row, col)->widget());
            btn->setColor(colors[i]);
        }
    }
};

ColorPalette::ColorPalette(int column, QWidget* parent)
    : QScrollArea(parent)
    , p(new Private(column, this))
{
    setWidgetResizable(true);
    setAcceptDrops(true);
}

void ColorPalette::addColor(const QColor& color)
{
    int index = p->colors.size();
    p->colors.push_back(color);

    auto btn = new ColorButton(this);
    btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    btn->setBolderWidth(1);
    connect(btn, &ColorButton::colorClicked, this, &ColorPalette::colorClicked);

    auto layoutIndex = p->getLayoutIndex(index);
    p->layout->addWidget(btn, layoutIndex.first, layoutIndex.second);

    p->updateLayout(index, index + 1);
}

void ColorPalette::setColor(const QColor& color, int row, int column)
{
    int index = row * p->columnCount + column;
    p->colors[index] = color;
    p->updateLayout(index, index + 1);
}

void ColorPalette::removeColor(const QColor& color, int row, int column)
{
    auto item = p->layout->itemAtPosition(row, column);
    p->layout->removeItem(item);
    delete item;

    int index = row * p->columnCount + column;
    p->colors.remove(index);
    p->updateLayout(index, p->colors.size());
}

void ColorPalette::dragEnterEvent(QDragEnterEvent* e)
{
    if (qvariant_cast<QColor>(e->mimeData()->colorData()).isValid())
        e->accept();
    else
        e->ignore();
}

void ColorPalette::dropEvent(QDropEvent* e)
{
    auto color = qvariant_cast<QColor>(e->mimeData()->colorData());
    if (color.isValid()) {
        addColor(color);
        e->accept();
    }
    else {
        e->ignore();
    }
}

//--------------------------------------------- color preview -------------------------------------------------------
class ColorPreview::Private
{
public:
    ColorButton* pbtnCurrent;
    ColorButton* pbtnPrevious;

    Private(QWidget* parent)
        : pbtnCurrent(new ColorButton(parent))
        , pbtnPrevious(new ColorButton(parent))
    {
        pbtnCurrent->setBolderWidth(0);
        pbtnPrevious->setBolderWidth(0);

        auto layout = new QHBoxLayout(parent);
        layout->setSpacing(0);
        layout->addWidget(pbtnPrevious);
        layout->addWidget(pbtnCurrent);
    }

    void setCurrent(const QColor& color)
    {
        auto previousColor = pbtnCurrent->color();
        pbtnCurrent->setColor(color);
        pbtnPrevious->setColor(previousColor);
    }
};

ColorPreview::ColorPreview(QWidget* parent)
    : QWidget(parent)
    , p(new Private(this))
{
    // only emit when current color changed
    connect(p->pbtnCurrent, &ColorButton::colorDroped, this, &ColorPreview::currentColorChanged);
}

void ColorPreview::setCurrentColor(const QColor& color)
{
    p->setCurrent(color);
}

QColor ColorPreview::currentColor() const
{
    return p->pbtnCurrent->color();
}

QColor ColorPreview::previousColor() const
{
    return p->pbtnPrevious->color();
}

//------------------------------------------- color combo widget ---------------------------
class ColorComboWidget::Private
{
public:
    static constexpr int factor = 360;
    std::queue<colorcombo::ICombination*> combs;
    QHBoxLayout* hlayout = nullptr;
    QPushButton* switchBtn = nullptr;
    QSlider* factorSlider = nullptr;
    QDoubleSpinBox* factorSpinbox = nullptr;

    Private(QWidget* parent)
    {
        factorSpinbox = new QDoubleSpinBox(parent);
        factorSlider = new QSlider(Qt::Horizontal, parent);
        switchBtn = new QPushButton(parent);
        factorSpinbox->setButtonSymbols(QAbstractSpinBox::NoButtons);
        factorSpinbox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        switchBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        auto layout = new QGridLayout(parent);
        hlayout = new QHBoxLayout();
        layout->addLayout(hlayout, 0, 0, 1, 3);
        layout->addWidget(switchBtn, 0, 3, 1, 1);
        layout->addWidget(factorSpinbox, 1, 0, 1, 1);
        layout->addWidget(factorSlider, 1, 1, 1, 3);

        connect(factorSlider, &QSlider::valueChanged, factorSpinbox, [this](int value) { factorSpinbox->setValue(1.0 * value / factor); });
        connect(factorSpinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), factorSlider,
                [this](double value) { factorSlider->setValue(value * factor); });
    }
};

ColorComboWidget::ColorComboWidget(QWidget* parent)
    : QWidget(parent)
    , p(new Private(this))
{
    // dummy
    addCombination(new colorcombo::ICombination(this));
    switchCombination();
}

void ColorComboWidget::addCombination(colorcombo::ICombination* combo)
{
    p->combs.push(combo);
}

void ColorComboWidget::setColors(const QVector<QColor>& colors)
{
    for (int i = 0; i < colors.size(); ++i) {
        auto btn = qobject_cast<ColorButton*>(p->hlayout->itemAt(i)->widget());
        btn->setColor(colors[i]);
    }
}

void ColorComboWidget::switchCombination()
{
    if (p->combs.empty()) return;

    auto front = p->combs.front();
    p->combs.pop();
    p->combs.push(front);

    auto currentComb = p->combs.front();
    emit combinationChanged(currentComb);

    // make slider looks like double slider
    p->factorSlider->setRange(currentComb->min() * p->factor, currentComb->max() * p->factor);
    p->factorSpinbox->setRange(currentComb->min(), currentComb->max());
    p->factorSpinbox->setSingleStep((currentComb->max() - currentComb->min()) / p->factor);
    p->factorSpinbox->setValue(currentComb->getValue());

    // clear
    for (int i = 0; i < p->hlayout->count(); ++i) {
        auto itemw = p->hlayout->itemAt(i)->widget();
        p->hlayout->removeWidget(itemw);
        delete itemw;
    }
    // add
    auto colors = currentComb->genColors(Qt::white);
    int size = colors.size() + 1;
    for (int i = 0; i < size; ++i) {
        auto btn = new ColorButton(this);
        btn->setBolderWidth(0);
        btn->setAcceptDrops(false); // color can't be changed by drop
        connect(btn, &ColorButton::colorClicked, this, &ColorComboWidget::colorClicked);
        p->hlayout->addWidget(btn);
    }
}

//------------------------------------------ color editor ----------------------------------
class ColorEditor::Private
{
public:
    ColorWheel* wheel;
    QLineEdit* colorText;
    ColorPreview* preview;
    ColorComboWidget* combo;
    QGroupBox* previewGroup;
    QGroupBox* comboGroup;
    ColorPalette* palette;
    ColorSpinHSlider* rSlider;
    ColorSpinHSlider* gSlider;
    ColorSpinHSlider* bSlider;
    ColorSpinHSlider* hSlider;
    ColorSpinHSlider* sSlider;
    ColorSpinHSlider* vSlider;

    Private(const QColor& color, QWidget* parent)
    {
        // left
        wheel = new ColorWheel(parent);
        colorText = new QLineEdit(parent);
        preview = new ColorPreview(parent);
        combo = new ColorComboWidget(parent);
        previewGroup = new QGroupBox(tr("Previous/Current Colors"), parent);
        comboGroup = new QGroupBox(tr("Color Combination"), parent);

        auto previwGroupLayout = new QHBoxLayout(previewGroup);
        previwGroupLayout->addWidget(preview);

        auto comboGroupLayout = new QHBoxLayout(comboGroup);
        comboGroupLayout->addWidget(combo);

        auto leftWidget = new QWidget(parent);
        auto leftLayout = new QVBoxLayout(leftWidget);
        leftLayout->addWidget(wheel, 5);
        leftLayout->addWidget(colorText, 1);
        leftLayout->addWidget(previewGroup, 2);
        leftLayout->addWidget(comboGroup, 2);

        // right
        palette = new ColorPalette(staticColorEditorData->colCount, parent);
        rSlider = new ColorSpinHSlider("R", parent);
        gSlider = new ColorSpinHSlider("G", parent);
        bSlider = new ColorSpinHSlider("B", parent);
        hSlider = new ColorSpinHSlider("H", parent);
        sSlider = new ColorSpinHSlider("S", parent);
        vSlider = new ColorSpinHSlider("V", parent);

        rSlider->setRange(0, 255);
        gSlider->setRange(0, 255);
        bSlider->setRange(0, 255);
        hSlider->setRange(0, 360);
        sSlider->setRange(0, 255);
        vSlider->setRange(0, 255);

        auto rightWidget = new QWidget(parent);
        auto rightLayout = new QVBoxLayout(rightWidget);
        rightLayout->addWidget(palette, 6);
        rightLayout->addWidget(rSlider, 1);
        rightLayout->addWidget(gSlider, 1);
        rightLayout->addWidget(bSlider, 1);
        rightLayout->addWidget(hSlider, 1);
        rightLayout->addWidget(sSlider, 1);
        rightLayout->addWidget(vSlider, 1);

        auto splitter = new QSplitter(parent);
        splitter->addWidget(leftWidget);
        splitter->addWidget(rightWidget);

        auto layout = new QHBoxLayout(parent);
        layout->addWidget(splitter);

        setCurrentColor(color);
    }

    void blockSignals(bool block)
    {
        wheel->blockSignals(block);
        colorText->blockSignals(block);
        preview->blockSignals(block);
        combo->blockSignals(block);
        palette->blockSignals(block);
        rSlider->blockSignals(block);
        gSlider->blockSignals(block);
        bSlider->blockSignals(block);
        hSlider->blockSignals(block);
        sSlider->blockSignals(block);
        vSlider->blockSignals(block);
    }

    void setCurrentColor(const QColor& color)
    {
        blockSignals(true);
        wheel->setSelectedColor(color);
        colorText->setText(color.name());
        preview->setCurrentColor(color);

        rSlider->setGradient(QColor(0, color.green(), color.blue()), QColor(255, color.green(), color.blue()));
        gSlider->setGradient(QColor(color.red(), 0, color.blue()), QColor(color.red(), 255, color.blue()));
        bSlider->setGradient(QColor(color.red(), color.green(), 0), QColor(color.red(), color.green(), 255));
        hSlider->setGradient(QColor::fromHsvF(0, color.hsvSaturationF(), color.valueF()), QColor::fromHsvF(1, color.hsvSaturationF(), color.valueF()));
        sSlider->setGradient(QColor::fromHsvF(color.hsvHueF(), 0, color.valueF()), QColor::fromHsvF(color.hsvHueF(), 1, color.valueF()));
        vSlider->setGradient(QColor::fromHsvF(color.hsvHueF(), color.hsvSaturationF(), 0), QColor::fromHsvF(color.hsvHueF(), color.hsvSaturationF(), 1));

        rSlider->setValue(color.redF());
        gSlider->setValue(color.green());
        bSlider->setValue(color.blue());
        hSlider->setValue(color.hsvHue());
        sSlider->setValue(color.hsvSaturationF());
        vSlider->setValue(color.valueF());
        blockSignals(false);
    }
};

ColorEditor::ColorEditor(const QColor& color, QWidget* parent)
    : QDialog(parent)
    , p(new Private(color, this))
{
}

void ColorEditor::setCurrentColor(const QColor& color)
{
    p->setCurrentColor(color);
}

QColor ColorEditor::currentColor() const
{
    return p->preview->currentColor();
}
