#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include "projecttreewidget.h"

namespace {
constexpr int ItemTypeRole        = Qt::UserRole + 1;
constexpr int ItemTypeForm        = 1;
constexpr int ItemScriptRunning   = Qt::UserRole + 3;
constexpr int ItemOpenRole        = Qt::UserRole + 4;

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
    qRegisterMetaType<ProjectFormRef>("ProjectFormRef");

    _iconDataClosed = dimmedIcon(":/res/actionShowData.png");
    _iconTrafficClosed = dimmedIcon(":/res/actionShowTraffic.png");
    _iconScriptClosed = dimmedIcon(":/res/actionShowScript.png");

    setHeaderHidden(true);
    setRootIsDecorated(true);
    setExpandsOnDoubleClick(true);
    setAnimated(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);

    const QIcon iconDir(":/res/icon-directory.png");

    _dataRoot = new QTreeWidgetItem(this, QStringList{tr("Data")});
    _dataRoot->setIcon(0, iconDir);
    _dataRoot->setExpanded(true);

    _trafficRoot = new QTreeWidgetItem(this, QStringList{tr("Traffic")});
    _trafficRoot->setIcon(0, iconDir);
    _trafficRoot->setExpanded(true);

    _scriptRoot = new QTreeWidgetItem(this, QStringList{tr("Script")});
    _scriptRoot->setIcon(0, iconDir);
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
void ProjectTreeWidget::addForm(ProjectFormType type, QWidget* frm)
{
    if (!frm || itemForForm(frm))
        return;

    QTreeWidgetItem* root = _dataRoot;
    QIcon baseIcon = _iconData;
    switch (type)
    {
        case ProjectFormType::Data:
            root = _dataRoot;
            baseIcon = _iconData;
            break;
        case ProjectFormType::Traffic:
            root = _trafficRoot;
            baseIcon = _iconTraffic;
            break;
        case ProjectFormType::Script:
            root = _scriptRoot;
            baseIcon = _iconScriptIdle;
            break;
    }

    auto item = new QTreeWidgetItem(root, QStringList{frm->windowTitle()});
    item->setIcon(0, baseIcon);
    item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void*>(frm)));
    item->setData(0, ItemTypeRole, ItemTypeForm);
    item->setData(0, ItemTypeRole + 1, static_cast<int>(type));
    item->setData(0, ItemOpenRole, true);
    item->setFlags(item->flags() | Qt::ItemIsEditable);

    root->setExpanded(true);
}

///
/// \brief ProjectTreeWidget::removeForm
///
void ProjectTreeWidget::removeForm(QWidget* frm)
{
    auto item = itemForForm(frm);
    if (!item)
        return;

    if (auto parent = item->parent())
        parent->removeChild(item);
    delete item;
}

///
/// \brief ProjectTreeWidget::updateFormTitle
///
void ProjectTreeWidget::updateFormTitle(QWidget* frm)
{
    auto item = itemForForm(frm);
    if (item)
        item->setText(0, frm->windowTitle());
}

///
/// \brief ProjectTreeWidget::setFormScriptRunning
///
void ProjectTreeWidget::setFormScriptRunning(QWidget* frm, bool running)
{
    auto item = itemForForm(frm);
    if (!item)
        return;

    item->setData(0, ItemScriptRunning, running);

    if (running) {
        item->setIcon(0, _iconScriptRunning);
        return;
    }

    const bool isOpen = item->data(0, ItemOpenRole).toBool();
    const auto type = static_cast<ProjectFormType>(item->data(0, ItemTypeRole + 1).toInt());
    item->setIcon(0, iconFor(type, isOpen, running, _iconData, _iconDataClosed, _iconTraffic,
                              _iconTrafficClosed, _iconScriptIdle, _iconScriptClosed, _iconScriptRunning));
}

///
/// \brief ProjectTreeWidget::setFormOpen
///
void ProjectTreeWidget::setFormOpen(QWidget* frm, bool open)
{
    auto item = itemForForm(frm);
    if (!item)
        return;

    const auto type = static_cast<ProjectFormType>(item->data(0, ItemTypeRole + 1).toInt());
    item->setIcon(0, iconFor(type, open, false, _iconData, _iconDataClosed, _iconTraffic,
                              _iconTrafficClosed, _iconScriptIdle, _iconScriptClosed, _iconScriptRunning));

    item->setData(0, ItemOpenRole, open);

    QColor textColor = palette().color(QPalette::Text);
    textColor.setAlpha(open ? 255 : 100);
    item->setForeground(0, textColor);
}

///
/// \brief ProjectTreeWidget::activateForm
///
void ProjectTreeWidget::activateForm(QWidget* frm)
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

    ProjectFormRef ref;
    ref.type = static_cast<ProjectFormType>(item->data(0, ItemTypeRole + 1).toInt());
    ref.widget = static_cast<QWidget*>(ptr);
    emit formActivated(ref);
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

    auto frm = static_cast<QWidget*>(ptr);
    const QString current = frm->windowTitle();
    const QString newName = item->text(0).trimmed();
    if (newName.isEmpty()) {
        item->setText(0, current);
        return;
    }

    if (current != newName) {
        frm->setWindowTitle(newName);
        ProjectFormRef ref;
        ref.type = static_cast<ProjectFormType>(item->data(0, ItemTypeRole + 1).toInt());
        ref.widget = frm;
        emit formRenamed(ref);
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
            auto frm = static_cast<QWidget*>(item->data(0, Qt::UserRole).value<void*>());
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
    const bool isFormItem = item && (item->data(0, ItemTypeRole).toInt() == ItemTypeForm);

    QMenu menu(this);

    // "New …" actions are available for all nodes
    auto newDataAction    = menu.addAction(_iconData,        tr("New Data View"));
    auto newTrafficAction = menu.addAction(_iconTraffic,     tr("New Traffic View"));
    auto newScriptAction  = menu.addAction(_iconScriptIdle,  tr("New Script"));

    QAction* runAction    = nullptr;
    QAction* stopAction   = nullptr;
    QAction* renameAction = nullptr;
    QAction* deleteAction = nullptr;

    if (isFormItem) {
        auto ptr = item->data(0, Qt::UserRole).value<void*>();
        if (!ptr)
            return;

        const auto formType = static_cast<ProjectFormType>(item->data(0, ItemTypeRole + 1).toInt());

        menu.addSeparator();

        if (formType == ProjectFormType::Script) {
            const bool running = item->data(0, ItemScriptRunning).toBool();
            runAction  = menu.addAction(QIcon(":/res/actionRunScript.png"),  tr("Run Script"));
            stopAction = menu.addAction(QIcon(":/res/actionStopScript.png"), tr("Stop Script"));
            runAction->setEnabled(!running);
            stopAction->setEnabled(running);
            menu.addSeparator();
        }

        renameAction = menu.addAction(tr("Rename"));
        menu.addSeparator();
        deleteAction = menu.addAction(tr("Delete"));
    }

    auto selected = menu.exec(viewport()->mapToGlobal(pos));
    if (!selected)
        return;

    if (selected == newDataAction) {
        emit formCreateRequested(ProjectFormType::Data);
        return;
    }
    if (selected == newTrafficAction) {
        emit formCreateRequested(ProjectFormType::Traffic);
        return;
    }
    if (selected == newScriptAction) {
        emit formCreateRequested(ProjectFormType::Script);
        return;
    }

    if (!isFormItem)
        return;

    auto ptr = item->data(0, Qt::UserRole).value<void*>();
    ProjectFormRef ref;
    ref.type   = static_cast<ProjectFormType>(item->data(0, ItemTypeRole + 1).toInt());
    ref.widget = static_cast<QWidget*>(ptr);

    if (selected == runAction) {
        emit formRunScriptRequested(ref);
        return;
    }
    if (selected == stopAction) {
        emit formStopScriptRequested(ref);
        return;
    }
    if (selected == renameAction) {
        const QString current = ref.widget ? ref.widget->windowTitle() : QString();
        bool ok = false;
        const QString newName = QInputDialog::getText(this, tr("Rename"), tr("New name:"),
                                                      QLineEdit::Normal, current, &ok).trimmed();
        if (ok && !newName.isEmpty() && newName != current)
            item->setText(0, newName);
        return;
    }
    if (selected == deleteAction) {
        const int ret = QMessageBox::question(this,
                                              tr("Delete Form"),
                                              tr("Delete \"%1\" from the project?").arg(ref.widget ? ref.widget->windowTitle() : QString()),
                                              QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes)
            emit formDeleteRequested(ref);
    }
}

///
/// \brief ProjectTreeWidget::itemForForm
///
QTreeWidgetItem* ProjectTreeWidget::itemForForm(QWidget* frm) const
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

QIcon ProjectTreeWidget::iconFor(ProjectFormType type, bool open, bool running, const QIcon& dataOpen,
                                 const QIcon& dataClosed, const QIcon& trafficOpen, const QIcon& trafficClosed,
                                 const QIcon& scriptIdle, const QIcon& scriptClosed, const QIcon& scriptRunning)
{
    if (running)
        return scriptRunning;

    switch (type)
    {
        case ProjectFormType::Data:
            return open ? dataOpen : dataClosed;
        case ProjectFormType::Traffic:
            return open ? trafficOpen : trafficClosed;
        case ProjectFormType::Script:
            return open ? scriptIdle : scriptClosed;
    }
    return QIcon();
}
