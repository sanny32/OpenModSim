// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file projectserializer.cpp
/// \brief Implements the project XML serializer.
///

#include <QtWidgets>

#include "apppreferences.h"
#include "appproject.h"
#include "controls/mdiareaex.h"
#include "controls/mditabbar.h"
#include "formdatamapview.h"
#include "formdataview.h"
#include "legacyprojectloader.h"
#include "mainwindow.h"
#include "projectaddressspacexml.h"
#include "projectformxml.h"
#include "projectserializer.h"

namespace {
// Property and panel names shared with AppProject (raw literals are the established
// pattern for these across the form classes).
constexpr const char* kFormPanelProperty = "SplitPanel";
constexpr const char* kFormClosedProperty = "Closed";
constexpr const char* kPanelLeft = "L";
constexpr const char* kPanelRight = "R";
constexpr const char* kSplitAutoCloneProperty = "SplitAutoClone";

///
/// \brief loadXmlOfForm dispatches the XML load call, routing legacy data-view
/// elements through the legacy loader.
/// \param project The project used by the legacy loader to create helper forms.
/// \param widget The form widget to restore.
/// \param r The reader positioned on the form element.
///
void loadXmlOfForm(AppProject& project, QWidget* widget, QXmlStreamReader& r)
{
    if (auto* frm = qobject_cast<FormDataView*>(widget)) {
        if (LegacyProjectLoader::isDataViewElement(r.name().toString())) {
            LegacyProjectLoader::loadDataView(r, *frm, project);
            applyDataFormPreferences(frm);
        } else {
            loadCurrentXmlOfForm(widget, r);
        }
        return;
    }

    loadCurrentXmlOfForm(widget, r);
}

}

///
/// \brief ProjectSerializer::ProjectSerializer
/// \param project The project used for form creation and lookup.
/// \param mbServer The server the address-space data is applied to and read from.
/// \param dataSimulator The simulator the loaded simulations are started on.
/// \param mdiArea The MDI area holding the form windows.
/// \param mainWindow The main window used to switch the view mode during load.
///
ProjectSerializer::ProjectSerializer(AppProject& project,
                                     ModbusMultiServer& mbServer,
                                     DataSimulator* dataSimulator,
                                     MdiAreaEx* mdiArea,
                                     MainWindow* mainWindow)
    : _project(project)
    , _mbServer(mbServer)
    , _dataSimulator(dataSimulator)
    , _mdiArea(mdiArea)
    , _mainWindow(mainWindow)
{
}

///
/// \brief ProjectSerializer::load reads the project XML from the device, restores
/// the forms, view mode and address-space data, and returns the project-global
/// settings for AppProject to apply.
/// \param device The opened device the XML is read from.
/// \param replace True when the project replaces the current one, false on merge.
/// \return The parsed project-global settings.
///
ProjectSerializer::LoadResult ProjectSerializer::load(QIODevice& device, bool replace)
{
    LoadResult result;

    QMdiArea::ViewMode viewMode = QMdiArea::TabbedView;
    bool splitView = false;
    bool viewPreparedForForms = !replace;
    ProjectAddressSpacePayload addressSpace;

    QXmlStreamReader xml(&device);
    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("OpenModSim")) {
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("AppPreferences")) {
                    // Backward compatibility: ignore legacy app-preferences block in project files.
                    xml.skipCurrentElement();
                }
                else if (xml.name() == QLatin1String("ViewSettings")) {
                    const auto attrs = xml.attributes();
                    viewMode = (QMdiArea::ViewMode)qBound(0, attrs.value("ViewMode").toInt(), 1);
                    splitView = attrs.value("SplitView").toInt() != 0;
                    result.ActivePrimaryWindow = attrs.value("ActivePrimaryWindow").toString();
                    result.ActiveSecondaryWindow = attrs.value("ActiveSecondaryWindow").toString();
                    result.ActivePanel = attrs.value("ActivePanel").toString();
                    if (attrs.hasAttribute("GlobalZeroBasedAddress")) {
                        result.HasGlobalZeroBasedAddress = true;
                        result.GlobalZeroBasedAddress = attrs.value("GlobalZeroBasedAddress").toInt() != 0;
                    }
                    if (attrs.hasAttribute("GlobalHexView")) {
                        result.HasGlobalHexView = true;
                        result.GlobalHexView = attrs.value("GlobalHexView").toInt() != 0;
                    }
                    xml.skipCurrentElement();
                }
                else if (xml.name() == QLatin1String("ModbusDefinitions")) {
                    xml >> result.Definitions;
                }
                else if (xml.name() == QLatin1String("Connections")) {
                    while (xml.readNextStartElement()) {
                        if (xml.name() == QLatin1String("ConnectionDetails")) {
                            ConnectionDetails cd;
                            xml >> cd;
                            result.Connections.append(cd);
                        } else {
                            xml.skipCurrentElement();
                        }
                    }
                }
                else if (xml.name() == QLatin1String("Forms")) {
                    if(!viewPreparedForForms) {
                        _mainWindow->setViewMode(viewMode);
                        if(_mdiArea->viewMode() == QMdiArea::TabbedView && _mdiArea->isSplitView() != splitView)
                            _mdiArea->setSplitViewEnabled(splitView);
                        viewPreparedForForms = true;
                    }
                    while (xml.readNextStartElement()) {
                        ProjectFormKind kind;
                        bool isForm = true;
                        if (xml.name() == QLatin1String("FormDataView")) {
                            kind = ProjectFormKind::Data;
                        } else if (LegacyProjectLoader::isDataViewElement(xml.name().toString())) {
                            kind = ProjectFormKind::Data;
                            _mainWindow->setViewMode(viewMode = QMdiArea::SubWindowView);
                        } else if (xml.name() == QLatin1String("FormTrafficView")) {
                            kind = ProjectFormKind::Traffic;
                        } else if (xml.name() == QLatin1String("FormScriptView")) {
                            kind = ProjectFormKind::Script;
                        } else if (xml.name() == QLatin1String("FormDataMapView")) {
                            kind = ProjectFormKind::DataMap;
                        } else {
                            isForm = false;
                        }

                        if (isForm) {
                            MdiArea* targetArea = _mdiArea->primaryArea();
                            const auto attrs = xml.attributes();
                            const QString panel = attrs.value("Panel").toString();
                            const QString savedTitle = attrs.value("Title").toString();
                            const bool isClosed = attrs.value("Closed").toString() == "1";
                            const bool isAutoClone = attrs.value("AutoClone").toString() == "1";
                            const bool onRightPanel = splitView && panel.compare(QLatin1String(kPanelRight), Qt::CaseInsensitive) == 0;
                            if(onRightPanel) {
                                if(auto* secondary = _project.secondaryArea())
                                    targetArea = secondary;
                            }

                            QWidget* frm = nullptr;
                            if(onRightPanel && isAutoClone && targetArea == _project.secondaryArea()) {
                                QWidget* sourceForm = nullptr;
                                if(auto* primary = _mdiArea->primaryArea()) {
                                    for(auto* wnd : primary->localSubWindowList()) {
                                        auto* candidate = qobject_cast<QWidget*>(wnd ? wnd->widget() : nullptr);
                                        bool okCandidate = false;
                                        if(!candidate || candidate->property(kSplitAutoCloneProperty).toBool())
                                            continue;
                                        if(projectFormKindFromWidget(candidate, &okCandidate) == kind &&
                                           okCandidate &&
                                           candidate->windowTitle() == savedTitle) {
                                            sourceForm = candidate;
                                            break;
                                        }
                                    }
                                }

                                if(sourceForm)
                                    frm = _project.createCloneOnArea(sourceForm, targetArea);
                            }

                            if(!frm)
                                frm = _project.createMdiChildOnArea(kind, targetArea, !isAutoClone);

                            if (frm) {
                                loadXmlOfForm(_project, frm, xml);
                                if (isClosed) {
                                    // Park closed forms directly without emitting close/activation churn.
                                    auto* wnd = qobject_cast<QMdiSubWindow*>(frm->parentWidget());
                                    _project.markFormClosed(frm);
                                    if (wnd) {
                                        targetArea->removeSubWindow(wnd);
                                        wnd->deleteLater();
                                    }
                                } else {
                                    frm->show();
                                }
                            } else {
                                xml.skipCurrentElement();
                            }
                        } else {
                            xml.skipCurrentElement();
                        }
                    }
                }
                else if (xml.name() == QLatin1String("TabOrder")) {
                    const auto attrs = xml.attributes();
                    const QString panel = attrs.value("Panel").toString();
                    QStringList* targetOrder = &result.PrimaryTabOrder;
                    if(panel.compare(QLatin1String(kPanelRight), Qt::CaseInsensitive) == 0)
                        targetOrder = &result.SecondaryTabOrder;

                    while(xml.readNextStartElement()) {
                        if(xml.name() == QLatin1String("TabRef"))
                            targetOrder->append(xml.attributes().value("title").toString());
                        xml.skipCurrentElement();
                    }
                }
                else if (xml.name() == QLatin1String("Scripts")) {
                    xml.skipCurrentElement();
                }
                else if (xml.name() == QLatin1String("AddressSpace")) {
                    readProjectAddressSpace(xml, addressSpace);
                }
                else {
                    xml.skipCurrentElement();
                }
            }
        }
        else if (LegacyProjectLoader::isDataViewElement(xml.name().toString())) {
            _mainWindow->setViewMode(viewMode = QMdiArea::SubWindowView);
            if (const auto frm = _project.createMdiChild(ProjectFormKind::Data)) {
                loadXmlOfForm(_project, frm, xml);
                frm->show();
            }
        }
        else {
            xml.skipCurrentElement();
        }
    }

    if(!viewPreparedForForms) {
        _mainWindow->setViewMode(viewMode);
        if(_mdiArea->viewMode() == QMdiArea::TabbedView && _mdiArea->isSplitView() != splitView)
            _mdiArea->setSplitViewEnabled(splitView);
    }

    // Apply values from <AddressSpace> (requires forms to exist so _mbServer has unit maps)
    applyProjectAddressSpace(addressSpace, _mbServer, _dataSimulator, replace);

    result.SplitView = splitView;
    return result;
}

///
/// \brief ProjectSerializer::save writes the project XML to the device: definitions,
/// connections, address space, view settings, forms and tab order.
/// \param device The opened device the XML is written to.
/// \return True when the XML was written without errors.
///
bool ProjectSerializer::save(QIODevice& device)
{
    QXmlStreamWriter w(&device);
    w.setAutoFormatting(true);

    w.writeStartDocument();
    w.writeStartElement("OpenModSim");
    w.writeAttribute("Version", qApp->applicationVersion());

    w << _mbServer.getModbusDefinitions();

    w.writeStartElement("Connections");
    for(auto&& cd : _mbServer.connections()) {
        w << cd;
    }
    w.writeEndElement(); // Connections

    {
        ProjectAddressSpaceRanges projectRanges;
        for (auto* widget : _project.allProjectForms()) {
            if (!widget || widget->property(kSplitAutoCloneProperty).toBool())
                continue;

            if (auto* dataView = qobject_cast<FormDataView*>(widget)) {
                const auto dd = dataView->displayDefinition();
                const auto startAddress = static_cast<quint16>(dd.PointAddress - (dataView->zeroBasedAddress() ? 0 : 1));
                projectRanges.append({
                    dd.DeviceId,
                    dd.PointType,
                    startAddress,
                    dd.Length
                });
            } else if (auto* dataMap = qobject_cast<FormDataMapView*>(widget)) {
                const auto dataMapRanges = dataMap->addressSpaceRanges();
                for (const auto& range : dataMapRanges)
                    projectRanges.append(range);
            }
        }

        const auto allSimulationMap = _dataSimulator->simulationMap();
        if (AppPreferences::instance().saveAllModifiedRegisters()) {
            const auto modifiedRanges = projectAddressSpaceModifiedRanges(_mbServer.descriptionMap(),
                                                                          _mbServer.timestampMap(),
                                                                          allSimulationMap);
            for (const auto& range : modifiedRanges)
                projectRanges.append(range);
        }

        writeProjectAddressSpace(w, _mbServer, allSimulationMap, projectRanges);
    }

    w.writeStartElement("ViewSettings");
    w.writeAttribute("ViewMode", QString::number(_mdiArea->viewMode()));
    w.writeAttribute("SplitView", _mdiArea->isSplitView() ? "1" : "0");
    w.writeAttribute("GlobalZeroBasedAddress", AppPreferences::instance().globalAddressBase() == AddressBase::Base0 ? "1" : "0");
    w.writeAttribute("GlobalHexView", AppPreferences::instance().globalHexView() ? "1" : "0");
    if (auto* activePanel = _mdiArea->activePanel()) {
        if (activePanel == _mdiArea->primaryArea())
            w.writeAttribute("ActivePanel", kPanelLeft);
        else if (activePanel == _project.secondaryArea())
            w.writeAttribute("ActivePanel", kPanelRight);
    }
    if(auto primary = _mdiArea->primaryArea())
        if(auto wnd = primary->activeSubWindow())
            if(auto frm = wnd->widget())
                w.writeAttribute("ActivePrimaryWindow", frm->windowTitle());
    if(_project.isSplitTabbedView())
        if(auto secondary = _project.secondaryArea())
            if(auto wnd = secondary->activeSubWindow())
                if(auto frm = wnd->widget())
                    w.writeAttribute("ActiveSecondaryWindow", frm->windowTitle());
    w.writeEndElement(); // ViewSettings

    w.writeStartElement("Forms");
    saveOpenFormsFromArea(w, _mdiArea->primaryArea(), kPanelLeft, false);
    if(_project.isSplitTabbedView()) {
        saveOpenFormsFromArea(w, _project.secondaryArea(), kPanelRight, false);
        saveOpenFormsFromArea(w, _project.secondaryArea(), kPanelRight, true);
    }
    // Also save forms that are closed (hidden in project tree)
    const auto closed = _project.closedForms();
    for (auto&& frm : closed) {
        if (frm) {
            frm->setProperty(kFormPanelProperty, QLatin1String(kPanelLeft));
            frm->setProperty(kFormClosedProperty, true);
            saveXmlOfForm(frm, w);
            frm->setProperty(kFormPanelProperty, QVariant());
            frm->setProperty(kFormClosedProperty, QVariant());
        }
    }
    w.writeEndElement(); // Forms

    writeTabOrder(w, _mdiArea->primaryArea(), kPanelLeft);
    if(_project.isSplitTabbedView())
        writeTabOrder(w, _project.secondaryArea(), kPanelRight);

    w.writeEndElement(); // OpenModSim
    w.writeEndDocument();

    return !w.hasError();
}

///
/// \brief ProjectSerializer::saveOpenFormsFromArea writes the open forms of one MDI
/// area, tagging each with its panel while it serializes itself.
/// \param w The writer the forms are written to.
/// \param area The MDI area whose forms are saved.
/// \param panel The panel tag written into each form element.
/// \param autoClonesOnly True to save only auto-clone forms, false to skip them.
///
void ProjectSerializer::saveOpenFormsFromArea(QXmlStreamWriter& w, MdiArea* area, const char* panel, bool autoClonesOnly)
{
    if(!area)
        return;

    for(auto* wnd : area->localSubWindowList()) {
        auto* widget = qobject_cast<QWidget*>(wnd ? wnd->widget() : nullptr);
        if(!widget)
            continue;

        const bool isAutoClone = widget->property(kSplitAutoCloneProperty).toBool();
        if(isAutoClone != autoClonesOnly)
            continue;

        widget->setProperty(kFormPanelProperty, QLatin1String(panel));
        saveXmlOfForm(widget, w);
        widget->setProperty(kFormPanelProperty, QVariant());
    }
}

///
/// \brief ProjectSerializer::writeTabOrder writes the tab order of one MDI area.
/// \param w The writer the tab order is written to.
/// \param area The MDI area whose tab order is saved.
/// \param panel The panel tag written into the TabOrder element.
///
void ProjectSerializer::writeTabOrder(QXmlStreamWriter& w, MdiArea* area, const char* panel)
{
    const auto order = tabTitlesForArea(area);
    if(order.isEmpty())
        return;
    w.writeStartElement("TabOrder");
    w.writeAttribute("Panel", panel);
    for(const auto& title : order) {
        w.writeStartElement("TabRef");
        w.writeAttribute("title", title);
        w.writeEndElement(); // TabRef
    }
    w.writeEndElement(); // TabOrder
}
