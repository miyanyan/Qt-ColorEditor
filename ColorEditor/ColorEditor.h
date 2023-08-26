#pragma once

#include <memory>

#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
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
    virtual QString name();
    virtual QVector<QColor> genColors(const QColor& color);
    void setFactorRange(double min, double max);
    void serFactorValue(double value);
    // factor range: [0, 1]
    double getFactor() const;

private:
    double m_min;
    double m_max;
    double m_value;
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
} // namespace colorcombo

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

signals:
    void currentColorChanged(const QColor& color);

private:
    class Private;
    std::unique_ptr<Private> p;
};

//--------------------------------------------- color button -------------------------------------------------------
class ColorButton : public QPushButton
{
    Q_OBJECT
public:
    explicit ColorButton(QWidget* parent);
    void setColor(const QColor& color);
    void setBolderWidth(int width);

signals:
    void colorSelected(const QColor& color);

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragLeaveEvent(QDragLeaveEvent*) override;
    void dropEvent(QDropEvent* e) override;

private:
    class Private;
    std::unique_ptr<Private> p;
};

//--------------------------------------------- color palette ------------------------------------------------------
class ColorPalette : public QScrollArea
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

//--------------------------------------------- color preview -------------------------------------------------------
class ColorPreview : public QWidget
{
    Q_OBJECT
public:
    explicit ColorPreview(const QColor& color, QWidget* parent = nullptr);
    void setCurrentColor(const QColor& color);
    QColor currentColor() const;
    QColor previousColor() const;

signals:
    void currentColorChanged(const QColor& color);

private:
    class Private;
    std::unique_ptr<Private> p;
};
