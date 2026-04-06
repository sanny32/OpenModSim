#ifndef FINDREPLACEBAR_H
#define FINDREPLACEBAR_H

#include <QFrame>
#include <QTextDocument>
#include <QKeyEvent>

namespace Ui {
class FindReplaceBar;
}

///
/// \brief The FindReplaceBar class
///
class FindReplaceBar : public QFrame
{
    Q_OBJECT

public:
    explicit FindReplaceBar(QWidget *parent = nullptr);
    ~FindReplaceBar();

    QString searchText() const;
    QString replaceText() const;

    QTextDocument::FindFlags findFlags() const;

    void updatePosition();
    void setReplaceEnabled(bool enabled);
    bool isReplaceEnabled() const;
    void setSearchOptionsVisible(bool visible);
    bool isSearchOptionsVisible() const;
    void setWindowedMode(bool on);
    bool isWindowedMode() const;
    void setUserMoved(bool moved);

public slots:
    void showFind(const QString& selectedText = QString());
    void showReplace(const QString& selectedText = QString());

signals:
    void findNext(const QString& text);
    void findPrevious(const QString& text);
    void replaceRequested(const QString& text, const QString& replacement);
    void replaceAllRequested(const QString& text, const QString& replacement);
    void searchTextChanged(const QString& text);
    void closed();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onFindNext();
    void onFindPrevious();
    void onReplace();
    void onReplaceAll();
    void onSearchTextEdited(const QString& text);
    void onOptionsChanged();
    void onToggleReplace();
    void onClose();

private:
    void setReplaceVisible(bool visible);

private:
    Ui::FindReplaceBar* ui;
    bool _replaceEnabled = true;
    bool _searchOptionsVisible = true;
    bool _windowedMode = false;
    bool _userMoved = false;
    QWidget* _titleBar = nullptr;
};

#endif // FINDREPLACEBAR_H

