#include <QFile>
#include <QApplication>
#include <QPlainTextEdit>
#include "componentinfocontrol.h"
#include "dialogabout.h"
#include "ui_dialogabout.h"

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
/// \brief DialogAbout::DialogAbout
/// \param parent
///
DialogAbout::DialogAbout(QWidget *parent) :
    QFixedSizeDialog(parent),
    ui(new Ui::DialogAbout)
{
    ui->setupUi(this);
    setWindowTitle(tr("About %1...").arg(APP_NAME));

    ui->labelName->setText(APP_NAME);
    ui->labelVersion->setText(tr("Version: %1").arg(APP_VERSION));

    /*ui->labelArch->setText(tr("• Architecture: %1").arg(arch()));
    ui->labelPlatform->setText(tr("• Platform: %1 %2").arg(QApplication::platformName(), QSysInfo::currentCpuArchitecture()));
    ui->labelQtFramework->setText(tr("• Qt %1 (build with version %2)").arg(qVersion(), QT_VERSION_STR));
    ui->labelFont->setText(tr("<html><head/><body><span>• Script Font: <a href=\"https://github.com/tonsky/FiraCode\"><span style=\"text-decoration: underline; color:#0000ff;\">Fira Code 6.2</span></a></span></body></html>"));
    */

    {
        auto vboxLayout = new QVBoxLayout();
        vboxLayout->setContentsMargins(0, 0, 0, 0);

        auto qtInfo = new ComponentInfoControl(this);
        qtInfo->setTitle("Qt");
        qtInfo->setVersion(tr("Using %1 and built against %2").arg(qVersion(), QT_VERSION_STR));
        qtInfo->setDescription(tr("Cross-platform application development framework."));
        qtInfo->setLinkUrl(QUrl("https://www.qt.io"));
        qtInfo->setLinkToolTip(tr("Visit component's homepage\n%1").arg(qtInfo->linkUrl().toString()));
        vboxLayout->addWidget(qtInfo);

        auto fontInfo = new ComponentInfoControl(this);
        fontInfo->setTitle("Fira Code");
        fontInfo->setVersion("6.2");
        fontInfo->setDescription(tr("Free monospaced font with programming ligatures."));
        fontInfo->setLinkUrl(QUrl("https://github.com/tonsky/FiraCode"));
        fontInfo->setLinkToolTip(tr("Visit component's homepage\n%1").arg(fontInfo->linkUrl().toString()));
        vboxLayout->addWidget(fontInfo);

        vboxLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
        ui->tabComponents->setLayout(vboxLayout);
    }

    {
        auto vboxLayout = new QVBoxLayout();
        vboxLayout->setContentsMargins(0, 0, 0, 0);

        auto maintainer = new ComponentInfoControl(this);
        maintainer->setTitle("Alexandr Ananev");
        maintainer->setDescription(tr("Maintainer"));
        maintainer->setLinkIcon(QIcon::fromTheme("emblem-mail"));
        maintainer->setLinkUrl(QUrl("mailto: mail@ananev.org"));
        maintainer->setLinkToolTip(tr("Email maintainer: %1").arg(maintainer->linkUrl().path()));
        vboxLayout->addWidget(maintainer);

        auto contributer1 = new ComponentInfoControl(this);
        contributer1->setTitle("Nikolay Raspopov");
        contributer1->setDescription(tr("Contributer"));
        contributer1->setLinkIcon(QIcon::fromTheme("emblem-mail"));
        contributer1->setLinkUrl(QUrl("mailto: raspopov@cherubicsoft.com"));
        contributer1->setLinkToolTip(tr("Email contributor: %1").arg(contributer1->linkUrl().path()));
        vboxLayout->addWidget(contributer1);

        vboxLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
        ui->tabAuthors->setLayout(vboxLayout);
    }

    {
        auto vboxLayout = new QVBoxLayout();
        vboxLayout->setContentsMargins(0, 0, 0, 0);

        auto translator1 = new ComponentInfoControl(this);
        translator1->setTitle("Alexandr Ananev");
        translator1->setDescription(tr("Russian"));
        vboxLayout->addWidget(translator1);

        auto translator2 = new ComponentInfoControl(this);
        translator2->setTitle("CWZ7605");
        translator2->setDescription(tr("Simplified Chinese and Traditional Chinese"));
        translator2->setLinkIcon(QIcon(":/res/github.svg"));
        translator2->setLinkUrl(QUrl("https://github.com/CWZ7605"));
        translator2->setLinkToolTip(tr("Visit github user's homepage\n%1").arg(translator2->linkUrl().toString()));
        vboxLayout->addWidget(translator2);

        vboxLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
        ui->tabTranslation->setLayout(vboxLayout);
    }

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
/// \brief DialogAbout::sizeHint
/// \return
///
QSize DialogAbout::sizeHint() const
{
    return QSize(400, 320);
}

///
/// \brief DialogAbout::on_labelLicense_clicked
///
void DialogAbout::on_labelLicense_clicked()
{
    QString license;
    QFile f(":/res/license.txt");
    if(f.open(QFile::ReadOnly))
        license = f.readAll();

    if(license.isEmpty())
        return;

    auto dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    dlg->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    dlg->setWindowTitle(QString(tr("License Agreement - %1")).arg(APP_NAME));
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
