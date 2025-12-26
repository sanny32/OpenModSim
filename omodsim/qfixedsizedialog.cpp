#include "qfixedsizedialog.h"

///
/// \brief QFixedSizeDialog::QFixedSizeDialog
/// \param parent
/// \param f
///
QFixedSizeDialog::QFixedSizeDialog(QWidget *parent, Qt::WindowFlags f)
    :QDialog(parent, f)
{
    setWindowModality(Qt::WindowModal);
}

///
/// \brief QFixedSizeDialog::showEvent
/// \param e
///
void QFixedSizeDialog::showEvent(QShowEvent* e)
{
    QDialog::showEvent(e);
    setFixedSize(sizeHint());
}
