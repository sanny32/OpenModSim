#include "searchlineedit.h"

///
/// \brief SearchLineEdit::SearchLineEdit
/// \param parent
///
SearchLineEdit::SearchLineEdit(QWidget* parent)
    :QLineEdit(parent)
{
    setPlaceholderText(tr("Type to search..."));
    setClearButtonEnabled(true);
    setMaximumWidth(200);

    connect(this, &QLineEdit::returnPressed, this, &SearchLineEdit::on_returnPressed);
}

///
/// \brief SearchLineEdit::on_returnPressed
///
void SearchLineEdit::on_returnPressed()
{
    emit searchText(text());
}
