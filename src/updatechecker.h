#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QVersionNumber>

///
/// \brief The UpdateChecker class
///
class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(QObject* parent = nullptr);

    void checkForUpdates();
    void setEnabled(bool enabled);

    QString latestVersion() const { return _latestVersion; }
    QString releaseUrl() const { return _releaseUrl; }
    bool hasNewVersion() const { return _hasNewVersion; }

signals:
    void newVersionAvailable(const QString& version, const QString& url);

private:
    void onReplyFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager* _networkManager;
    QTimer* _checkTimer;
    QString _latestVersion;
    QString _releaseUrl;
    bool _hasNewVersion = false;
};

#endif // UPDATECHECKER_H
