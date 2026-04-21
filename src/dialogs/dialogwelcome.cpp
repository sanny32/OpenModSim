#include <QDir>
#include <QFont>
#include <QListWidgetItem>
#include <QCoreApplication>
#include <QStandardPaths>
#include "dialogwelcome.h"
#include "ui_dialogwelcome.h"

namespace {
const char* kDemoPlcSimulator  = "demo_plc_simulator.omp";
const char* kDemoWaveGenerator = "demo_wave_generator.omp";

///
/// \brief findExistingDirectory
/// \param candidates
/// \return
///
QString findExistingDirectory(const QStringList& candidates)
{
    for (const QString& candidate : candidates) {
        if (QDir(candidate).exists())
            return QDir::cleanPath(candidate);
    }

    return {};
}
}

///
/// \brief DialogWelcome::DialogWelcome
/// \param recentProjects list of recent project paths (most recent first)
/// \param parent
///
DialogWelcome::DialogWelcome(const QStringList& recentProjects, QWidget *parent) :
    QFixedSizeDialog(parent),
    ui(new Ui::DialogWelcome)
{
    ui->setupUi(this);
    ui->labelAppName->setText(APP_PRODUCT_NAME);
    setWindowTitle(tr("Welcome to %1").arg(APP_PRODUCT_NAME));

    // Collect all paths: recent projects first, then demos not yet in the list
    QStringList allPaths;
    for (const QString& path : recentProjects) {
        if (QFile::exists(path))
            allPaths.append(path);
    }

    const QString dir = demosDir();
    for (const char* name : { kDemoPlcSimulator, kDemoWaveGenerator }) {
        const QString path = QDir::cleanPath(dir + "/" + name);
        if (!dir.isEmpty() && QFile::exists(path) && !allPaths.contains(path, Qt::CaseInsensitive))
            allPaths.append(path);
    }

    for (const QString& path : allPaths) {
        auto* item = new QListWidgetItem(path, ui->listProjects);
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
    }

    if (ui->listProjects->count() > 0)
        ui->listProjects->setCurrentRow(0);

    ui->buttonOpenProject->setEnabled(ui->listProjects->currentItem() != nullptr);
    connect(ui->listProjects, &QListWidget::itemSelectionChanged, this,
            [this]() {
                ui->buttonOpenProject->setEnabled(!ui->listProjects->selectedItems().isEmpty());
            });
}

///
/// \brief DialogWelcome::~DialogWelcome
///
DialogWelcome::~DialogWelcome()
{
    delete ui;
}

///
/// \brief DialogWelcome::dontShowAgain
/// \return true if the user checked "Don't show this dialog again"
///
bool DialogWelcome::dontShowAgain() const
{
    return ui->checkBoxDontShow->isChecked();
}

///
/// \brief DialogWelcome::demosDir
/// \return path to the demos/projects directory, or empty string if not found
///
QString DialogWelcome::demosDir()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + "/demos/projects",
        appDir + "/../demos/projects",
        appDir + "/../../../demos/projects"
    };

#ifdef Q_OS_LINUX
    candidates.append(appDir + "/../share/omodsim/demos/projects");

    const QString installedDir = QStandardPaths::locate(
        QStandardPaths::GenericDataLocation,
        QStringLiteral("omodsim/demos/projects"),
        QStandardPaths::LocateDirectory
    );
    if (!installedDir.isEmpty())
        candidates.append(installedDir);
#endif

    return findExistingDirectory(candidates);
}

///
/// \brief DialogWelcome::sizeHint
/// \return dialog size enforcing a 4:3 (width:height) aspect ratio
///
QSize DialogWelcome::sizeHint() const
{
    const QSize natural = QFixedSizeDialog::sizeHint();
    const int w = qMax(natural.width(), natural.height() * 4 / 3);
    return { w, w * 3 / 4 };
}

///
/// \brief DialogWelcome::changeEvent
/// \param event
///
void DialogWelcome::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);

    QDialog::changeEvent(event);
}

///
/// \brief DialogWelcome::on_buttonNewProject_clicked
///
void DialogWelcome::on_buttonNewProject_clicked()
{
    _action = Action::NewProject;
    accept();
}

///
/// \brief DialogWelcome::on_buttonOpenProject_clicked
///
void DialogWelcome::on_buttonOpenProject_clicked()
{
    const auto items = ui->listProjects->selectedItems();
    if (!items.isEmpty())
        openFile(items.first()->data(Qt::UserRole).toString());
}

///
/// \brief DialogWelcome::on_listProjects_itemDoubleClicked
/// \param item
///
void DialogWelcome::on_listProjects_itemDoubleClicked(QListWidgetItem* item)
{
    const QString path = item->data(Qt::UserRole).toString();
    if (!path.isEmpty())
        openFile(path);
}

///
/// \brief DialogWelcome::openFile
/// \param path
///
void DialogWelcome::openFile(const QString& path)
{
    _action = Action::OpenFile;
    _filePath = path;
    accept();
}
