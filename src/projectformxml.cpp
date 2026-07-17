// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectformxml.cpp
/// \brief Implements form XML dispatch and tab-order helpers shared by AppProject and ProjectSerializer.
///

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "apppreferences.h"
#include "controls/mdiarea.h"
#include "controls/mditabbar.h"
#include "formdatamapview.h"
#include "formdataview.h"
#include "formscriptview.h"
#include "formtrafficview.h"
#include "projectformxml.h"

///
/// \brief projectFormKindFromWidget resolves the project form kind of a widget.
/// \param widget The form widget to inspect.
/// \param ok Optional flag set to false when the widget is not a project form.
/// \return The resolved kind, or ProjectFormKind::Data when the widget is not a form.
///
ProjectFormKind projectFormKindFromWidget(QWidget* widget, bool* ok)
{
    if (qobject_cast<FormDataView*>(widget)) {
        if(ok) *ok = true;
        return ProjectFormKind::Data;
    }
    if (qobject_cast<FormTrafficView*>(widget)) {
        if(ok) *ok = true;
        return ProjectFormKind::Traffic;
    }
    if (qobject_cast<FormScriptView*>(widget)) {
        if(ok) *ok = true;
        return ProjectFormKind::Script;
    }
    if (qobject_cast<FormDataMapView*>(widget)) {
        if(ok) *ok = true;
        return ProjectFormKind::DataMap;
    }

    if(ok) *ok = false;
    return ProjectFormKind::Data;
}

///
/// \brief saveXmlOfForm dispatches the XML save call to the concrete form type.
/// \param widget The form widget to serialize.
/// \param w The writer the form serializes itself to.
///
void saveXmlOfForm(QWidget* widget, QXmlStreamWriter& w)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) frm->saveXml(w);
    else if (auto* frm = qobject_cast<FormTrafficView*>(widget)) frm->saveXml(w);
    else if (auto* frm = qobject_cast<FormScriptView*>(widget)) frm->saveXml(w);
    else if (auto* frm = qobject_cast<FormDataMapView*>(widget)) frm->saveXml(w);
}

///
/// \brief applyDataFormPreferences applies global display preferences after XML loading.
/// \param form The data view the preferences are applied to.
///
void applyDataFormPreferences(FormDataView* form)
{
    if (!form)
        return;

    const AppPreferences& prefs = AppPreferences::instance();
    form->setFont(prefs.font());
    form->setZoomPercent(prefs.fontZoom());
    form->setForegroundColor(prefs.foregroundColor());
    form->setBackgroundColor(prefs.backgroundColor());
    form->setAddressColor(prefs.addressColor());
    form->setCommentColor(prefs.commentColor());
}

///
/// \brief loadCurrentXmlOfForm dispatches the current-format XML load call to the
/// concrete form type, skipping the element when the widget is not a form.
/// \param widget The form widget to restore.
/// \param r The reader positioned on the form element.
///
void loadCurrentXmlOfForm(QWidget* widget, QXmlStreamReader& r)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) {
        frm->loadXml(r);
        applyDataFormPreferences(frm);
    }
    else if (auto* frm = qobject_cast<FormTrafficView*>(widget)) frm->loadXml(r);
    else if (auto* frm = qobject_cast<FormScriptView*>(widget)) frm->loadXml(r);
    else if (auto* frm = qobject_cast<FormDataMapView*>(widget)) frm->loadXml(r);
    else r.skipCurrentElement();
}

///
/// \brief tabTitlesForArea lists the window titles of an MDI area in tab order.
/// \param area The MDI area to inspect.
/// \return The titles in current tab order, empty when the area has no tab bar.
///
QStringList tabTitlesForArea(const MdiArea* area)
{
    QStringList titles;
    if(!area)
        return titles;

    const auto* tabBar = qobject_cast<const MdiTabBar*>(area->tabBar());
    if(!tabBar)
        return titles;

    titles.reserve(tabBar->count());
    for(int i = 0; i < tabBar->count(); ++i) {
        const auto* wnd = tabBar->subWindowAt(i);
        const auto* widget = wnd ? wnd->widget() : nullptr;
        if(widget)
            titles << widget->windowTitle();
    }
    return titles;
}

///
/// \brief applyTabOrderToArea reorders the tabs of an MDI area to match the given titles.
/// \param area The MDI area to reorder.
/// \param orderedTitles The window titles in the desired tab order.
///
void applyTabOrderToArea(MdiArea* area, const QStringList& orderedTitles)
{
    if(!area || orderedTitles.isEmpty())
        return;

    auto* tabBar = qobject_cast<MdiTabBar*>(area->tabBar());
    if(!tabBar)
        return;

    for(int targetIndex = 0; targetIndex < orderedTitles.size(); ++targetIndex) {
        const QString& wantedTitle = orderedTitles.at(targetIndex);
        int currentIndex = -1;
        for(int i = 0; i < tabBar->count(); ++i) {
            auto* wnd = tabBar->subWindowAt(i);
            auto* widget = wnd ? wnd->widget() : nullptr;
            if(widget && widget->windowTitle() == wantedTitle) {
                currentIndex = i;
                break;
            }
        }

        if(currentIndex >= 0 && currentIndex != targetIndex)
            tabBar->moveTab(currentIndex, targetIndex);
    }
}
