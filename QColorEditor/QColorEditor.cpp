#include "QColorEditor.h"

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>

QColorWheel::QColorWheel(QWidget *parent)
    : QWidget(parent)
    , m_radius(0)
    , m_selectedColor(Qt::white)
{
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

void QColorWheel::paintEvent(QPaintEvent *e)
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
    auto line = QLineF::fromPolar(m_selectedColor.hsvSaturationF() * m_radius, m_selectedColor.hsvHueF() * 360.0);
    line.translate(center);
    painter.drawEllipse(line.p2(), 3, 3);
}

void QColorWheel::mousePressEvent(QMouseEvent *e)
{
    processMouseEvent(e);
}

void QColorWheel::mouseMoveEvent(QMouseEvent *e)
{
    processMouseEvent(e);
}

void QColorWheel::processMouseEvent(QMouseEvent *e)
{
    if (e->buttons() & Qt::LeftButton) {
        m_selectedColor = getColor(e->x(), e->y());
        emit selectedColorChanged(m_selectedColor);
        update();
    }
}
