#include "ColorEditor.h"

#include <QApplication>
#include <QDebug>
#include <QDrag>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>

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
        emit selectedColorChanged(p->selectedColor);
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
{
}

QString ICombination::name()
{
    return "None";
}

QVector<QColor> ICombination::genColors(const QColor& color)
{
    return {};
}

void ICombination::setFactorRange(double min, double max)
{
    m_min = min;
    m_max = max;
}

void ICombination::serFactorValue(double value)
{
    m_value = value;
}

double ICombination::getFactor() const
{
    return m_value / (m_max - m_min);
}

Complementary::Complementary(QObject* parent)
    : ICombination(parent)
{
}

QString Complementary::name()
{
    return "Complementary";
}

QVector<QColor> Complementary::genColors(const QColor& color)
{
    return {QColor::fromHsv((color.hsvHue() + 180) % 360, color.hsvSaturation(), color.value())};
}

Monochromatic::Monochromatic(QObject* parent)
    : ICombination(parent)
{
}

QString Monochromatic::name()
{
    return "Monochromatic";
}

QVector<QColor> Monochromatic::genColors(const QColor& color)
{
    return {QColor::fromHsvF(color.hsvHueF(), color.hsvSaturationF(), color.valueF() * getFactor())};
}

Analogous::Analogous(QObject* parent)
    : ICombination(parent)
{
}

QString Analogous::name()
{
    return "Analogous";
}

QVector<QColor> Analogous::genColors(const QColor& color)
{
    int add = getFactor() * 180;
    return {QColor::fromHsv((color.hsvHue() + add) % 360, color.hsvSaturation(), color.value()),
            QColor::fromHsv((color.hsvHue() - add + 360) % 360, color.hsvSaturation(), color.value())};
}

Triadic::Triadic(QObject* parent)
    : ICombination(parent)
{
}

QString Triadic::name()
{
    return "Triadic";
}

QVector<QColor> Triadic::genColors(const QColor& color)
{
    return {QColor::fromHsv((color.hsvHue() + 120) % 360, color.hsvSaturation(), color.value()),
            QColor::fromHsv((color.hsvHue() - 120 + 360) % 360, color.hsvSaturation(), color.value())};
}

Tetradic::Tetradic(QObject* parent)
    : ICombination(parent)
{
}

QString Tetradic::name()
{
    return "Tetradic";
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
    int add = getFactor() * 180;
    return {QColor::fromHsv((color.hsvHue() - 90 + add + 360) % 360, color.hsvSaturation(), color.value()),
            QColor::fromHsv((color.hsvHue() + 180) % 360, color.hsvSaturation(), color.value()),
            QColor::fromHsv((color.hsvHue() + 90 + add + 360) % 360, color.hsvSaturation(), color.value())};
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
    connect(this, &QPushButton::clicked, this, [this]() { emit colorSelected(p->color); });
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
    qDebug() << "111111111";
    if (qvariant_cast<QColor>(e->mimeData()->colorData()).isValid())
        e->accept();
    else
        e->ignore();
}

void ColorButton::dragLeaveEvent(QDragLeaveEvent*)
{
    if (hasFocus())
        parentWidget()->setFocus();
}

void ColorButton::dropEvent(QDropEvent* e)
{
    auto color = qvariant_cast<QColor>(e->mimeData()->colorData());
        qDebug() << color;
    if (color.isValid()) {
        setColor(color);
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
}

void ColorPalette::addColor(const QColor& color)
{
    int index = p->colors.size();
    p->colors.push_back(color);

    auto btn = new ColorButton(this);
    btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    btn->setBolderWidth(1);
    connect(btn, &ColorButton::colorSelected, this, &ColorPalette::colorSelected);

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

//--------------------------------------------- color preview -------------------------------------------------------
class ColorPreview::Private
{
public:
    QColor currentColor;
    QColor previousColor;

    ColorButton* pbtnCurrent;
    ColorButton* pbtnPrevious;

    Private(const QColor& color, QWidget* parent)
        : currentColor(color)
        , previousColor(color)
        , pbtnCurrent(new ColorButton(parent))
        , pbtnPrevious(new ColorButton(parent))
    {
        pbtnCurrent->setBolderWidth(0);
        pbtnPrevious->setBolderWidth(0);

        auto layout = new QHBoxLayout(parent);
        layout->setSpacing(0);
        layout->addWidget(pbtnPrevious);
        layout->addWidget(pbtnCurrent);

        setCurrent(color);
    }

    void setCurrent(const QColor& color)
    {
        previousColor = currentColor;
        currentColor = color;

        pbtnCurrent->setColor(currentColor);
        pbtnPrevious->setColor(previousColor);
    }
};

ColorPreview::ColorPreview(const QColor& color, QWidget* parent)
    : QWidget(parent)
    , p(new Private(color, this))
{
}

void ColorPreview::setCurrentColor(const QColor& color)
{
    p->setCurrent(color);
}

QColor ColorPreview::currentColor() const
{
    return p->currentColor;
}

QColor ColorPreview::previousColor() const
{
    return p->previousColor;
}
