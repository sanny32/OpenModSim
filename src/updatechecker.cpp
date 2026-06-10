// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file updatechecker.cpp
/// \brief Implements the updatechecker functionality.
///

#include <QJsonDocument>
#include <QJsonObject>
#include <QSslSocket>
#include <QVersionNumber>
#include "updatechecker.h"
#include "apppreferences.h"

namespace {

// Parsed version with pre-release suffix support.
// Suffix rank: 4=stable, 3=rc, 2=beta, 1=alpha, 0=dev/unknown
struct ParsedVersion {
    QVersionNumber numeric;
    int suffixRank = 4;
    int suffixNum = 0;

    bool isNull() const { return numeric.isNull(); }

    bool operator>(const ParsedVersion& other) const {
        if(numeric != other.numeric) return numeric > other.numeric;
        if(suffixRank != other.suffixRank) return suffixRank > other.suffixRank;
        return suffixNum > other.suffixNum;
    }
};

ParsedVersion parseVersion(const QString& str)
{
    ParsedVersion v;
    const int dash = str.indexOf('-');
    const QString numPart = dash >= 0 ? str.left(dash) : str;
    const QString suffix = dash >= 0 ? str.mid(dash + 1).toLower() : QString();

    v.numeric = QVersionNumber::fromString(numPart);
    if(suffix.isEmpty()) {
        v.suffixRank = 4;
    } else if(suffix.startsWith("rc")) {
        v.suffixRank = 3;
        v.suffixNum = suffix.mid(2).toInt();
    } else if(suffix.startsWith("beta")) {
        v.suffixRank = 2;
        v.suffixNum = suffix.mid(4).toInt();
    } else if(suffix.startsWith("alpha")) {
        v.suffixRank = 1;
        v.suffixNum = suffix.mid(5).toInt();
    } else {
        v.suffixRank = 0;
    }
    return v;
}

} // namespace

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
    if(_isChecking)
        return;

    if(!QSslSocket::supportsSsl())
    {
        _checkTimer->stop();
        emit checkFailed(tr("SSL is not supported in this build."));
        return;
    }

    _isChecking = true;
    emit checkStarted();

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
    _isChecking = false;

    if(reply->error() != QNetworkReply::NoError)
    {
        emit checkFailed(reply->errorString());
        return;
    }

    const auto data = reply->readAll();
    const auto doc = QJsonDocument::fromJson(data);
    if(!doc.isObject())
    {
        emit checkFailed(tr("Failed to parse update information."));
        return;
    }

    const auto obj = doc.object();
    const auto tagName = obj["tag_name"].toString();
    const auto htmlUrl = obj["html_url"].toString();

    if(tagName.isEmpty() || htmlUrl.isEmpty())
    {
        emit checkFailed(tr("Update information is incomplete."));
        return;
    }

    // Strip leading 'v' or 'V' from tag
    auto versionStr = tagName;
    if(versionStr.startsWith('v', Qt::CaseInsensitive))
        versionStr = versionStr.mid(1);

    const auto remoteVersion = parseVersion(versionStr);
    const auto currentVersion = parseVersion(APP_VERSION);

    if(!remoteVersion.isNull() && remoteVersion > currentVersion) {
        _hasNewVersion = true;
        _latestVersion = versionStr;
        _releaseUrl = htmlUrl;
        emit newVersionAvailable(versionStr, htmlUrl);
    }
    else {
        _hasNewVersion = false;
        _latestVersion.clear();
        _releaseUrl.clear();
        emit noUpdatesAvailable();
    }
}

