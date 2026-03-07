#include <QtWidgets>
#include "colorswatch.h"
#include "controls/addressbasecombobox.h"
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

    for (int i = 0; i < ui->listWidget->count(); ++i)
        ui->listWidget->item(i)->setSizeHint(QSize(0, 28));

    ui->comboBoxLanguage->addItem("English",                       "en");
    ui->comboBoxLanguage->addItem(QString::fromUtf8("Русский"),    "ru");
    ui->comboBoxLanguage->addItem(QString::fromUtf8("简体中文"),    "zh_CN");
    ui->comboBoxLanguage->addItem(QString::fromUtf8("繁體中文"),    "zh_TW");

    ui->fontComboBoxFont->setFontFilters(QFontComboBox::MonospacedFonts);

    connect(ui->pushButtonBackgroundColor, &QPushButton::clicked, this, [this]() {
        QColorDialog dlg(_bgColor, this);
        if (dlg.exec() == QDialog::Accepted) {
            _bgColor = dlg.currentColor();
            ui->pushButtonBackgroundColor->setColor(_bgColor);
        }
    });
    connect(ui->pushButtonResetBackgroundColor, &QPushButton::clicked, this, [this]() {
        _bgColor = Qt::white;
        ui->pushButtonBackgroundColor->setColor(_bgColor);
    });

    connect(ui->pushButtonForegroundColor, &QPushButton::clicked, this, [this]() {
        QColorDialog dlg(_fgColor, this);
        if (dlg.exec() == QDialog::Accepted) {
            _fgColor = dlg.currentColor();
            ui->pushButtonForegroundColor->setColor(_fgColor);
        }
    });
    connect(ui->pushButtonResetForegroundColor, &QPushButton::clicked, this, [this]() {
        _fgColor = Qt::black;
        ui->pushButtonForegroundColor->setColor(_fgColor);
    });

    connect(ui->pushButtonStatusColor, &QPushButton::clicked, this, [this]() {
        QColorDialog dlg(_statusColor, this);
        if (dlg.exec() == QDialog::Accepted) {
            _statusColor = dlg.currentColor();
            ui->pushButtonStatusColor->setColor(_statusColor);
        }
    });
    connect(ui->pushButtonResetStatusColor, &QPushButton::clicked, this, [this]() {
        _statusColor = Qt::red;
        ui->pushButtonStatusColor->setColor(_statusColor);
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
/// rief DialogPreferences::changeEvent
///
void DialogPreferences::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);

    QDialog::changeEvent(event);
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
    ui->pushButtonBackgroundColor->setColor(_bgColor);
    ui->pushButtonForegroundColor->setColor(_fgColor);
    ui->pushButtonStatusColor->setColor(_statusColor);

    // Interface — language
    const int langIdx = ui->comboBoxLanguage->findData(prefs.language());
    ui->comboBoxLanguage->setCurrentIndex(langIdx >= 0 ? langIdx : 0);

    // Interface — updates
    ui->checkBoxCheckForUpdates->setChecked(prefs.checkForUpdates());

    // Interface — font
    const QFont& f = prefs.font();
    ui->fontComboBoxFont->setCurrentFont(f);
    ui->spinBoxFontSize->setValue(f.pointSize() > 0 ? f.pointSize() : 10);
    ui->spinBoxFontZoom->setValue(prefs.fontZoom());
    ui->checkBoxFontAntialias->setChecked(!(f.styleStrategy() & QFont::NoAntialias));

    // Display
    const auto& dd = prefs.displayDefinition();
    ui->comboBoxAddressBase->setCurrentAddressBase(dd.ZeroBasedAddress ? AddressBase::Base0 : AddressBase::Base1);
    ui->checkBoxHexAddress->setChecked(dd.HexAddress);
    ui->checkBoxLeadingZeros->setChecked(dd.LeadingZeros);
    ui->spinBoxColumnsDistance->setValue(dd.DataViewColumnsDistance);
    ui->checkBoxAutoscrollLog->setChecked(dd.AutoscrollLog);
    ui->checkBoxVerboseLogging->setChecked(dd.VerboseLogging);
    ui->spinBoxLogLimit->setValue(dd.LogViewLimit);

    // Script — font
    const QFont& sf = prefs.scriptFont();
    ui->fontComboBoxScriptFont->setCurrentFont(sf);
    ui->spinBoxScriptFontSize->setValue(sf.pointSize() > 0 ? sf.pointSize() : 10);
    ui->checkBoxScriptFontAntialias->setChecked(!(sf.styleStrategy() & QFont::NoAntialias));

    // Script — editor
    ui->checkBoxAutoComplete->setChecked(prefs.codeAutoComplete());
    ui->checkBoxRunOnStartup->setChecked(prefs.runScriptOnStartup());

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
    if (_mainWindow) _mainWindow->applyColors(_bgColor, _fgColor, _statusColor);

    // Interface — language
    const QString lang = ui->comboBoxLanguage->currentData().toString();
    prefs.setLanguage(lang);
    if (_mainWindow) _mainWindow->setLanguage(lang);

    // Interface — updates
    const bool checkUpdates = ui->checkBoxCheckForUpdates->isChecked();
    prefs.setCheckForUpdates(checkUpdates);
    if (_mainWindow) _mainWindow->applyCheckForUpdates(checkUpdates);

    // Interface — font
    const QFont displayFont = fontFromControls(ui->fontComboBoxFont, ui->spinBoxFontSize, ui->checkBoxFontAntialias);
    prefs.setFont(displayFont);
    prefs.setFontZoom(ui->spinBoxFontZoom->value());
    if (_mainWindow) {
        _mainWindow->applyFont(displayFont);
        _mainWindow->applyZoom(ui->spinBoxFontZoom->value());
    }

    // Display
    DisplayDefinition dd = prefs.displayDefinition();
    dd.ZeroBasedAddress        = (ui->comboBoxAddressBase->currentAddressBase() == AddressBase::Base0);
    dd.HexAddress              = ui->checkBoxHexAddress->isChecked();
    dd.LeadingZeros            = ui->checkBoxLeadingZeros->isChecked();
    dd.DataViewColumnsDistance = ui->spinBoxColumnsDistance->value();
    dd.AutoscrollLog           = ui->checkBoxAutoscrollLog->isChecked();
    dd.VerboseLogging          = ui->checkBoxVerboseLogging->isChecked();
    dd.LogViewLimit            = ui->spinBoxLogLimit->value();
    prefs.setDisplayDefinition(dd);

    // Script — font
    const QFont scriptFont = fontFromControls(ui->fontComboBoxScriptFont, ui->spinBoxScriptFontSize, ui->checkBoxScriptFontAntialias);
    prefs.setScriptFont(scriptFont);
    if (_mainWindow) _mainWindow->applyScriptFont(scriptFont);

    // Script — editor
    const bool autoComplete = ui->checkBoxAutoComplete->isChecked();
    prefs.setCodeAutoComplete(autoComplete);
    prefs.setRunScriptOnStartup(ui->checkBoxRunOnStartup->isChecked());
    if (_mainWindow) _mainWindow->applyAutoComplete(autoComplete);
}

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
