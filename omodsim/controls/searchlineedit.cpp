#include <QEvent>
#include "searchlineedit.h"

///
/// \brief SearchLineEdit::SearchLineEdit
/// \param parent
///
SearchLineEdit::SearchLineEdit(QWidget* parent)
    :QLineEdit(parent)
{
    setPlaceholderText(tr("Type text to search..."));
    setClearButtonEnabled(true);
    setMaximumWidth(200);

    connect(this, &QLineEdit::textEdited, this, &SearchLineEdit::on_textEdited);
    connect(this, &QLineEdit::returnPressed, this, &SearchLineEdit::on_returnPressed);
}

///
/// \brief SearchLineEdit::changeEvent
/// \param event
///
void SearchLineEdit::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        setPlaceholderText(tr("Type text to search..."));
    }

    QLineEdit::changeEvent(event);
}

///
/// \brief SearchLineEdit::on_returnPressed
///
void SearchLineEdit::on_returnPressed()
{
    emit searchText(text());
}

///
/// \brief SearchLineEdit::on_textEdited
/// \param text
///
void SearchLineEdit::on_textEdited(const QString& text)
{
    emit searchText(text);
}
