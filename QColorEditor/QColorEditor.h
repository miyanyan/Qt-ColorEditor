#pragma once

#include <QWidget>

class QColorWheel : public QWidget
{
    Q_OBJECT
public:
    explicit QColorWheel(QWidget* parent = nullptr);

    void setSelectedColor(const QColor& color);
    QColor getSelectedColor() const;
    QColor getColor(int x, int y) const;

signals:
    void selectedColorChanged(const QColor& color);

protected:
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;

private:
    void processMouseEvent(QMouseEvent* e);

    int m_radius;
    QColor m_selectedColor;
};
