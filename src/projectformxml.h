// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectformxml.h
/// \brief Declares form XML dispatch and tab-order helpers shared by AppProject and ProjectSerializer.
///

#ifndef PROJECTFORMXML_H
#define PROJECTFORMXML_H

#include <QStringList>
#include "projectformkind.h"

class QWidget;
class QXmlStreamReader;
class QXmlStreamWriter;
class FormDataView;
class MdiArea;

///
/// \brief projectFormKindFromWidget resolves the project form kind of a widget.
/// \param widget The form widget to inspect.
/// \param ok Optional flag set to false when the widget is not a project form.
/// \return The resolved kind, or ProjectFormKind::Data when the widget is not a form.
///
ProjectFormKind projectFormKindFromWidget(QWidget* widget, bool* ok = nullptr);

///
/// \brief saveXmlOfForm dispatches the XML save call to the concrete form type.
/// \param widget The form widget to serialize.
/// \param w The writer the form serializes itself to.
///
void saveXmlOfForm(QWidget* widget, QXmlStreamWriter& w);

///
/// \brief loadCurrentXmlOfForm dispatches the current-format XML load call to the
/// concrete form type, skipping the element when the widget is not a form.
/// \param widget The form widget to restore.
/// \param r The reader positioned on the form element.
///
void loadCurrentXmlOfForm(QWidget* widget, QXmlStreamReader& r);

///
/// \brief applyDataFormPreferences applies global display preferences after XML loading.
/// \param form The data view the preferences are applied to.
///
void applyDataFormPreferences(FormDataView* form);

///
/// \brief tabTitlesForArea lists the window titles of an MDI area in tab order.
/// \param area The MDI area to inspect.
/// \return The titles in current tab order, empty when the area has no tab bar.
///
QStringList tabTitlesForArea(const MdiArea* area);

///
/// \brief applyTabOrderToArea reorders the tabs of an MDI area to match the given titles.
/// \param area The MDI area to reorder.
/// \param orderedTitles The window titles in the desired tab order.
///
void applyTabOrderToArea(MdiArea* area, const QStringList& orderedTitles);

#endif // PROJECTFORMXML_H
