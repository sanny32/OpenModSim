#ifndef DIALOGPREFERENCES_H
#define DIALOGPREFERENCES_H

#include <QDialog>
#include <QAbstractButton>
#include "apppreferences.h"

namespace Ui {
class DialogPreferences;
}

class MainWindow;

///
/// \brief The DialogPreferences class
///
/// Preferences dialog modelled after Qt Creator's Preferences window.
/// Left panel: category list.  Right panel: stacked page per category.
/// All controls are defined in dialogpreferences.ui.
///
class DialogPreferences : public QDialog
{
    Q_OBJECT

public:
    explicit DialogPreferences(MainWindow* mainWindow, QWidget* parent = nullptr);
    ~DialogPreferences();

    void accept() override;

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_buttonBox_clicked(QAbstractButton* btn);
    void on_listWidget_currentRowChanged(int row);

private:
    void loadFromPreferences();
    void apply();

    static QFont fontFromControls(QFontComboBox* familyCombo, QSpinBox* sizeBox, QCheckBox* antialiasCheck);

private:
    Ui::DialogPreferences* ui;
    MainWindow*            _mainWindow;

    QColor _bgColor;
    QColor _fgColor;
    QColor _addrColor;
    QColor _commentColor;
};

#endif // DIALOGPREFERENCES_H

