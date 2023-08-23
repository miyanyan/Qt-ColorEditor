#include "QColorEditor.h"

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QImage>

//--------------------------------------------------------- color wheel ------------------------------------------------
class QColorWheel::Private
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

QColorWheel::QColorWheel(QWidget* parent)
    : QWidget(parent)
    , p(new Private)
{
}

void QColorWheel::setColorCombination(colorcombo::ICombination* combination)
{
    p->colorCombination = combination;
}

void QColorWheel::setSelectedColor(const QColor& color)
{
    p->selectedColor = color;
    update();
}

QColor QColorWheel::getSelectedColor() const
{
    return p->selectedColor;
}

QColor QColorWheel::getColor(int x, int y) const
{
    if (p->radius <= 0) return QColor();

    auto line = QLineF(this->rect().center(), QPointF(x, y));
    auto h = line.angle() / 360.0;
    auto s = std::min(1.0, line.length() / p->radius);
    auto v = p->selectedColor.valueF();
    return QColor::fromHsvF(h, s, v);
}

void QColorWheel::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // draw wheel
    painter.drawImage(0, 0, p->colorBuffer);
    // draw selected color circle
    painter.setPen(Qt::black);
    painter.setBrush(Qt::white);
    drawColorCircle(&painter, p->selectedColor, 4);
    // draw color combination circle
    if (p->colorCombination) {
        auto colors = p->colorCombination->genColors(p->selectedColor);
        for (const auto& color : colors) {
            drawColorCircle(&painter, color, 3);
        }
        emit combinationColorChanged(colors);
    }
}

void QColorWheel::mousePressEvent(QMouseEvent* e)
{
    processMouseEvent(e);
}

void QColorWheel::mouseMoveEvent(QMouseEvent* e)
{
    processMouseEvent(e);
}

void QColorWheel::resizeEvent(QResizeEvent* e)
{
    p->renderWheel(this->contentsRect());
}

void QColorWheel::processMouseEvent(QMouseEvent* e)
{
    if (e->buttons() & Qt::LeftButton) {
        p->selectedColor = getColor(e->x(), e->y());
        emit selectedColorChanged(p->selectedColor);
        update();
    }
}

void QColorWheel::drawColorCircle(QPainter* painter, const QColor& color, int radius)
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
} // namespace colorcombination