#ifndef RUNMODECOMBOBOX_H
#define RUNMODECOMBOBOX_H

#include <QComboBox>
#include "enums.h"

///
/// \brief The RunModeComboBox class
///
class RunModeComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit RunModeComboBox(QWidget *parent = nullptr);

    RunMode currentRunMode() const;
    void setCurrentRunMode(RunMode mode);

protected:
    void changeEvent(QEvent* event) override;

signals:
    void runModeChanged(RunMode value);

private slots:
    void on_currentIndexChanged(int);
};

#endif // RUNMODECOMBOBOX_H
