#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include "projecttreewidget.h"

namespace {
constexpr int ItemTypeRole = Qt::UserRole + 1;
constexpr int ItemTypeForm = 1;

QIcon dimmedIcon(const QString& path)
{
    const QPixmap srcPixmap(path);
    if (srcPixmap.isNull())
        return QIcon();

    QPixmap grayPixmap(srcPixmap.size());
    grayPixmap.fill(Qt::transparent);
    QPainter painter(&grayPixmap);
    painter.setOpacity(0.4);
    painter.drawPixmap(0, 0, srcPixmap);
    painter.end();
    return QIcon(grayPixmap);
}
}

///
/// \brief ProjectTreeWidget::ProjectTreeWidget
///
ProjectTreeWidget::ProjectTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
    , _iconData(QIcon(":/res/actionShowData.png"))
    , _iconTraffic(QIcon(":/res/actionShowTraffic.png"))
    , _iconScriptIdle(QIcon(":/res/actionShowScript.png"))
    , _iconScriptRunning(QIcon(":/res/actionRunScript.png"))
{
    _iconDataClosed = dimmedIcon(":/res/actionShowData.png");
    _iconTrafficClosed = dimmedIcon(":/res/actionShowTraffic.png");
    _iconScriptClosed = dimmedIcon(":/res/actionShowScript.png");

    setHeaderHidden(true);
    setRootIsDecorated(true);
    setExpandsOnDoubleClick(true);
    setAnimated(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);

    _dataRoot = new QTreeWidgetItem(this, QStringList{tr("Data")});
    _dataRoot->setExpanded(true);

    _trafficRoot = new QTreeWidgetItem(this, QStringList{tr("Traffic")});
    _trafficRoot->setExpanded(true);

    _scriptRoot = new QTreeWidgetItem(this, QStringList{tr("Script")});
    _scriptRoot->setExpanded(true);

    connect(this, &QTreeWidget::itemActivated,
            this, &ProjectTreeWidget::on_itemActivated);
    connect(this, &QTreeWidget::itemChanged,
            this, &ProjectTreeWidget::on_itemChanged);
    connect(this, &QTreeWidget::customContextMenuRequested,
            this, &ProjectTreeWidget::on_contextMenu);
}

///
/// \brief ProjectTreeWidget::addForm
///
void ProjectTreeWidget::addForm(FormModSim* frm)
{
    if (!frm || itemForForm(frm))
        return;

    QTreeWidgetItem* root = _dataRoot;
    QIcon baseIcon = _iconData;
    switch (frm->formKind())
    {
        case FormModSim::FormKind::Data:
            root = _dataRoot;
            baseIcon = _iconData;
            break;
        case FormModSim::FormKind::Traffic:
            root = _trafficRoot;
            baseIcon = _iconTraffic;
            break;
        case FormModSim::FormKind::Script:
            root = _scriptRoot;
            baseIcon = _iconScriptIdle;
            break;
    }

    auto item = new QTreeWidgetItem(root, QStringList{frm->windowTitle()});
    item->setIcon(0, baseIcon);
    item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void*>(frm)));
    item->setData(0, ItemTypeRole, ItemTypeForm);
    item->setFlags(item->flags() | Qt::ItemIsEditable);

    connect(frm, &FormModSim::scriptRunning, this, [this, frm]() {
        setFormScriptRunning(frm, true);
    });
    connect(frm, &FormModSim::scriptStopped, this, [this, frm]() {
        setFormScriptRunning(frm, false);
    });

    root->setExpanded(true);
}

///
/// \brief ProjectTreeWidget::removeForm
///
void ProjectTreeWidget::removeForm(FormModSim* frm)
{
    auto item = itemForForm(frm);
    if (!item)
        return;

    if (auto parent = item->parent())
        parent->removeChild(item);
    delete item;
}

///
/// \brief ProjectTreeWidget::setFormScriptRunning
///
void ProjectTreeWidget::setFormScriptRunning(FormModSim* frm, bool running)
{
    auto item = itemForForm(frm);
    if (!item)
        return;

    if (running) {
        item->setIcon(0, _iconScriptRunning);
        return;
    }

    const bool isOpen = !item->font(0).italic();
    switch (frm->formKind())
    {
        case FormModSim::FormKind::Data:
            item->setIcon(0, isOpen ? _iconData : _iconDataClosed);
            break;
        case FormModSim::FormKind::Traffic:
            item->setIcon(0, isOpen ? _iconTraffic : _iconTrafficClosed);
            break;
        case FormModSim::FormKind::Script:
            item->setIcon(0, isOpen ? _iconScriptIdle : _iconScriptClosed);
            break;
    }
}

///
/// \brief ProjectTreeWidget::setFormOpen
///
void ProjectTreeWidget::setFormOpen(FormModSim* frm, bool open)
{
    auto item = itemForForm(frm);
    if (!item)
        return;

    switch (frm->formKind())
    {
        case FormModSim::FormKind::Data:
            item->setIcon(0, open ? _iconData : _iconDataClosed);
            break;
        case FormModSim::FormKind::Traffic:
            item->setIcon(0, open ? _iconTraffic : _iconTrafficClosed);
            break;
        case FormModSim::FormKind::Script:
            item->setIcon(0, open ? _iconScriptIdle : _iconScriptClosed);
            break;
    }

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
    if (!item)
        return;

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
    if (!item)
        return;

    if (item->data(0, ItemTypeRole).toInt() != ItemTypeForm)
        return;

    auto ptr = item->data(0, Qt::UserRole).value<void*>();
    if (!ptr)
        return;

    emit formActivated(static_cast<FormModSim*>(ptr));
}

///
/// \brief ProjectTreeWidget::on_itemChanged
///
void ProjectTreeWidget::on_itemChanged(QTreeWidgetItem* item, int column)
{
    if (column != 0)
        return;

    if (item->data(0, ItemTypeRole).toInt() != ItemTypeForm)
        return;

    auto ptr = item->data(0, Qt::UserRole).value<void*>();
    if (!ptr)
        return;

    auto frm = static_cast<FormModSim*>(ptr);
    const QString current = frm->windowTitle();
    const QString newName = item->text(0).trimmed();
    if (newName.isEmpty()) {
        item->setText(0, current);
        return;
    }

    if (current != newName) {
        frm->setWindowTitle(newName);
        emit formRenamed(frm);
    }
}

///
/// \brief ProjectTreeWidget::retranslateUi
///
void ProjectTreeWidget::retranslateUi()
{
    _dataRoot->setText(0, tr("Data"));
    _trafficRoot->setText(0, tr("Traffic"));
    _scriptRoot->setText(0, tr("Script"));

    auto retranslateRoot = [](QTreeWidgetItem* root) {
        for (int i = 0; i < root->childCount(); ++i) {
            auto item = root->child(i);
            auto frm = static_cast<FormModSim*>(item->data(0, Qt::UserRole).value<void*>());
            if (frm)
                item->setText(0, frm->windowTitle());
        }
    };

    retranslateRoot(_dataRoot);
    retranslateRoot(_trafficRoot);
    retranslateRoot(_scriptRoot);
}

///
/// \brief ProjectTreeWidget::on_contextMenu
///
void ProjectTreeWidget::on_contextMenu(const QPoint& pos)
{
    auto item = itemAt(pos);
    if (!item)
        return;

    if (item->data(0, ItemTypeRole).toInt() != ItemTypeForm)
        return;

    auto ptr = item->data(0, Qt::UserRole).value<void*>();
    if (!ptr)
        return;

    auto frm = static_cast<FormModSim*>(ptr);

    QMenu menu(this);
    auto deleteAction = menu.addAction(tr("Delete"));
    if (menu.exec(viewport()->mapToGlobal(pos)) != deleteAction)
        return;

    const int ret = QMessageBox::question(this,
                                          tr("Delete Form"),
                                          tr("Delete \"%1\" from the project?").arg(frm->windowTitle()),
                                          QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes)
        emit formDeleteRequested(frm);
}

///
/// \brief ProjectTreeWidget::itemForForm
///
QTreeWidgetItem* ProjectTreeWidget::itemForForm(FormModSim* frm) const
{
    auto findInRoot = [frm](QTreeWidgetItem* root) -> QTreeWidgetItem* {
        for (int i = 0; i < root->childCount(); ++i) {
            auto item = root->child(i);
            if (item->data(0, Qt::UserRole).value<void*>() == static_cast<void*>(frm))
                return item;
        }
        return nullptr;
    };

    if (auto item = findInRoot(_dataRoot))
        return item;
    if (auto item = findInRoot(_trafficRoot))
        return item;
    if (auto item = findInRoot(_scriptRoot))
        return item;

    return nullptr;
}
