#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include "projecttreewidget.h"

namespace {
    constexpr int ItemTypeRole = Qt::UserRole + 1;
    constexpr int ItemTypeForm   = 1;
    constexpr int ItemTypeScript = 2;
}

///
/// \brief ProjectTreeWidget::ProjectTreeWidget
///
ProjectTreeWidget::ProjectTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
    , _iconForm(QIcon(":/res/actionNew.png"))
    , _iconScriptIdle(QIcon(":/res/actionShowScript.png"))
    , _iconScriptRunning(QIcon(":/res/actionRunScript.png"))
{
    // Build a grayed version of the form icon for closed forms
    const QPixmap srcPixmap(":/res/actionNew.png");
    QPixmap grayPixmap(srcPixmap.size());
    grayPixmap.fill(Qt::transparent);
    QPainter painter(&grayPixmap);
    painter.setOpacity(0.4);
    painter.drawPixmap(0, 0, srcPixmap);
    painter.end();
    _iconFormClosed = QIcon(grayPixmap);

    setHeaderHidden(true);
    setRootIsDecorated(true);
    setExpandsOnDoubleClick(true);
    setAnimated(true);
    setContextMenuPolicy(Qt::CustomContextMenu);

    _dataRoot = new QTreeWidgetItem(this, QStringList{tr("Data")});
    _dataRoot->setExpanded(true);

    _scriptsRoot = new QTreeWidgetItem(this, QStringList{tr("Scripts")});
    _scriptsRoot->setExpanded(true);

    connect(this, &QTreeWidget::itemActivated,
            this, &ProjectTreeWidget::on_itemActivated);
    connect(this, &QTreeWidget::customContextMenuRequested,
            this, &ProjectTreeWidget::on_contextMenu);
}

///
/// \brief ProjectTreeWidget::addForm
///
void ProjectTreeWidget::addForm(FormModSim* frm)
{
    if (!frm || itemForForm(frm)) return;

    auto item = new QTreeWidgetItem(_dataRoot, QStringList{frm->windowTitle()});
    item->setIcon(0, _iconForm);
    item->setData(0, Qt::UserRole,  QVariant::fromValue(static_cast<void*>(frm)));
    item->setData(0, ItemTypeRole, ItemTypeForm);

    connect(frm, &FormModSim::scriptRunning, this, [this, frm]() {
        setFormScriptRunning(frm, true);
    });
    connect(frm, &FormModSim::scriptStopped, this, [this, frm]() {
        setFormScriptRunning(frm, false);
    });

    _dataRoot->setExpanded(true);
}

///
/// \brief ProjectTreeWidget::removeForm
///
void ProjectTreeWidget::removeForm(FormModSim* frm)
{
    auto item = itemForForm(frm);
    if (!item) return;

    _dataRoot->removeChild(item);
    delete item;
}

///
/// \brief ProjectTreeWidget::setFormScriptRunning
///
void ProjectTreeWidget::setFormScriptRunning(FormModSim* frm, bool running)
{
    auto item = itemForForm(frm);
    if (!item) return;

    item->setIcon(0, running ? _iconScriptRunning : _iconForm);
}

///
/// \brief ProjectTreeWidget::setFormOpen
///
void ProjectTreeWidget::setFormOpen(FormModSim* frm, bool open)
{
    auto item = itemForForm(frm);
    if (!item) return;

    item->setIcon(0, open ? _iconForm : _iconFormClosed);

    QFont f = item->font(0);
    f.setItalic(!open);
    item->setFont(0, f);
}

///
/// \brief ProjectTreeWidget::activateForm
///
void ProjectTreeWidget::activateForm(FormModSim* frm)
{
    auto item = itemForForm(frm);
    if (!item) return;

    blockSignals(true);
    setCurrentItem(item);
    blockSignals(false);
}

///
/// \brief ProjectTreeWidget::addScript
///
void ProjectTreeWidget::addScript(ScriptDocument* doc)
{
    if (!doc || itemForScript(doc)) return;

    auto item = new QTreeWidgetItem(_scriptsRoot, QStringList{doc->name()});
    item->setIcon(0, _iconScriptIdle);
    item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void*>(doc)));
    item->setData(0, ItemTypeRole, ItemTypeScript);

    connect(doc, &ScriptDocument::nameChanged, this, [this, doc](const QString& name) {
        auto it = itemForScript(doc);
        if (it) it->setText(0, name);
    });

    _scriptsRoot->setExpanded(true);
}

///
/// \brief ProjectTreeWidget::removeScript
///
void ProjectTreeWidget::removeScript(ScriptDocument* doc)
{
    auto item = itemForScript(doc);
    if (!item) return;

    _scriptsRoot->removeChild(item);
    delete item;
}

///
/// \brief ProjectTreeWidget::setScriptRunning
///
void ProjectTreeWidget::setScriptRunning(ScriptDocument* doc, bool running)
{
    auto item = itemForScript(doc);
    if (!item) return;

    item->setIcon(0, running ? _iconScriptRunning : _iconScriptIdle);
}

///
/// \brief ProjectTreeWidget::activateScript
///
void ProjectTreeWidget::activateScript(ScriptDocument* doc)
{
    auto item = itemForScript(doc);
    if (!item) return;

    blockSignals(true);
    setCurrentItem(item);
    blockSignals(false);
}

///
/// \brief ProjectTreeWidget::changeEvent
///
void ProjectTreeWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslateUi();
    QTreeWidget::changeEvent(event);
}

///
/// \brief ProjectTreeWidget::on_itemActivated
///
void ProjectTreeWidget::on_itemActivated(QTreeWidgetItem* item, int /*column*/)
{
    if (!item) return;
    const int type = item->data(0, ItemTypeRole).toInt();
    auto ptr = item->data(0, Qt::UserRole).value<void*>();
    if (!ptr) return;

    if (type == ItemTypeForm)
        emit formActivated(static_cast<FormModSim*>(ptr));
    else if (type == ItemTypeScript)
        emit scriptActivated(static_cast<ScriptDocument*>(ptr));
}

///
/// \brief ProjectTreeWidget::retranslateUi
///
void ProjectTreeWidget::retranslateUi()
{
    _dataRoot->setText(0, tr("Data"));
    _scriptsRoot->setText(0, tr("Scripts"));

    for (int i = 0; i < _dataRoot->childCount(); ++i) {
        auto item = _dataRoot->child(i);
        auto frm = static_cast<FormModSim*>(item->data(0, Qt::UserRole).value<void*>());
        if (frm) item->setText(0, frm->windowTitle());
    }
}

///
/// \brief ProjectTreeWidget::on_contextMenu
///
void ProjectTreeWidget::on_contextMenu(const QPoint& pos)
{
    auto item = itemAt(pos);
    if (!item) return;

    const int type = item->data(0, ItemTypeRole).toInt();
    auto ptr = item->data(0, Qt::UserRole).value<void*>();
    if (!ptr) return;

    QMenu menu(this);
    auto deleteAction = menu.addAction(tr("Delete"));

    if (menu.exec(viewport()->mapToGlobal(pos)) != deleteAction)
        return;

    if (type == ItemTypeForm) {
        auto frm = static_cast<FormModSim*>(ptr);
        const int ret = QMessageBox::question(this, tr("Delete Form"),
            tr("Delete \"%1\" from the project?").arg(frm->windowTitle()),
            QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes)
            emit formDeleteRequested(frm);
    }
    else if (type == ItemTypeScript) {
        auto doc = static_cast<ScriptDocument*>(ptr);
        const int ret = QMessageBox::question(this, tr("Delete Script"),
            tr("Delete \"%1\" from the project?").arg(doc->name()),
            QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes)
            emit scriptDeleteRequested(doc);
    }
}

///
/// \brief ProjectTreeWidget::itemForForm
///
QTreeWidgetItem* ProjectTreeWidget::itemForForm(FormModSim* frm) const
{
    for (int i = 0; i < _dataRoot->childCount(); ++i) {
        auto item = _dataRoot->child(i);
        if (item->data(0, Qt::UserRole).value<void*>() == static_cast<void*>(frm))
            return item;
    }
    return nullptr;
}

///
/// \brief ProjectTreeWidget::itemForScript
///
QTreeWidgetItem* ProjectTreeWidget::itemForScript(ScriptDocument* doc) const
{
    for (int i = 0; i < _scriptsRoot->childCount(); ++i) {
        auto item = _scriptsRoot->child(i);
        if (item->data(0, Qt::UserRole).value<void*>() == static_cast<void*>(doc))
            return item;
    }
    return nullptr;
}
