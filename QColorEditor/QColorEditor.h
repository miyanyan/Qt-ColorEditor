#pragma once

#include <memory>

#include <QWidget>

//--------------------------------------------------- color combination --------------------------------------------------
namespace colorcombo
{
class ICombination : public QObject
{
    Q_OBJECT
public:
    explicit ICombination(QObject* parent = nullptr);
    virtual ~ICombination() = default;
    virtual QString name() = 0;
    virtual QVector<QColor> genColors(const QColor& color) = 0;
    // factor range: [0, 1]
    void setFactor(double factor);
    double getFactor() const;

private:
    double m_factor;
};

class Complementary : public ICombination
{
public:
    explicit Complementary(QObject* parent = nullptr);
    virtual QString name() override;
    virtual QVector<QColor> genColors(const QColor& color) override;
};

class Monochromatic : public ICombination
{
public:
    explicit Monochromatic(QObject* parent = nullptr);
    virtual QString name() override;
    virtual QVector<QColor> genColors(const QColor& color) override;
};

class Analogous : public ICombination
{
public:
    explicit Analogous(QObject* parent = nullptr);
    virtual QString name() override;
    virtual QVector<QColor> genColors(const QColor& color) override;
};

class Triadic : public ICombination
{
public:
    explicit Triadic(QObject* parent = nullptr);
    virtual QString name() override;
    virtual QVector<QColor> genColors(const QColor& color) override;
};

class Tetradic : public ICombination
{
public:
    explicit Tetradic(QObject* parent = nullptr);
    virtual QString name() override;
    virtual QVector<QColor> genColors(const QColor& color) override;
};
} // namespace colorcombination

//-------------------------------------------------- color wheel --------------------------------------------------
class QColorWheel : public QWidget
{
    Q_OBJECT
public:
    explicit QColorWheel(QWidget* parent = nullptr);

    void setColorCombination(colorcombo::ICombination* combination);
    void setSelectedColor(const QColor& color);
    QColor getSelectedColor() const;
    QColor getColor(int x, int y) const;

signals:
    void selectedColorChanged(const QColor& color);
    void combinationColorChanged(const QVector<QColor>& colors);

protected:
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

private:
    void processMouseEvent(QMouseEvent* e);
    void drawColorCircle(QPainter* painter, const QColor& color, int radius);

    class Private;
    std::unique_ptr<Private> p;
};
