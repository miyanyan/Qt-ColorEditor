#pragma once

#include <memory>

#include <QDialog>
#include <QLineEdit>
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
    explicit ICombination(double min, double max, double value, bool rangeEnabled, QObject* parent = nullptr);
    virtual ~ICombination() = default;
    virtual QString name();
    virtual QVector<QColor> genColors(const QColor& color);
    void setRange(double min, double max);
    void serValue(double value);
    double min() const;
    double max() const;
    double getValue() const;
    bool rangeEnabled() const;

private:
    double m_min;
    double m_max;
    double m_value;
    bool m_rangeEnabled;
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
    void colorSelected(const QColor& color);
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
    void setGradient(const QVector<QPair<float, QColor>>& colors);
    QVector<QPair<float, QColor>> gradientColor() const;

signals:
    void currentColorChanged(const QColor& color);

private:
    class Private;
    std::unique_ptr<Private> p;
};

class ColorSpinHSlider : public QWidget
{
    Q_OBJECT
public:
    explicit ColorSpinHSlider(const QString& name, QWidget* parent);
    void setGradient(const QColor& startColor, const QColor& stopColor);
    void setGradient(const QVector<QPair<float, QColor>>& colors);
    void setValue(double value);
    void setRange(double min, double max);
    QVector<QPair<float, QColor>> gradientColor() const;

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
    QColor color() const;

signals:
    void colorClicked(const QColor& color);
    void colorDroped(const QColor& color);

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
    void colorClicked(const QColor& color);

protected:
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;

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

//------------------------------------------- color combo widget ---------------------------
class ColorComboWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ColorComboWidget(QWidget* parent = nullptr);

    void addCombination(colorcombo::ICombination* combo);
    void switchCombination();
    void setColors(const QVector<QColor>& colors);

signals:
    void colorClicked(const QColor& color);
    void combinationChanged(colorcombo::ICombination* combo);

private:
    class Private;
    std::unique_ptr<Private> p;
};

//------------------------------------------ color lineedit --------------------------------
class ColorLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit ColorLineEdit(QWidget* parent = nullptr);
    void setColor(const QColor& color);

signals:
    void currentColorChanged(const QColor& color);
};

//------------------------------------------ color editor ----------------------------------
class ColorEditor : public QDialog
{
    Q_OBJECT
public:
    explicit ColorEditor(const QColor& color = Qt::white, QWidget* parent = nullptr);
    ~ColorEditor();

    void setCurrentColor(const QColor& color);
    QColor currentColor() const;

signals:
    void currentColorChanged(const QColor& color);

private:
    class Private;
    std::unique_ptr<Private> p;
};