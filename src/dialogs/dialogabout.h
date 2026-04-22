#ifndef DIALOGABOUT_H
#define DIALOGABOUT_H

#include "qfixedsizedialog.h"
#include "updatechecker.h"

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

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_labelLicense_clicked();
    void on_buttonCheckForUpdates_clicked();

private:
    void adjustSize();
    void addComponent(QLayout* layout, const QString& title, const QString& version, const QString& description, const QString& url = QString());
    void addAuthor(QLayout* layout, const QString& name, const QString& description, const QString& url = QString());
    void onUpdateCheckStarted();
    void onNoUpdatesAvailable();
    void onUpdateCheckFailed(const QString& errorString);
    void onNewVersionAvailable(const QString& version, const QString& url);

private:
    Ui::DialogAbout *ui;
    UpdateChecker* _updateChecker;
};

#endif // DIALOGABOUT_H

