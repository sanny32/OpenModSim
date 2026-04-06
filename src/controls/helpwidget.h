#ifndef HELPWIDGET_H
#define HELPWIDGET_H

#include <QHelpEngine>
#include <QLineEdit>
#include <QLabel>
#include "helpbrowser.h"

///
/// \brief The HelpWidget class
///
class HelpWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HelpWidget(QWidget* parent = nullptr);

    void setHelp(const QString& helpFile);
    void showHelp(const QString& helpKey);
    void showFind();

protected:
    void changeEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void onFindNext();
    void onFindPrevious();
    void onSearchTextEdited(const QString& text);
    void onClose();

private:
    HelpBrowser* _helpBrowser;
    QWidget*     _findBar;
    QLineEdit*   _searchEdit;
    QPushButton* _prevButton;
    QPushButton* _nextButton;
    QToolButton* _closeButton;
    QLabel*      _matchCountLabel;
};

#endif // HELPWIDGET_H

