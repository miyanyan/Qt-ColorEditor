#include "QColorEditor.h"

#include <QDebug>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>

//------------------------------------------------------- static color data --------------------------------------------
struct StaticColorEditorData
{
    static constexpr int standardCount = 4 * 12;
    bool customSet = false;
    QRgb standardRgb[standardCount];
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
    , m_factor(0.5)
{
}

void ICombination::setFactor(double factor)
{
    m_factor = factor;
}

double ICombination::getFactor() const
{
    return m_factor;
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
