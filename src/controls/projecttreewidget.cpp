#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include "formscriptview.h"
#include "projecttreewidget.h"

namespace {
constexpr int ItemTypeRole        = Qt::UserRole + 1;
constexpr int ItemTypeForm        = 1;
constexpr int ItemScriptRunning   = Qt::UserRole + 3;
constexpr int ItemOpenRole        = Qt::UserRole + 4;
constexpr const char* kSplitAutoCloneProperty = "SplitAutoClone";
constexpr const char* kSplitOriginIdProperty  = "SplitOriginId";

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
    , _iconData(QIcon(":/res/icon-show-data.png"))
    , _iconTraffic(QIcon(":/res/icon-show-traffic.png"))
    , _iconScriptIdle(QIcon(":/res/icon-show-script.png"))
    , _iconScriptRunning(QIcon(":/res/icon-run-script.png"))
    , _iconDataMapLocked(QIcon(":/res/icon-data-locked.png"))
{
    qRegisterMetaType<ProjectFormRef>("ProjectFormRef");

    _iconDataClosed = dimmedIcon(":/res/icon-show-data.png");
    _iconTrafficClosed = dimmedIcon(":/res/icon-show-traffic.png");
    _iconScriptClosed = dimmedIcon(":/res/icon-show-script.png");
    _iconDataMap = QIcon(":/res/icon-show-data.png");
    _iconDataMapClosed = dimmedIcon(":/res/icon-show-data.png");
    _iconDataMapLockedClosed = dimmedIcon(":/res/icon-data-locked.png");

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

    _dataMapRoot = new QTreeWidgetItem(this, QStringList{tr("Map")});
    _dataMapRoot->setIcon(0, iconDir);
    _dataMapRoot->setExpanded(true);

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
        case ProjectFormType::DataMap:
            root = _dataMapRoot;
            baseIcon = _iconDataMap;
            break;
    }

    if (type == ProjectFormType::DataMap && frm->property("DeleteLocked").toBool())
        baseIcon = _iconDataMapLocked;

    auto item = new QTreeWidgetItem(root, QStringList{frm->windowTitle()});
    item->setIcon(0, baseIcon);
    item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void*>(frm)));
    item->setData(0, ItemTypeRole, ItemTypeForm);
    item->setData(0, ItemTypeRole + 1, static_cast<int>(type));
    item->setData(0, ItemOpenRole, true);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    refreshFormItem(item, frm);

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
    if (item) {
        item->setText(0, frm->windowTitle());
        refreshFormItem(item, frm);
    }
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
    refreshFormItem(item, frm);
}

///
/// \brief ProjectTreeWidget::setFormOpen
///
void ProjectTreeWidget::setFormOpen(QWidget* frm, bool open)
{
    auto item = itemForForm(frm);
    if (!item)
        return;

    item->setData(0, ItemOpenRole, open);
    refreshFormItem(item, frm);
}

///
/// \brief ProjectTreeWidget::activateForm
///
void ProjectTreeWidget::activateForm(QWidget* frm)
{
    auto item = itemForForm(frm);
    if (!item && frm && frm->property(kSplitAutoCloneProperty).toBool()) {
        const QUuid originId = frm->property(kSplitOriginIdProperty).toUuid();
        const QString title = frm->windowTitle();

        auto findOriginalInRoot = [&](QTreeWidgetItem* root) -> QTreeWidgetItem* {
            if (!root)
                return nullptr;

            for (int i = 0; i < root->childCount(); ++i) {
                auto* candidateItem = root->child(i);
                auto* candidateForm = static_cast<QWidget*>(candidateItem->data(0, Qt::UserRole).value<void*>());
                if (!candidateForm)
                    continue;

                const bool sameOrigin = !originId.isNull()
                                        && candidateForm->property(kSplitOriginIdProperty).toUuid() == originId;
                const bool sameTitle = candidateForm->windowTitle() == title;
                if (sameOrigin || sameTitle)
                    return candidateItem;
            }

            return nullptr;
        };

        item = findOriginalInRoot(_dataRoot);
        if (!item) item = findOriginalInRoot(_trafficRoot);
        if (!item) item = findOriginalInRoot(_scriptRoot);
        if (!item) item = findOriginalInRoot(_dataMapRoot);
    }

    if (!item)
        return;

    blockSignals(true);
    setCurrentItem(item);
    blockSignals(false);
}

void ProjectTreeWidget::refreshFormItem(QTreeWidgetItem* item, QWidget* frm)
{
    if (!item || !frm)
        return;

    const bool open = item->data(0, ItemOpenRole).toBool();
    const bool running = item->data(0, ItemScriptRunning).toBool();
    const auto type = static_cast<ProjectFormType>(item->data(0, ItemTypeRole + 1).toInt());
    const bool deleteLocked = frm->property("DeleteLocked").toBool();

    item->setIcon(0, iconFor(type, open, running, _iconData, _iconDataClosed, _iconTraffic,
                              _iconTrafficClosed, _iconScriptIdle, _iconScriptClosed, _iconScriptRunning,
                              deleteLocked ? _iconDataMapLocked : _iconDataMap,
                              deleteLocked ? _iconDataMapLockedClosed : _iconDataMapClosed));

    QColor textColor = palette().color(QPalette::Text);
    textColor.setAlpha(open ? 255 : 100);
    item->setForeground(0, textColor);
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
    if (frm->property("DeleteLocked").toBool()) {
        if (item->text(0) != current)
            item->setText(0, current);
        return;
    }

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
    _dataMapRoot->setText(0, tr("Map"));

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
    retranslateRoot(_dataMapRoot);
}

///
/// \brief ProjectTreeWidget::on_contextMenu
///
void ProjectTreeWidget::on_contextMenu(const QPoint& pos)
{
    auto item = itemAt(pos);
    const bool isFormItem = item && (item->data(0, ItemTypeRole).toInt() == ItemTypeForm);
    const bool isRootItem = item == _dataRoot || item == _dataMapRoot || item == _trafficRoot || item == _scriptRoot;
    const bool isScriptRootItem = (item == _scriptRoot);
    ProjectFormType rootType = ProjectFormType::Data;
    if (item == _dataRoot)
        rootType = ProjectFormType::Data;
    else if (item == _dataMapRoot)
        rootType = ProjectFormType::DataMap;
    else if (item == _trafficRoot)
        rootType = ProjectFormType::Traffic;
    else if (item == _scriptRoot)
        rootType = ProjectFormType::Script;

    QMenu menu(this);

    // "New" actions are available for all nodes
    auto newDataAction        = menu.addAction(_iconData,         tr("New Data View"));
    auto newDataMapAction     = menu.addAction(_iconDataMap,      tr("New Map View"));
    auto newTrafficAction     = menu.addAction(_iconTraffic,      tr("New Traffic View"));
    auto newScriptAction      = menu.addAction(_iconScriptIdle,   tr("New Script"));

    QAction* runAction    = nullptr;
    QAction* stopAction   = nullptr;
    QAction* runAllAction = nullptr;
    QAction* stopAllAction = nullptr;
    QAction* deleteAllAction = nullptr;
    QAction* renameAction = nullptr;
    QAction* deleteAction = nullptr;

    if (isRootItem) {
        bool canDeleteAny = false;
        for (int i = 0; i < item->childCount(); ++i) {
            auto* child = item->child(i);
            auto* form = static_cast<QWidget*>(child->data(0, Qt::UserRole).value<void*>());
            if (form && !form->property("DeleteLocked").toBool()) {
                canDeleteAny = true;
                break;
            }
        }

        menu.addSeparator();
        deleteAllAction = menu.addAction(tr("Delete All"));
        deleteAllAction->setEnabled(canDeleteAny);
    }

    if (isScriptRootItem) {
        bool canRunAny = false;
        bool canStopAny = false;

        for (int i = 0; i < _scriptRoot->childCount(); ++i) {
            auto* child = _scriptRoot->child(i);
            auto* script = qobject_cast<FormScriptView*>(static_cast<QWidget*>(child->data(0, Qt::UserRole).value<void*>()));
            if (!script)
                continue;

            canRunAny = canRunAny || script->canRunScript();
            canStopAny = canStopAny || script->canStopScript();
            if (canRunAny && canStopAny)
                break;
        }

        menu.addSeparator();
        runAllAction = menu.addAction(QIcon(":/res/icon-run-script.png"), tr("Run All Scripts"));
        stopAllAction = menu.addAction(QIcon(":/res/icon-stop-script.png"), tr("Stop All Scripts"));
        runAllAction->setEnabled(canRunAny);
        stopAllAction->setEnabled(canStopAny);
    }

    if (isFormItem) {
        auto ptr = item->data(0, Qt::UserRole).value<void*>();
        if (!ptr)
            return;

        auto* formWidget = static_cast<QWidget*>(ptr);
        const auto formType = static_cast<ProjectFormType>(item->data(0, ItemTypeRole + 1).toInt());
        const bool deleteLocked = formWidget->property("DeleteLocked").toBool();

        menu.addSeparator();

        if (formType == ProjectFormType::Script) {
            const bool running = item->data(0, ItemScriptRunning).toBool();
            runAction  = menu.addAction(QIcon(":/res/icon-run-script.png"),  tr("Run Script"));
            stopAction = menu.addAction(QIcon(":/res/icon-stop-script.png"), tr("Stop Script"));
            runAction->setEnabled(!running);
            stopAction->setEnabled(running);
            menu.addSeparator();
        }

        renameAction = menu.addAction(tr("Rename"));
        renameAction->setEnabled(!deleteLocked);
        menu.addSeparator();
        deleteAction = menu.addAction(tr("Delete"));
        deleteAction->setEnabled(!deleteLocked);
    }

    auto selected = menu.exec(viewport()->mapToGlobal(pos));
    if (!selected)
        return;

    if (selected == newDataAction) {
        emit formCreateRequested(ProjectFormType::Data);
        return;
    }
    if (selected == newDataMapAction) {
        emit formCreateRequested(ProjectFormType::DataMap);
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
    if (selected == runAllAction) {
        emit runAllScriptsRequested();
        return;
    }
    if (selected == stopAllAction) {
        emit stopAllScriptsRequested();
        return;
    }
    if (selected == deleteAllAction) {
        const QString nodeName = item ? item->text(0) : QString();
        const int ret = QMessageBox::question(this,
                                              tr("Delete All"),
                                              tr("Delete all in \"%1\"?").arg(nodeName),
                                              QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes)
            emit deleteAllFormsRequested(rootType);
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
    if (auto item = findInRoot(_dataMapRoot))
        return item;

    return nullptr;
}

QIcon ProjectTreeWidget::iconFor(ProjectFormType type, bool open, bool running, const QIcon& dataOpen,
                                 const QIcon& dataClosed, const QIcon& trafficOpen, const QIcon& trafficClosed,
                                 const QIcon& scriptIdle, const QIcon& scriptClosed, const QIcon& scriptRunning,
                                 const QIcon& DataMapOpen, const QIcon& DataMapClosed)
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
        case ProjectFormType::DataMap:
            return open ? DataMapOpen : DataMapClosed;
    }
    return QIcon();
}


