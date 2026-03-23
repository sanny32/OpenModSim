#ifndef PROJECTTREEWIDGET_H
#define PROJECTTREEWIDGET_H

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QWidget>

enum class ProjectFormType
{
    Data = 0,
    Traffic,
    Script
};

struct ProjectFormRef
{
    ProjectFormType type = ProjectFormType::Data;
    QWidget* widget = nullptr;
};
Q_DECLARE_METATYPE(ProjectFormRef)

///
/// \brief The ProjectTreeWidget class
/// Left-dock tree showing Data forms and Scripts (running state).
///
class ProjectTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit ProjectTreeWidget(QWidget* parent = nullptr);

    void addForm(ProjectFormType type, QWidget* frm);
    void removeForm(QWidget* frm);
    void updateFormTitle(QWidget* frm);
    void setFormScriptRunning(QWidget* frm, bool running);
    void setFormOpen(QWidget* frm, bool open);
    void activateForm(QWidget* frm);

signals:
    void formActivated(ProjectFormRef ref);
    void formDeleteRequested(ProjectFormRef ref);
    void formRenamed(ProjectFormRef ref);
    void formRunScriptRequested(ProjectFormRef ref);
    void formStopScriptRequested(ProjectFormRef ref);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_itemActivated(QTreeWidgetItem* item, int column);
    void on_itemChanged(QTreeWidgetItem* item, int column);
    void on_contextMenu(const QPoint& pos);
    void retranslateUi();

private:
    QTreeWidgetItem* itemForForm(QWidget* frm) const;
    static QIcon iconFor(ProjectFormType type, bool open, bool running, const QIcon& dataOpen,
                         const QIcon& dataClosed, const QIcon& trafficOpen, const QIcon& trafficClosed,
                         const QIcon& scriptIdle, const QIcon& scriptClosed, const QIcon& scriptRunning);

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
