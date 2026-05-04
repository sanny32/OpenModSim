#include <QFile>
#include <QApplication>
#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPlainTextEdit>
#include "aboutdatawidget.h"
#include "dialogabout.h"
#include "ui_dialogabout.h"

#ifdef Q_OS_WIN
#include <windows.h>

typedef LONG (WINAPI *RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

///
/// \brief windowsPrettyName
/// \return
///
QString windowsPrettyName()
{
    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if (!hMod)
        return "Windows";

    auto rtlGetVersion =
        reinterpret_cast<RtlGetVersionPtr>(
            ::GetProcAddress(hMod, "RtlGetVersion"));

    if (!rtlGetVersion)
        return "Windows";

    RTL_OSVERSIONINFOW rovi = {};
    rovi.dwOSVersionInfoSize = sizeof(rovi);

    if (rtlGetVersion(&rovi) != 0)
        return "Windows";

    const DWORD build = rovi.dwBuildNumber;

    QString name;
    if (rovi.dwMajorVersion == 10 && build >= 22000)
        name = "Windows 11";
    else if (rovi.dwMajorVersion == 10)
        name = "Windows 10";
    else
        name = QString("Windows %1").arg(rovi.dwMajorVersion);

    return DialogAbout::tr("%1 build %2").arg(name, QString::number(build));
}
#endif

///
/// \brief arch
/// \return
///
QString arch()
{
#if defined(Q_PROCESSOR_X86_64)
    return "x64";
#elif defined(Q_PROCESSOR_X86)
    return "x86";
#elif defined(Q_PROCESSOR_ARM)
    return "ARM";
#elif defined(Q_PROCESSOR_ARM64)
    return "ARM64";
#else
    return DialogAbout::tr("Unknown");
#endif
}

///
/// \brief aboutData
/// \return
///
QJsonObject aboutData()
{
    QFile f(":/res/about.json");
    if (!f.open(QFile::ReadOnly))
        return {};

    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(f.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
        return {};

    return doc.object();
}

///
/// \brief roleDescription
/// \param role
/// \return
///
QString roleDescription(const QString& role)
{
    if (role == "author_maintainer")
        return DialogAbout::tr("Author and Maintainer");

    return DialogAbout::tr("Contributor");
}

///
/// \brief languageName
/// \param language
/// \return
///
QString languageName(const QString& language)
{
    if (language == "russian")
        return DialogAbout::tr("Russian");
    if (language == "simplified_chinese")
        return DialogAbout::tr("Simplified Chinese");
    if (language == "traditional_chinese")
        return DialogAbout::tr("Traditional Chinese");

    return language;
}

///
/// \brief languagesDescription
/// \param languages
/// \return
///
QString languagesDescription(const QJsonArray& languages)
{
    QStringList names;
    for (const auto& value : languages) {
        const auto language = value.toString();
        if (!language.isEmpty())
            names.append(languageName(language));
    }

    if (names.size() == 2)
        return DialogAbout::tr("%1 and %2").arg(names.at(0), names.at(1));

    return names.join(", ");
}

///
/// \brief DialogAbout::DialogAbout
/// \param parent
///
DialogAbout::DialogAbout(QWidget *parent) :
    QFixedSizeDialog(parent),
    ui(new Ui::DialogAbout),
    _updateChecker(new UpdateChecker(this))
{
    ui->setupUi(this);
    setWindowTitle(tr("About %1...").arg(APP_PRODUCT_NAME));

    ui->labelName->setText(APP_PRODUCT_NAME);
    ui->labelVersion->setText(tr("Version: <b>%1</b> %2").arg(APP_VERSION, arch()));

    const auto copyright = QString(ui->labelCopyright->text()).arg(BUILD_YEAR);
    ui->labelCopyright->setText(copyright);

    connect(_updateChecker, &UpdateChecker::checkStarted, this, &DialogAbout::onUpdateCheckStarted);
    connect(_updateChecker, &UpdateChecker::noUpdatesAvailable, this, &DialogAbout::onNoUpdatesAvailable);
    connect(_updateChecker, &UpdateChecker::checkFailed, this, &DialogAbout::onUpdateCheckFailed);
    connect(_updateChecker, &UpdateChecker::newVersionAvailable, this, &DialogAbout::onNewVersionAvailable);

    {
        auto vboxLayout = new QVBoxLayout();
        vboxLayout->setContentsMargins(0, 0, 0, 0);

        addComponent(vboxLayout,
                    "Qt",
                    tr("Using %1 and built against %2").arg(qVersion(), QT_VERSION_STR),
                    tr("Cross-platform application development framework."),
                    "https://www.qt.io");

        addComponent(vboxLayout,
                     "Fira Code",
                     "6.2",
                     tr("Free monospaced font with programming ligatures."),
                     "https://github.com/tonsky/FiraCode");

    #ifdef Q_OS_LINUX
        addComponent(vboxLayout,
                     QSysInfo::prettyProductName(),
                     QApplication::platformName(),
                     tr("Underlying platform."));
    #endif

    #ifdef Q_OS_WIN
        addComponent(vboxLayout,
                     windowsPrettyName(),
                     QSysInfo::currentCpuArchitecture(),
                     tr("Underlying platform."));
    #endif

    #ifdef Q_OS_MAC
        addComponent(vboxLayout,
                     QSysInfo::prettyProductName(),
                     QSysInfo::currentCpuArchitecture(),
                     tr("Underlying platform."));
    #endif

        vboxLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
        ui->scrollAreaComponentsWidget->setLayout(vboxLayout);
    }

    loadAuthors();
    loadTranslators();

    adjustSize();
    ui->tabWidget->setCurrentIndex(0);
}

///
/// \brief DialogAbout::~DialogAbout
///
DialogAbout::~DialogAbout()
{
    delete ui;
}

///
/// rief DialogAbout::changeEvent
///
///
/// \brief DialogAbout::changeEvent
///
void DialogAbout::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);

    QDialog::changeEvent(event);
}

///
/// \brief DialogAbout::adjustSize
///
void DialogAbout::adjustSize()
{
    ensurePolished();

    auto s = style();
    const int sbWidth = s->pixelMetric(QStyle::PM_ScrollBarExtent);
    const int frameWidth = s->pixelMetric(QStyle::PM_DefaultFrameWidth) * 2;

    QSize maxContentSize(0, 0);
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        auto tab = ui->tabWidget->widget(i);
        auto scroll = tab->findChild<QScrollArea*>();

        if (scroll && scroll->widget()) {
            if(scroll->widget()->layout()) {
                scroll->widget()->layout()->activate();
            }

            int marginH = 0;
            int marginV = 0;
            if (tab->layout()) {
                marginH = tab->layout()->contentsMargins().left() + tab->layout()->contentsMargins().right();
                marginV = tab->layout()->contentsMargins().top() + tab->layout()->contentsMargins().bottom();
            }

            QSize contentSize = scroll->widget()->sizeHint();
            contentSize.rwidth() += marginH + sbWidth + frameWidth;
            contentSize.rheight() += marginV + ui->tabWidget->tabBar()->sizeHint().height() + frameWidth;

            maxContentSize = maxContentSize.expandedTo(contentSize);
        }
    }

    int tabBarWidth = ui->tabWidget->tabBar()->sizeHint().width();
    tabBarWidth += 40 + s->pixelMetric(QStyle::PM_TabBarBaseOverlap);

    if (maxContentSize.width() < tabBarWidth) {
        maxContentSize.setWidth(tabBarWidth);
    }

    ui->tabWidget->setMinimumSize(maxContentSize);
    QFixedSizeDialog::adjustSize();
}

///
/// \brief DialogAbout::loadAuthors
///
void DialogAbout::loadAuthors()
{
    auto vboxLayout = new QVBoxLayout();
    vboxLayout->setContentsMargins(0, 0, 0, 0);

    const auto data = aboutData();
    const QStringList sections = { "authors", "contributors" };
    for (const auto& sectionName : sections) {
        const auto people = data.value(sectionName).toArray();
        for (const auto& value : people) {
            const auto person = value.toObject();
            const auto name = person.value("name").toString();
            if (name.isEmpty())
                continue;

            const auto role = person.value("role").toString();
            const auto description = roleDescription(role);
            const auto url = person.value("url").toString();
            addAuthor(vboxLayout, name, description, url);
        }
    }

    vboxLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    ui->scrollAreaAuthorsWidget->setLayout(vboxLayout);
}

///
/// \brief DialogAbout::loadTranslators
///
void DialogAbout::loadTranslators()
{
    auto vboxLayout = new QVBoxLayout();
    vboxLayout->setContentsMargins(0, 0, 0, 0);

    const auto data = aboutData();
    const auto translators = data.value("translators").toArray();
    for (const auto& value : translators) {
        const auto person = value.toObject();
        const auto name = person.value("name").toString();
        if (name.isEmpty())
            continue;

        const auto description = languagesDescription(person.value("languages").toArray());
        const auto url = person.value("url").toString();
        addAuthor(vboxLayout, name, description, url);
    }

    vboxLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    ui->scrollAreaTranslationWidget->setLayout(vboxLayout);
}

///
/// \brief DialogAbout::addComponent
/// \param layout
/// \param title
/// \param version
/// \param description
/// \param url
///
///
/// \brief DialogAbout::addComponent
///
void DialogAbout::addComponent(QLayout* layout, const QString& title, const QString& version, const QString& description, const QString& url)
{
    auto w = new AboutDataWidget(this);
    w->setTitle(title);
    w->setVersion(version);
    w->setDescription(description);
    w->setLinkUrl(QUrl(url));
    w->setLinkIcon(QIcon::fromTheme("applications-internet", QIcon(":/res/applications-internet.svg")));
    w->setLinkToolTip(tr("Visit component's homepage\n%1").arg(w->linkUrl().toString()));
    layout->addWidget(w);
}

///
/// \brief DialogAbout::addAuthor
/// \param layout
/// \param name
/// \param description
/// \param url
///
void DialogAbout::addAuthor(QLayout* layout, const QString& name, const QString& description, const QString& url)
{
    auto w = new AboutDataWidget(this);
    w->setTitle(name);
    w->setDescription(description);
    w->setLinkUrl(QUrl(url));

    if(url.contains("mailto:"))
    {
        w->setLinkIcon(QIcon::fromTheme("emblem-mail", QIcon(":/res/emblem-mail.svg")));
        w->setLinkToolTip(tr("Email contributer: %1").arg(w->linkUrl().path()));
    }
    else if(url.contains("github"))
    {
        w->setLinkIcon(QIcon(":/res/emblem-github.svg"));
        w->setLinkToolTip(tr("Visit github user's homepage\n%1").arg(w->linkUrl().toString()));
    }
    else if(!url.isEmpty())
    {
        w->setLinkToolTip(tr("Visit user's homepage\n%1").arg(w->linkUrl().toString()));
    }

    layout->addWidget(w);
}

///
/// \brief DialogAbout::on_labelLicense_clicked
///
void DialogAbout::on_labelLicense_clicked()
{
    QString license;
    QFile f(":/res/license.txt");
    if(f.open(QFile::ReadOnly)) {
        license = f.readAll();
    }

    if(license.isEmpty()) {
        return;
    }

    auto dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    dlg->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    dlg->setWindowTitle(QString(tr("License Agreement - %1")).arg(APP_PRODUCT_NAME));
    dlg->resize({ 530, 415});

    auto buttonBox = new QDialogButtonBox(dlg);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Close);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, dlg, &DialogAbout::reject);

    auto textEdit = new QPlainTextEdit(dlg);
    textEdit->setReadOnly(true);
    textEdit->setPlainText(license);

    auto layout = new QVBoxLayout(dlg);
    layout->addWidget(textEdit);
    layout->addWidget(buttonBox);

    dlg->setLayout(layout);
    dlg->show();
}

///
/// \brief DialogAbout::on_buttonCheckForUpdates_clicked
///
void DialogAbout::on_buttonCheckForUpdates_clicked()
{
    _updateChecker->checkForUpdates();
}

///
/// \brief DialogAbout::onUpdateCheckStarted
///
void DialogAbout::onUpdateCheckStarted()
{
    ui->buttonCheckForUpdates->setEnabled(false);
    ui->buttonCheckForUpdates->setText(tr("Checking..."));
}

///
/// \brief DialogAbout::onNoUpdatesAvailable
///
void DialogAbout::onNoUpdatesAvailable()
{
    ui->buttonCheckForUpdates->setEnabled(true);
    ui->buttonCheckForUpdates->setText(tr("Check for updates"));
    QMessageBox::information(this, tr("Check for updates"), tr("No updates available."));
}

///
/// \brief DialogAbout::onUpdateCheckFailed
/// \param errorString
///
void DialogAbout::onUpdateCheckFailed(const QString& errorString)
{
    ui->buttonCheckForUpdates->setEnabled(true);
    ui->buttonCheckForUpdates->setText(tr("Check for updates"));
    QMessageBox::warning(this,
                         tr("Check for updates"),
                         tr("Failed to check for updates.\n\n%1").arg(errorString));
}

///
/// \brief DialogAbout::onNewVersionAvailable
/// \param version
/// \param url
///
void DialogAbout::onNewVersionAvailable(const QString& version, const QString& url)
{
    ui->buttonCheckForUpdates->setEnabled(true);
    ui->buttonCheckForUpdates->setText(tr("Check for updates"));

    const auto answer = QMessageBox::question(this,
                                              tr("New version available"),
                                              tr("A new version %1 is available.\n\nOpen the download page?")
                                                  .arg(version),
                                              QMessageBox::Yes | QMessageBox::No,
                                              QMessageBox::Yes);

    if(answer == QMessageBox::Yes)
        QDesktopServices::openUrl(QUrl(url));
}
