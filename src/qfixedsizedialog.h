#ifndef QFIXEDSIZEDIALOG_H
#define QFIXEDSIZEDIALOG_H

#include <QDialog>

///
/// \brief The QFixedSizeDialog class
///
class QFixedSizeDialog : public QDialog
{
    Q_OBJECT

public:
    QFixedSizeDialog(QWidget *parent = nullptr, Qt::WindowFlags f =
                     Qt::Dialog |
                     Qt::CustomizeWindowHint |
                     Qt::WindowCloseButtonHint |
                     Qt::WindowTitleHint);

    QSize sizeHint() const override;

protected:
    void showEvent(QShowEvent* e) override;

private:
    bool _shown = false;
};

#endif // QFIXEDSIZEDIALOG_H

