#ifndef DIALOGABOUT_H
#define DIALOGABOUT_H

#include "qfixedsizedialog.h"

namespace Ui {
class DialogAbout;
}

///
/// \brief The DialogAbout class
///
class DialogAbout : public QFixedSizeDialog
{
    Q_OBJECT

public:
    explicit DialogAbout(QWidget *parent = nullptr);
    ~DialogAbout();

    QSize sizeHint() const override;

private slots:
    void on_labelLicense_clicked();

private:
    Ui::DialogAbout *ui;
};

#endif // DIALOGABOUT_H
