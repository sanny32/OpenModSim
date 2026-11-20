// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file test_projecttreewidget.cpp
/// \brief Unit tests for project tree item presentation.
///

#include <QImage>
#include <QPixmap>
#include <QTest>

#include "application.h"
#include "style/qlementineappstyle.h"
#include "widgets/projecttreewidget.h"

namespace {
///
/// \brief Returns the device-pixel rectangle containing all non-transparent pixels.
///
QRect visibleBounds(const QPixmap& pixmap)
{
    const QImage image = pixmap.toImage();
    int left = image.width();
    int top = image.height();
    int right = -1;
    int bottom = -1;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y).alpha() == 0)
                continue;
            left = qMin(left, x);
            top = qMin(top, y);
            right = qMax(right, x);
            bottom = qMax(bottom, y);
        }
    }
    return right < 0 ? QRect{} : QRect(left, top, right - left + 1, bottom - top + 1);
}
}

class TestProjectTreeWidget : public QObject
{
    Q_OBJECT

private slots:
    void closedFormIconPreservesHighDpiContent();
};

void TestProjectTreeWidget::closedFormIconPreservesHighDpiContent()
{
    ProjectTreeWidget tree;
    QWidget form;
    tree.addForm(ProjectFormType::Data, &form);

    auto* root = tree.topLevelItem(0);
    QVERIFY(root);
    QCOMPARE(root->childCount(), 1);
    auto* item = root->child(0);
    const QPixmap openPixmap = item->icon(0).pixmap(QSize(16, 16));
    if (openPixmap.devicePixelRatio() != 2.0)
        QSKIP("High-DPI themed icons are not available");

    QCOMPARE(openPixmap.size(), QSize(32, 32));
    const QRect openBounds = visibleBounds(openPixmap);
    QVERIFY(!openBounds.isEmpty());

    tree.setFormOpen(&form, false);
    const QPixmap closedPixmap = item->icon(0).pixmap(QSize(16, 16));
    QVERIFY(!closedPixmap.isNull());
    QCOMPARE(closedPixmap.devicePixelRatio(), openPixmap.devicePixelRatio());
    QCOMPARE(closedPixmap.size(), openPixmap.size());
    QCOMPARE(visibleBounds(closedPixmap), openBounds);
}

int main(int argc, char** argv)
{
    qputenv("QT_SCALE_FACTOR", "2");
    Application app(argc, argv);
#if defined(HAVE_QLEMENTINE_APP_STYLE)
    app.setStyle(new QlementineAppStyle(&app));
#endif
    TestProjectTreeWidget test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_projecttreewidget.moc"
