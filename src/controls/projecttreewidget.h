#ifndef PROJECTTREEWIDGET_H
#define PROJECTTREEWIDGET_H

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include "formmodsim.h"
#include "scriptdocument.h"

///
/// \brief The ProjectTreeWidget class
/// Left-dock tree showing Data forms and Scripts (running state).
///
class ProjectTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit ProjectTreeWidget(QWidget* parent = nullptr);

    void addForm(FormModSim* frm);
    void removeForm(FormModSim* frm);
    void setFormScriptRunning(FormModSim* frm, bool running);
    void activateForm(FormModSim* frm);

    void addScript(ScriptDocument* doc);
    void removeScript(ScriptDocument* doc);
    void setScriptRunning(ScriptDocument* doc, bool running);
    void activateScript(ScriptDocument* doc);

signals:
    void formActivated(FormModSim* frm);
    void scriptActivated(ScriptDocument* doc);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_itemActivated(QTreeWidgetItem* item, int column);
    void retranslateUi();

private:
    QTreeWidgetItem* itemForForm(FormModSim* frm) const;
    QTreeWidgetItem* itemForScript(ScriptDocument* doc) const;

private:
    QTreeWidgetItem* _dataRoot    = nullptr;
    QTreeWidgetItem* _scriptsRoot = nullptr;

    QIcon _iconForm;
    QIcon _iconScriptIdle;
    QIcon _iconScriptRunning;
};

#endif // PROJECTTREEWIDGET_H
