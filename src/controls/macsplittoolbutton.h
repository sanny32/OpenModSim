// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file macsplittoolbutton.h
/// \brief Declares the macsplittoolbutton interfaces.
///

#ifndef MACSPLITTOOLBUTTON_H
#define MACSPLITTOOLBUTTON_H

#ifdef Q_OS_MAC

#include <QPointer>
#include <QToolButton>

class QTabBar;

///
/// \brief The MacSplitToolButton class
///
class MacSplitToolButton final : public QToolButton
{
public:
    explicit MacSplitToolButton(QWidget* parent = nullptr);

    void setReferenceTabBar(QTabBar* tabBar);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    bool hitButton(const QPoint& pos) const override;

private:
    QRect chromeRect() const;
    static QColor mix(const QColor& a, const QColor& b, qreal amount);

    QPointer<QTabBar> _tabBar;
};

#endif // Q_OS_MAC
#endif // MACSPLITTOOLBUTTON_H
