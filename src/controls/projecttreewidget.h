#ifndef PROJECTTREEWIDGET_H
#define PROJECTTREEWIDGET_H

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include "formmodsim.h"

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
    void setFormOpen(FormModSim* frm, bool open);
    void activateForm(FormModSim* frm);

signals:
    void formActivated(FormModSim* frm);
    void formDeleteRequested(FormModSim* frm);
    void formRenamed(FormModSim* frm);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_itemActivated(QTreeWidgetItem* item, int column);
    void on_itemChanged(QTreeWidgetItem* item, int column);
    void on_contextMenu(const QPoint& pos);
    void retranslateUi();

private:
    QTreeWidgetItem* itemForForm(FormModSim* frm) const;

private:
    QTreeWidgetItem* _dataRoot    = nullptr;
    QTreeWidgetItem* _trafficRoot = nullptr;
    QTreeWidgetItem* _scriptRoot  = nullptr;

    QIcon _iconData;
    QIcon _iconDataClosed;
    QIcon _iconTraffic;
    QIcon _iconTrafficClosed;
    QIcon _iconScriptIdle;
    QIcon _iconScriptClosed;
    QIcon _iconScriptRunning;
};

#endif // PROJECTTREEWIDGET_H
