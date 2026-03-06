#include <QtWidgets>
#include "colorswatch.h"
#include "dialogpreferences.h"
#include "mainwindow.h"
#include "ui_dialogpreferences.h"

///
/// \brief DialogPreferences::DialogPreferences
///
DialogPreferences::DialogPreferences(MainWindow* mainWindow, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::DialogPreferences)
    , _mainWindow(mainWindow)
{
    ui->setupUi(this);

    // Uniform item height
    for (int i = 0; i < ui->listWidget->count(); ++i)
        ui->listWidget->item(i)->setSizeHint(QSize(0, 28));

    // Language items (need userData for findData())
    ui->langCombo->addItem("English",                       "en");
    ui->langCombo->addItem(QString::fromUtf8("Русский"),    "ru");
    ui->langCombo->addItem(QString::fromUtf8("简体中文"),    "zh_CN");
    ui->langCombo->addItem(QString::fromUtf8("繁體中文"),    "zh_TW");

    // Display font: restrict to monospace only
    ui->fontFamilyCombo->setFontFilters(QFontComboBox::MonospacedFonts);
    // Script editor: show all fonts so the bundled Fira Code is always available

    // Color button connections
    connect(ui->bgColorBtn, &QPushButton::clicked, this, [this]() {
        QColorDialog dlg(_bgColor, this);
        if (dlg.exec() == QDialog::Accepted) {
            _bgColor = dlg.currentColor();
            ui->bgColorBtn->setColor(_bgColor);
        }
    });
    connect(ui->bgResetBtn, &QPushButton::clicked, this, [this]() {
        _bgColor = Qt::white;
        ui->bgColorBtn->setColor(_bgColor);
    });

    connect(ui->fgColorBtn, &QPushButton::clicked, this, [this]() {
        QColorDialog dlg(_fgColor, this);
        if (dlg.exec() == QDialog::Accepted) {
            _fgColor = dlg.currentColor();
            ui->fgColorBtn->setColor(_fgColor);
        }
    });
    connect(ui->fgResetBtn, &QPushButton::clicked, this, [this]() {
        _fgColor = Qt::black;
        ui->fgColorBtn->setColor(_fgColor);
    });

    connect(ui->statusColorBtn, &QPushButton::clicked, this, [this]() {
        QColorDialog dlg(_statusColor, this);
        if (dlg.exec() == QDialog::Accepted) {
            _statusColor = dlg.currentColor();
            ui->statusColorBtn->setColor(_statusColor);
        }
    });
    connect(ui->statusResetBtn, &QPushButton::clicked, this, [this]() {
        _statusColor = Qt::red;
        ui->statusColorBtn->setColor(_statusColor);
    });

    connect(ui->listWidget, &QListWidget::currentRowChanged,
            this, &DialogPreferences::on_listWidget_currentRowChanged);
    connect(ui->buttonBox, &QDialogButtonBox::clicked,
            this, &DialogPreferences::on_buttonBox_clicked);

    loadFromPreferences();
}

///
/// \brief DialogPreferences::~DialogPreferences
///
DialogPreferences::~DialogPreferences()
{
    delete ui;
}

///
/// \brief DialogPreferences::accept
///
void DialogPreferences::accept()
{
    apply();
    QDialog::accept();
}

///
/// \brief DialogPreferences::on_buttonBox_clicked
///
void DialogPreferences::on_buttonBox_clicked(QAbstractButton* btn)
{
    if (ui->buttonBox->buttonRole(btn) == QDialogButtonBox::ApplyRole)
        apply();
}

///
/// \brief DialogPreferences::on_listWidget_currentRowChanged
///
void DialogPreferences::on_listWidget_currentRowChanged(int row)
{
    ui->stackedWidget->setCurrentIndex(row);
    if (auto item = ui->listWidget->item(row))
        ui->labelTitle->setText(item->text());
}

// ----------------------------------------------------------------
//  Load / Apply
// ----------------------------------------------------------------

///
/// \brief DialogPreferences::loadFromPreferences
///
void DialogPreferences::loadFromPreferences()
{
    const auto& prefs = AppPreferences::instance();

    // Interface — colors
    _bgColor     = prefs.backgroundColor();
    _fgColor     = prefs.foregroundColor();
    _statusColor = prefs.statusColor();
    ui->bgColorBtn->setColor(_bgColor);
    ui->fgColorBtn->setColor(_fgColor);
    ui->statusColorBtn->setColor(_statusColor);

    // Interface — language
    const int langIdx = ui->langCombo->findData(prefs.language());
    ui->langCombo->setCurrentIndex(langIdx >= 0 ? langIdx : 0);

    // Interface — font
    const QFont& f = prefs.font();
    ui->fontFamilyCombo->setCurrentFont(f);
    ui->fontSizeSpinBox->setValue(f.pointSize() > 0 ? f.pointSize() : 10);
    ui->fontZoomSpinBox->setValue(prefs.fontZoom());
    ui->fontAntialiasCheck->setChecked(!(f.styleStrategy() & QFont::NoAntialias));

    // Display
    const auto& dd = prefs.displayDefinition();
    ui->zeroBasedAddrCheck->setChecked(dd.ZeroBasedAddress);
    ui->hexAddressCheck->setChecked(dd.HexAddress);
    ui->leadingZerosCheck->setChecked(dd.LeadingZeros);
    ui->colsDistSpinBox->setValue(dd.DataViewColumnsDistance);
    ui->autoscrollCheck->setChecked(dd.AutoscrollLog);
    ui->verboseLoggingCheck->setChecked(dd.VerboseLogging);
    ui->logLimitSpinBox->setValue(dd.LogViewLimit);

    // Script — font
    const QFont& sf = prefs.scriptFont();
    ui->scriptFontFamilyCombo->setCurrentFont(sf);
    ui->scriptFontSizeSpinBox->setValue(sf.pointSize() > 0 ? sf.pointSize() : 10);
    ui->scriptFontAntialiasCheck->setChecked(!(sf.styleStrategy() & QFont::NoAntialias));

    // Script — editor
    ui->autoCompleteCheck->setChecked(prefs.codeAutoComplete());

    on_listWidget_currentRowChanged(ui->listWidget->currentRow());
}

///
/// \brief DialogPreferences::apply
///
void DialogPreferences::apply()
{
    auto& prefs = AppPreferences::instance();

    // Interface — colors
    prefs.setBackgroundColor(_bgColor);
    prefs.setForegroundColor(_fgColor);
    prefs.setStatusColor(_statusColor);

    // Interface — language
    const QString lang = ui->langCombo->currentData().toString();
    prefs.setLanguage(lang);
    if (_mainWindow)
        _mainWindow->setLanguage(lang);

    // Interface — font
    prefs.setFont(fontFromControls(ui->fontFamilyCombo, ui->fontSizeSpinBox, ui->fontAntialiasCheck));
    prefs.setFontZoom(ui->fontZoomSpinBox->value());

    // Display
    DisplayDefinition dd = prefs.displayDefinition();
    dd.ZeroBasedAddress        = ui->zeroBasedAddrCheck->isChecked();
    dd.HexAddress              = ui->hexAddressCheck->isChecked();
    dd.LeadingZeros            = ui->leadingZerosCheck->isChecked();
    dd.DataViewColumnsDistance = ui->colsDistSpinBox->value();
    dd.AutoscrollLog           = ui->autoscrollCheck->isChecked();
    dd.VerboseLogging          = ui->verboseLoggingCheck->isChecked();
    dd.LogViewLimit            = ui->logLimitSpinBox->value();
    prefs.setDisplayDefinition(dd);

    // Script — font
    const QFont scriptFont = fontFromControls(ui->scriptFontFamilyCombo, ui->scriptFontSizeSpinBox, ui->scriptFontAntialiasCheck);
    prefs.setScriptFont(scriptFont);
    if (_mainWindow)
        _mainWindow->applyScriptFont(scriptFont);

    // Script — editor
    const bool autoComplete = ui->autoCompleteCheck->isChecked();
    prefs.setCodeAutoComplete(autoComplete);
    if (_mainWindow)
        _mainWindow->applyAutoComplete(autoComplete);
}

// ----------------------------------------------------------------
//  Helpers
// ----------------------------------------------------------------

///
/// \brief DialogPreferences::fontFromControls
///
QFont DialogPreferences::fontFromControls(QFontComboBox* familyCombo, QSpinBox* sizeBox, QCheckBox* antialiasCheck)
{
    QFont font = familyCombo->currentFont();
    font.setPointSize(sizeBox->value());
    font.setStyleStrategy(antialiasCheck->isChecked() ? QFont::PreferAntialias : QFont::NoAntialias);
    return font;
}
