#ifndef FINDREPLACEBAR_H
#define FINDREPLACEBAR_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>

///
/// \brief The FindReplaceBar class
///
class FindReplaceBar : public QWidget
{
    Q_OBJECT

public:
    explicit FindReplaceBar(QWidget *parent = nullptr);

    QString searchText() const;
    QString replaceText() const;

    void updateMatchCount(int current, int total);
    void updatePosition();

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
    void onClose();

private:
    QLineEdit* _searchEdit;
    QLineEdit* _replaceEdit;
    QPushButton* _prevButton;
    QPushButton* _nextButton;
    QPushButton* _replaceButton;
    QPushButton* _replaceAllButton;
    QToolButton* _closeButton;
    QLabel* _matchCountLabel;
    QWidget* _replaceRow;
};

#endif // FINDREPLACEBAR_H
