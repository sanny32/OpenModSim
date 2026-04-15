#ifndef DIALOGWELCOME_H
#define DIALOGWELCOME_H

#include "qfixedsizedialog.h"

namespace Ui {
class DialogWelcome;
}

///
/// \brief The DialogWelcome class
///
/// Shown on startup when no last project is available, as long as the user
/// hasn't disabled it via the "Don't show this dialog again" checkbox.
///
class DialogWelcome : public QFixedSizeDialog
{
    Q_OBJECT

public:
    enum class Action {
        NewProject,
        OpenProject,  // show file-open dialog
        OpenFile      // open _filePath (demo or recent project)
    };

    explicit DialogWelcome(const QStringList& recentProjects, QWidget *parent = nullptr);
    ~DialogWelcome();

    Action action() const { return _action; }
    QString filePath() const { return _filePath; }
    bool dontShowAgain() const;

    static QString demosDir();

    QSize sizeHint() const override;

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_buttonNewProject_clicked();
    void on_buttonOpenProject_clicked();
    void on_listProjects_itemDoubleClicked(QListWidgetItem* item);

private:
    void openFile(const QString& path);

private:
    Ui::DialogWelcome *ui;
    Action _action = Action::NewProject;
    QString _filePath;
};

#endif // DIALOGWELCOME_H
