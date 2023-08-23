#include "QColorEditor.h"

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>

//--------------------------------------------------------- color wheel ------------------------------------------------
QColorWheel::QColorWheel(QWidget* parent)
    : QWidget(parent)
    , m_radius(0)
    , m_selectedColor(Qt::white)
    , m_colorCombination(nullptr)
{
}

void QColorWheel::setColorCombination(colorcombo::ICombination* combination)
{
    m_colorCombination = combination;
}

void QColorWheel::setSelectedColor(const QColor& color)
{
    m_selectedColor = color;
    update();
}

QColor QColorWheel::getSelectedColor() const
{
    return m_selectedColor;
}

QColor QColorWheel::getColor(int x, int y) const
{
    if (m_radius <= 0) return QColor();

    auto line = QLineF(this->rect().center(), QPointF(x, y));
    auto h = line.angle() / 360.0;
    auto s = std::min(1.0, line.length() / m_radius);
    auto v = m_selectedColor.valueF();
    return QColor::fromHsvF(h, s, v);
}

void QColorWheel::paintEvent(QPaintEvent* e)
{
    auto center = this->rect().center();
    m_radius = std::min(this->contentsRect().width(), this->contentsRect().height()) / 2;

    QConicalGradient hsvGradient(center, 0);
    for (int deg = 0; deg < 360; ++deg) {
        hsvGradient.setColorAt(deg / 360.0, QColor::fromHsvF(deg / 360.0, 1.0, m_selectedColor.valueF()));
    }

    QRadialGradient valueGradient(center, m_radius);
    valueGradient.setColorAt(0.0, QColor::fromHsvF(0.0, 0.0, m_selectedColor.valueF()));
    valueGradient.setColorAt(1.0, Qt::transparent);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // draw color wheel
    painter.setPen(Qt::transparent);
    painter.setBrush(hsvGradient);
    painter.drawEllipse(center, m_radius, m_radius);
    painter.setBrush(valueGradient);
    painter.drawEllipse(center, m_radius, m_radius);
    // draw selected color circle
    painter.setPen(Qt::black);
    painter.setBrush(Qt::white);
    drawColorCircle(&painter, m_selectedColor, 4);
    // draw color combination circle
    if (m_colorCombination) {
        auto colors = m_colorCombination->genColors(m_selectedColor);

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

void QColorWheel::processMouseEvent(QMouseEvent* e)
{
    if (e->buttons() & Qt::LeftButton) {
        m_selectedColor = getColor(e->x(), e->y());
        emit selectedColorChanged(m_selectedColor);
        update();
    }
}

void QColorWheel::drawColorCircle(QPainter* painter, const QColor& color, int radius)
{
    auto line = QLineF::fromPolar(color.hsvSaturationF() * m_radius, color.hsvHueF() * 360.0);
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