#include <QJsonDocument>
#include <QJsonObject>
#include <QSslSocket>
#include <QVersionNumber>
#include "updatechecker.h"
#include "apppreferences.h"

///
/// \brief UpdateChecker::UpdateChecker
/// \param parent
///
UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , _networkManager(new QNetworkAccessManager(this))
    , _checkTimer(new QTimer(this))
{
    connect(_networkManager, &QNetworkAccessManager::finished, this, &UpdateChecker::onReplyFinished);

    // Check every 6 hours
    _checkTimer->setInterval(6 * 60 * 60 * 1000);
    connect(_checkTimer, &QTimer::timeout, this, &UpdateChecker::checkForUpdates);

    if(AppPreferences::instance().checkForUpdates() && QSslSocket::supportsSsl())
        _checkTimer->start();
}

///
/// \brief UpdateChecker::setEnabled
/// \param enabled
///
void UpdateChecker::setEnabled(bool enabled)
{
    if(!QSslSocket::supportsSsl())
    {
        _checkTimer->stop();
        return;
    }

    if(enabled)
    {
        _checkTimer->start();
        if(!_hasNewVersion)
            checkForUpdates();
    }
    else
    {
        _checkTimer->stop();
    }
}

///
/// \brief UpdateChecker::checkForUpdates
///
void UpdateChecker::checkForUpdates()
{
    if(!QSslSocket::supportsSsl())
    {
        _checkTimer->stop();
        return;
    }

    QNetworkRequest request(QUrl("https://api.github.com/repos/sanny32/OpenModSim/releases/latest"));
    request.setHeader(QNetworkRequest::UserAgentHeader, "OpenModSim");
    request.setRawHeader("Accept", "application/vnd.github.v3+json");
    _networkManager->get(request);
}

///
/// \brief UpdateChecker::onReplyFinished
/// \param reply
///
void UpdateChecker::onReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if(reply->error() != QNetworkReply::NoError)
        return;

    const auto data = reply->readAll();
    const auto doc = QJsonDocument::fromJson(data);
    if(!doc.isObject())
        return;

    const auto obj = doc.object();
    const auto tagName = obj["tag_name"].toString();
    const auto htmlUrl = obj["html_url"].toString();

    // Strip leading 'v' or 'V' from tag
    auto versionStr = tagName;
    if(versionStr.startsWith('v', Qt::CaseInsensitive))
        versionStr = versionStr.mid(1);

    const auto remoteVersion = QVersionNumber::fromString(versionStr);
    const auto currentVersion = QVersionNumber::fromString(APP_VERSION);

    if(!remoteVersion.isNull() && remoteVersion > currentVersion)
    {
        _latestVersion = versionStr;
        _releaseUrl = htmlUrl;
        _hasNewVersion = true;
        emit newVersionAvailable(versionStr, htmlUrl);
    }
}
