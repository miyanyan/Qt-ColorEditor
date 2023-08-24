#pragma once

#include <memory>

#include <QWidget>
#include <QSlider>

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
class ColorWheel : public QWidget
{
    Q_OBJECT
public:
    explicit ColorWheel(QWidget* parent = nullptr);

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
    void drawSelector(QPainter* painter, const QColor& color, int radius);

    class Private;
    std::unique_ptr<Private> p;
};

//---------------------------------------------- color slider -------------------------------------------------------
class ColorSlider : public QSlider
{
    Q_OBJECT
public:
    explicit ColorSlider(QWidget* parent = nullptr);
    void setGradient(const QColor& startColor, const QColor& stopColor);
    QColor startColor() const;
    QColor stopColor() const;

private:
    class Private;
    std::unique_ptr<Private> p;
};

//--------------------------------------------- color palette ------------------------------------------------------
class ColorPalette : public QWidget
{
    Q_OBJECT
public:
    explicit ColorPalette(int column, QWidget* parent = nullptr);
    void addColor(const QColor& color);
    void setColor(const QColor& color, int row, int column);
    void removeColor(const QColor& color, int row, int column);

signals:
    void colorSelected(const QColor& color);

private:
    class Private;
    std::unique_ptr<Private> p;
};
