#ifndef COLORSWATCH_H
#define COLORSWATCH_H

#include <QPushButton>
#include <QPainter>

///
/// \brief The ColorSwatch class
///
/// A clickable flat colored rectangle button. Uses custom paintEvent
/// to render a solid color fill with a border — no stylesheet needed.
///
class ColorSwatch : public QPushButton
{
public:
    explicit ColorSwatch(QWidget* parent = nullptr) : QPushButton(parent)
    {
        setFixedSize(56, 22);
        setCursor(Qt::PointingHandCursor);
    }

    void setColor(const QColor& c) { _color = c; update(); }
    QColor color() const           { return _color; }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.fillRect(rect(), _color);
        p.setPen(palette().color(QPalette::Shadow));
        p.drawRect(rect().adjusted(0, 0, -1, -1));
    }

private:
    QColor _color;
};

#endif // COLORSWATCH_H

