/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2023- Equinor ASA
//
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
//
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#include "RimWellConnectivityTable.h"

#include "RiaPreferences.h"
#include "RiaQDateTimeTools.h"
#include "RiaWellFlowDefines.h"

#include "RigAccWellFlowCalculator.h"
#include "RigEclipseCaseData.h"
#include "RigSimWellData.h"
#include "RigSimulationWellCenterLineCalculator.h"
#include "RigWellAllocationOverTime.h"

#include "RimCellFilter.h"
#include "RimCellFilterCollection.h"
#include "RimEclipseCaseTools.h"
#include "RimEclipseCellColors.h"
#include "RimEclipseResultCase.h"
#include "RimEclipseView.h"
#include "RimFlowDiagSolution.h"
#include "RimFlowDiagnosticsTools.h"
#include "RimRegularLegendConfig.h"
#include "RimSimWellInView.h"
#include "RimTools.h"
#include "RimWellAllocationTools.h"
#include "RimWellLogFile.h"
#include "RimWellPlotTools.h"

#include "RiuMatrixPlotWidget.h"

#include "cafPdmUiComboBoxEditor.h"
#include "cafPdmUiPushButtonEditor.h"
#include "cafPdmUiToolButtonEditor.h"
#include "cafPdmUiTreeSelectionEditor.h"

#include "cvfScalarMapper.h"

#include <QObject>

CAF_PDM_SOURCE_INIT( RimWellConnectivityTable, "RimWellConnectivityTable" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
namespace caf
{
template <>
void AppEnum<RimWellConnectivityTable::TimeStepSelection>::setUp()
{
    addItem( RimWellConnectivityTable::TimeStepSelection::SINGLE_TIME_STEP, "SINGLE_TIME_STEP", "Single Time Step" );
    addItem( RimWellConnectivityTable::TimeStepSelection::TIME_STEP_RANGE, "TIME_STEP_RANGE", "Time Step Range" );
    setDefault( RimWellConnectivityTable::TimeStepSelection::SINGLE_TIME_STEP );
}
template <>
void AppEnum<RimWellConnectivityTable::TimeSampleValueType>::setUp()
{
    addItem( RimWellConnectivityTable::TimeSampleValueType::FLOW_RATE, "FLOW_RATE", "Flow Rate" );
    addItem( RimWellConnectivityTable::TimeSampleValueType::FLOW_RATE_FRACTION, "FLOW_RATE_FRACTION", "Flow Rate Fraction" );
    addItem( RimWellConnectivityTable::TimeSampleValueType::FLOW_RATE_PERCENTAGE, "FLOW_RATE_PERCENTAGE", "Flow Rate Percentage" );
    setDefault( RimWellConnectivityTable::TimeSampleValueType::FLOW_RATE_FRACTION );
}
template <>
void AppEnum<RimWellConnectivityTable::TimeRangeValueType>::setUp()
{
    addItem( RimWellConnectivityTable::TimeRangeValueType::ACCUMULATED_FLOW_VOLUME, "ACCUMULATED_FLOW_VOLUME", "Accumulated Flow Volume" );
    addItem( RimWellConnectivityTable::TimeRangeValueType::ACCUMULATED_FLOW_VOLUME_FRACTION,
             "ACCUMULATED_FLOW_VOLUME_FRACTION",
             "Accumulated Flow Volume Fraction" );
    addItem( RimWellConnectivityTable::TimeRangeValueType::ACCUMULATED_FLOW_VOLUME_PERCENTAGE,
             "ACCUMULATED_FLOW_VOLUME_PERCENTAGE",
             "Accumulated Flow Volume Percentage" );
    setDefault( RimWellConnectivityTable::TimeRangeValueType::ACCUMULATED_FLOW_VOLUME_FRACTION );
}

template <>
void AppEnum<RimWellConnectivityTable::TimeStepRangeFilterMode>::setUp()
{
    addItem( RimWellConnectivityTable::TimeStepRangeFilterMode::NONE, "NONE", "Show All Time Steps" );
    addItem( RimWellConnectivityTable::TimeStepRangeFilterMode::TIME_STEP_COUNT, "TIME_STEP_COUNT", "Time Step Count" );
    setDefault( RimWellConnectivityTable::TimeStepRangeFilterMode::TIME_STEP_COUNT );
}
} // namespace caf

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimWellConnectivityTable::RimWellConnectivityTable()
{
    CAF_PDM_InitObject( "Producer/Injector Connectivity", ":/CorrelationMatrixPlot16x16.png" );
    uiCapability()->setUiTreeChildrenHidden( true );

    CAF_PDM_InitFieldNoDefault( &m_case, "CurveCase", "Case" );
    m_case.uiCapability()->setUiTreeChildrenHidden( true );
    CAF_PDM_InitFieldNoDefault( &m_cellFilterView, "VisibleCellView", "Filter by 3D View Visibility" );
    CAF_PDM_InitFieldNoDefault( &m_flowDiagSolution, "FlowDiagSolution", "Flow Diag Solution" );
    m_flowDiagSolution.uiCapability()->setUiHidden( true );

    CAF_PDM_InitFieldNoDefault( &m_timeStepSelection, "TimeStepSelectopn", "Time Step Type" );
    m_timeStepSelection.uiCapability()->setUiEditorTypeName( caf::PdmUiComboBoxEditor::uiEditorTypeName() );

    CAF_PDM_InitField( &m_selectProducersAndInjectorsForTimeSteps, "SelectProducersAndInjectorsForTimeSteps", true, "Select Wells For Time Step(s)" );

    CAF_PDM_InitField( &m_thresholdValue, "ThresholdValue", 0.0, "Threshold" );

    // Single time step configuration
    CAF_PDM_InitFieldNoDefault( &m_timeSampleValueType, "TimeSampleValueType", "Value Type" );
    m_timeSampleValueType.uiCapability()->setUiEditorTypeName( caf::PdmUiComboBoxEditor::uiEditorTypeName() );
    CAF_PDM_InitFieldNoDefault( &m_selectedTimeStep, "TimeStep", "Time Step" );
    m_selectedTimeStep.uiCapability()->setUiEditorTypeName( caf::PdmUiComboBoxEditor::uiEditorTypeName() );

    // Time step range configuration
    CAF_PDM_InitFieldNoDefault( &m_timeRangeValueType, "TimeRangeValueType", "Value Type" );
    m_timeRangeValueType.uiCapability()->setUiEditorTypeName( caf::PdmUiComboBoxEditor::uiEditorTypeName() );
    CAF_PDM_InitFieldNoDefault( &m_selectedFromTimeStep, "FromTimeStep", "From Time Step" );
    m_selectedFromTimeStep.uiCapability()->setUiEditorTypeName( caf::PdmUiComboBoxEditor::uiEditorTypeName() );
    CAF_PDM_InitFieldNoDefault( &m_selectedToTimeStep, "ToTimeStep", "To Time Step" );
    m_selectedToTimeStep.uiCapability()->setUiEditorTypeName( caf::PdmUiComboBoxEditor::uiEditorTypeName() );
    CAF_PDM_InitFieldNoDefault( &m_timeStepFilterMode, "TimeStepRangeFilterMode", "Filter" );
    m_timeStepFilterMode.uiCapability()->setUiEditorTypeName( caf::PdmUiComboBoxEditor::uiEditorTypeName() );
    CAF_PDM_InitField( &m_timeStepCount, "TimeStepCount", m_initialNumberOfTimeSteps, "Number of Time Steps" );
    CAF_PDM_InitFieldNoDefault( &m_excludeTimeSteps, "ExcludeTimeSteps", "" );
    m_excludeTimeSteps.uiCapability()->setUiEditorTypeName( caf::PdmUiTreeSelectionEditor::uiEditorTypeName() );
    CAF_PDM_InitFieldNoDefault( &m_applyTimeStepSelections, "ApplyTimeStepSelections", "" );
    caf::PdmUiPushButtonEditor::configureEditorForField( &m_applyTimeStepSelections );

    // Producer/Injector tracer configuration
    CAF_PDM_InitFieldNoDefault( &m_selectedProducerTracersUiField, "SelectedProducerTracers", "Producer Tracers" );
    m_selectedProducerTracersUiField.xmlCapability()->disableIO();
    m_selectedProducerTracersUiField.uiCapability()->setUiEditorTypeName( caf::PdmUiTreeSelectionEditor::uiEditorTypeName() );
    m_selectedProducerTracersUiField.uiCapability()->setUiLabelPosition( caf::PdmUiItemInfo::HIDDEN );
    CAF_PDM_InitFieldNoDefault( &m_selectedInjectorTracersUiField, "SelectedInjectorTracers", "Injector Tracers" );
    m_selectedInjectorTracersUiField.xmlCapability()->disableIO();
    m_selectedInjectorTracersUiField.uiCapability()->setUiEditorTypeName( caf::PdmUiTreeSelectionEditor::uiEditorTypeName() );
    m_selectedInjectorTracersUiField.uiCapability()->setUiLabelPosition( caf::PdmUiItemInfo::HIDDEN );
    CAF_PDM_InitField( &m_syncSelectedInjectorsFromProducerSelection, "SyncSelectedProdInj", false, "Synch Communicators ->" );
    m_syncSelectedInjectorsFromProducerSelection.uiCapability()->setUiEditorTypeName( caf::PdmUiToolButtonEditor::uiEditorTypeName() );
    CAF_PDM_InitField( &m_syncSelectedProducersFromInjectorSelection, "SyncSelectedInjProd", false, "<- Synch Communicators" );
    m_syncSelectedProducersFromInjectorSelection.uiCapability()->setUiEditorTypeName( caf::PdmUiToolButtonEditor::uiEditorTypeName() );
    CAF_PDM_InitFieldNoDefault( &m_applySelectedInectorProducerTracers, "ApplySelectedInectorProducerTracers", "" );
    caf::PdmUiPushButtonEditor::configureEditorForField( &m_applySelectedInectorProducerTracers );

    // Table settings
    CAF_PDM_InitField( &m_showValueLabels, "ShowValueLabels", false, "Show Value Labels" );

    // Font control
    CAF_PDM_InitFieldNoDefault( &m_axisTitleFontSize, "AxisTitleFontSize", "Axis Title Font Size" );
    CAF_PDM_InitFieldNoDefault( &m_axisLabelFontSize, "AxisLabelFontSize", "Axis Label Font Size" );
    CAF_PDM_InitFieldNoDefault( &m_valueLabelFontSize, "ValueLabelFontSize", "Value Label Font Size" );
    m_axisTitleFontSize = caf::FontTools::RelativeSize::Large;
    m_axisLabelFontSize = caf::FontTools::RelativeSize::Medium;

    CAF_PDM_InitFieldNoDefault( &m_legendConfig, "LegendConfig", "" );
    m_legendConfig = new RimRegularLegendConfig();
    m_legendConfig->setShowLegend( true );
    m_legendConfig->setAutomaticRanges( 0.0, 100.0, 0.0, 100.0 );
    m_legendConfig->setColorLegend( RimRegularLegendConfig::mapToColorLegend( RimRegularLegendConfig::ColorRangesType::HEAT_MAP ) );

    setLegendsVisible( true );
    setAsPlotMdiWindow();
    setShowWindow( false );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimWellConnectivityTable::~RimWellConnectivityTable()
{
    if ( isMdiWindow() ) removeMdiWindowFromMdiArea();

    cleanupBeforeClose();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::setFromSimulationWell( RimSimWellInView* simWell )
{
    if ( !simWell ) return;

    RimEclipseView* eclView;
    simWell->firstAncestorOrThisOfType( eclView );
    RimEclipseResultCase* eclCase;
    simWell->firstAncestorOrThisOfType( eclCase );

    m_cellFilterView = eclView;
    m_case           = eclCase;

    // Set valid single time step and time step range selections based on case
    setValidTimeStepSelectionsForCase();

    // Set single time step and current selected time step from active view
    m_timeStepSelection = TimeStepSelection::SINGLE_TIME_STEP;

    if ( eclCase )
    {
        m_selectedTimeStep = eclCase->timeStepDates().at( eclView->currentTimeStep() );
    }

    // Use the active flow diagnostics solutions, or the first one as default
    if ( eclView )
    {
        m_flowDiagSolution = eclView->cellResult()->flowDiagSolution();
        if ( !m_flowDiagSolution )
        {
            m_flowDiagSolution = m_case->defaultFlowDiagSolution();
        }
    }

    connectViewCellFiltersChangedToSlot( m_cellFilterView );
    setSelectedProducersAndInjectorsForSingleTimeStep();
    onLoadDataAndUpdate();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::cleanupBeforeClose()
{
    if ( m_matrixPlotWidget )
    {
        m_matrixPlotWidget->qwtPlot()->detachItems();
        m_matrixPlotWidget->setParent( nullptr );
        delete m_matrixPlotWidget;
        m_matrixPlotWidget = nullptr;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    RimPlotWindow::fieldChangedByUi( changedField, oldValue, newValue );

    if ( changedField == &m_case )
    {
        if ( m_case )
        {
            m_flowDiagSolution = m_case->defaultFlowDiagSolution();
            m_cellFilterView   = !m_case->views().empty() ? dynamic_cast<RimEclipseView*>( m_case->views().front() ) : nullptr;
        }
        else
        {
            m_flowDiagSolution = nullptr;
            m_cellFilterView   = nullptr;
        }

        setValidTimeStepSelectionsForCase();
        onLoadDataAndUpdate();
    }
    else if ( changedField == &m_flowDiagSolution || changedField == &m_thresholdValue )
    {
        if ( m_thresholdValue() < 0.0 )
        {
            m_matrixPlotWidget->setInvalidValueRange( m_thresholdValue(), 0.0 );
        }
        else
        {
            m_matrixPlotWidget->setInvalidValueRange( 0.0, m_thresholdValue() );
        }
        onLoadDataAndUpdate();
    }
    else if ( changedField == &m_cellFilterView )
    {
        // Disconnect signal/slots for previous cellFilterView
        PdmObjectHandle* prevValue          = oldValue.value<caf::PdmPointer<PdmObjectHandle>>().rawPtr();
        auto*            prevCellFilterView = dynamic_cast<RimEclipseView*>( prevValue );
        disconnectViewCellFiltersChangedFromSlots( prevCellFilterView );

        // Connect signal/slots for current cellFilterView
        connectViewCellFiltersChangedToSlot( m_cellFilterView );

        onLoadDataAndUpdate();
    }
    else if ( changedField == &m_syncSelectedInjectorsFromProducerSelection )
    {
        syncSelectedInjectorsFromProducerSelection();
    }
    else if ( changedField == &m_syncSelectedProducersFromInjectorSelection )
    {
        syncSelectedProducersFromInjectorSelection();
    }
    else if ( m_matrixPlotWidget && changedField == &m_showValueLabels )
    {
        m_matrixPlotWidget->setShowValueLabel( m_showValueLabels );
    }
    else if ( changedField == &m_selectProducersAndInjectorsForTimeSteps )
    {
        if ( m_selectProducersAndInjectorsForTimeSteps && m_timeStepSelection == TimeStepSelection::SINGLE_TIME_STEP )
        {
            setSelectedProducersAndInjectorsForSingleTimeStep();
            onLoadDataAndUpdate();
        }
        else if ( m_selectProducersAndInjectorsForTimeSteps && m_timeStepSelection == TimeStepSelection::TIME_STEP_RANGE )
        {
            setSelectedProducersAndInjectorsForTimeStepRange();
            onLoadDataAndUpdate();
        }
    }
    else if ( changedField == &m_selectedTimeStep || changedField == &m_timeSampleValueType ||
              ( changedField == &m_timeStepSelection && m_timeStepSelection() == TimeStepSelection::SINGLE_TIME_STEP ) )
    {
        // Update selected prod/injectors based on selected time step
        if ( m_selectProducersAndInjectorsForTimeSteps )
        {
            setSelectedProducersAndInjectorsForSingleTimeStep();
        }

        // For singe time step - no apply buttons due to low computation time
        onLoadDataAndUpdate();
    }
    else if ( changedField == &m_selectedFromTimeStep || changedField == &m_selectedToTimeStep ||
              ( changedField == &m_timeStepSelection && m_timeStepSelection() == TimeStepSelection::TIME_STEP_RANGE ) )
    {
        // Update selected prod/injectors based on time step range
        if ( m_selectProducersAndInjectorsForTimeSteps )
        {
            setSelectedProducersAndInjectorsForTimeStepRange();
        }
    }
    else if ( changedField == &m_applyTimeStepSelections || changedField == &m_applySelectedInectorProducerTracers )
    {
        // For time step range - depends on apply buttons to prevent unwanted loading of large amount of data
        onLoadDataAndUpdate();
    }
    else if ( changedField == &m_timeStepCount && m_timeStepFilterMode == TimeStepRangeFilterMode::TIME_STEP_COUNT )
    {
        m_excludeTimeSteps.setValue( {} );
    }
    else if ( m_matrixPlotWidget && ( changedField == &m_titleFontSize || changedField == &m_legendFontSize ||
                                      changedField == &m_axisTitleFontSize || changedField == &m_axisLabelFontSize ) )
    {
        m_matrixPlotWidget->setPlotTitleFontSize( titleFontSize() );
        m_matrixPlotWidget->setLegendFontSize( legendFontSize() );
        m_matrixPlotWidget->setAxisTitleFontSize( axisTitleFontSize() );
        m_matrixPlotWidget->setAxisLabelFontSize( axisLabelFontSize() );
    }
    else if ( changedField == &m_valueLabelFontSize && m_matrixPlotWidget )
    {
        m_matrixPlotWidget->setValueFontSize( valueLabelFontSize() );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::childFieldChangedByUi( const caf::PdmFieldHandle* changedChildField )
{
    onLoadDataAndUpdate();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    RimViewWindow::defineUiOrdering( uiConfigName, uiOrdering );

    caf::PdmUiGroup& dataGroup = *uiOrdering.addNewGroup( "Plot Data" );
    dataGroup.add( &m_case );
    dataGroup.add( &m_cellFilterView );
    dataGroup.add( &m_flowDiagSolution );
    dataGroup.add( &m_timeStepSelection );
    dataGroup.add( &m_selectProducersAndInjectorsForTimeSteps );
    dataGroup.add( &m_thresholdValue );

    caf::PdmUiGroup& flowDiagConfigGroup = *uiOrdering.addNewGroup( "Flow Diagnostics Configuration" );
    if ( m_timeStepSelection() == TimeStepSelection::SINGLE_TIME_STEP )
    {
        flowDiagConfigGroup.add( &m_timeSampleValueType );
        flowDiagConfigGroup.add( &m_selectedTimeStep );
    }
    else if ( m_timeStepSelection() == TimeStepSelection::TIME_STEP_RANGE )
    {
        flowDiagConfigGroup.add( &m_timeRangeValueType );
        flowDiagConfigGroup.add( &m_selectedFromTimeStep );
        flowDiagConfigGroup.add( &m_selectedToTimeStep );
        flowDiagConfigGroup.add( &m_timeStepFilterMode );

        if ( m_timeStepFilterMode == TimeStepRangeFilterMode::TIME_STEP_COUNT )
        {
            flowDiagConfigGroup.add( &m_timeStepCount );
        }
        caf::PdmUiGroup& excludeTimeStepGroup = *flowDiagConfigGroup.addNewGroup( "Exclude Time Steps" );
        excludeTimeStepGroup.add( &m_excludeTimeSteps );
        excludeTimeStepGroup.setCollapsedByDefault();
        flowDiagConfigGroup.add( &m_applyTimeStepSelections );
    }

    caf::PdmUiGroup* selectionGroup = uiOrdering.addNewGroup( "Tracer Selection" );
    caf::PdmUiGroup* producerGroup  = selectionGroup->addNewGroup( "Producers" );
    producerGroup->add( &m_selectedProducerTracersUiField );
    producerGroup->add( &m_syncSelectedInjectorsFromProducerSelection );
    caf::PdmUiGroup* injectorGroup = selectionGroup->addNewGroup( "Injectors", false );
    injectorGroup->add( &m_selectedInjectorTracersUiField );
    injectorGroup->add( &m_syncSelectedProducersFromInjectorSelection );

    selectionGroup->add( &m_applySelectedInectorProducerTracers );

    caf::PdmUiGroup* tableSettingsGroup = uiOrdering.addNewGroup( "Table Settings" );
    tableSettingsGroup->add( &m_showValueLabels );
    m_legendConfig->uiOrdering( "FlagColorsAndMappingModeOnly", *tableSettingsGroup );

    caf::PdmUiGroup* fontGroup = uiOrdering.addNewGroup( "Fonts" );
    fontGroup->setCollapsedByDefault();
    RimPlotWindow::uiOrderingForFonts( uiConfigName, *fontGroup );
    fontGroup->add( &m_axisTitleFontSize );
    fontGroup->add( &m_axisLabelFontSize );
    fontGroup->add( &m_valueLabelFontSize );

    uiOrdering.skipRemainingFields( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::defineEditorAttribute( const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute* attribute )
{
    if ( field == &m_applyTimeStepSelections || field == &m_applySelectedInectorProducerTracers )
    {
        caf::PdmUiPushButtonEditorAttribute* attrib = dynamic_cast<caf::PdmUiPushButtonEditorAttribute*>( attribute );
        if ( attrib )
        {
            attrib->m_buttonText = "Apply";
        }
    }
    if ( field == &m_selectedTimeStep || field == &m_selectedFromTimeStep || field == &m_selectedToTimeStep )
    {
        auto* attrib = dynamic_cast<caf::PdmUiComboBoxEditorAttribute*>( attribute );
        if ( attrib )
        {
            attrib->nextIcon                   = QIcon( ":/ComboBoxDown.svg" );
            attrib->previousIcon               = QIcon( ":/ComboBoxUp.svg" );
            attrib->showPreviousAndNextButtons = true;
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::onLoadDataAndUpdate()
{
    updateMdiWindowVisibility();

    if ( m_matrixPlotWidget == nullptr || m_case == nullptr )
    {
        return;
    }

    // If no 3D view is open, we have to make sure the case is opened
    if ( !m_case->ensureReservoirCaseIsOpen() )
    {
        return;
    }

    // Clear matrix plot
    m_matrixPlotWidget->clearPlotData();

    // Create well flow diagnostics data - with selected production wells (no selection -> no filtering)
    std::set<QString> selectedProductionWells =
        std::set<QString>( m_selectedProducerTracersUiField().begin(), m_selectedProducerTracersUiField().end() );
    const std::map<QString, RigWellAllocationOverTime> productionWellsAllocationOverTime =
        createProductionWellsAllocationOverTimeMap( selectedProductionWells );

    // Retrieve union of injection wells across selected producers
    std::set<QString> injectionWellsUnion;
    for ( const auto& [prodWellName, prodWellData] : productionWellsAllocationOverTime )
    {
        for ( const auto& [injectionWell, values] : prodWellData.wellValuesMap() )
        {
            injectionWellsUnion.insert( injectionWell );
        }
    }

    // Detect matrix column headers
    std::set<QString> selectedInjectors =
        std::set<QString>( m_selectedInjectorTracersUiField().begin(), m_selectedInjectorTracersUiField().end() );
    const std::set<QString>    injectionWells = selectedInjectors.empty() ? injectionWellsUnion : selectedInjectors;
    const std::vector<QString> columnHeaders( injectionWells.begin(), injectionWells.end() );

    // Store rows where minimum one injector well value is above threshold
    std::map<QString, std::vector<double>> filteredRows;
    for ( const auto& [prodWellName, prodWellData] : productionWellsAllocationOverTime )
    {
        bool                hasValueAboveThreshold = false;
        std::vector<double> rowValues              = std::vector<double>( columnHeaders.size(), 0.0 );
        for ( const auto& [injectionWell, values] : prodWellData.wellValuesMap() )
        {
            auto itr = std::find( columnHeaders.begin(), columnHeaders.end(), injectionWell );
            if ( itr == columnHeaders.end() ) continue;
            auto index = static_cast<size_t>( std::distance( columnHeaders.begin(), itr ) );
            if ( index > rowValues.size() - 1 ) continue;

            // Get value at last time step
            const auto value = values.rbegin()->second;
            rowValues[index] = value;

            if ( value > m_thresholdValue() )
            {
                hasValueAboveThreshold = true;
            }
        }

        if ( rowValues.empty() || !hasValueAboveThreshold ) continue;
        filteredRows.emplace( prodWellName, rowValues );
    }

    // Detect columns with one or more value above threshold
    std::set<size_t> columnIndicesToInclude;
    for ( const auto& [rowName, rowValues] : filteredRows )
    {
        for ( size_t i = 0; i < rowValues.size(); i++ )
        {
            if ( rowValues[i] > m_thresholdValue() )
            {
                columnIndicesToInclude.insert( i );
            }
        }
    }

    // Filter column headers based on column indexes to include
    std::vector<QString> filteredColumnHeaders;
    for ( size_t i = 0; i < columnHeaders.size(); ++i )
    {
        if ( columnIndicesToInclude.contains( i ) )
        {
            filteredColumnHeaders.push_back( columnHeaders[i] );
        }
    }

    // Fill matrix plot widget with filtered rows/columns
    double maxValue = 0.0;
    m_matrixPlotWidget->setColumnHeaders( filteredColumnHeaders );
    for ( const auto& [rowName, rowValues] : filteredRows )
    {
        // Add columns with values above threshold
        std::vector<double> columns;
        for ( size_t i = 0; i < rowValues.size(); ++i )
        {
            if ( !columnIndicesToInclude.contains( i ) ) continue;

            maxValue = maxValue < rowValues[i] ? rowValues[i] : maxValue;
            columns.push_back( rowValues[i] );
        }

        m_matrixPlotWidget->setRowValues( rowName, columns );
    }

    // Set ranges using max value
    if ( m_legendConfig )
    {
        const auto [min, max] = createLegendMinMaxValues( maxValue );
        m_legendConfig->setAutomaticRanges( min, max, 0.0, 0.0 );
    }

    // Set titles and font sizes
    m_matrixPlotWidget->setPlotTitle( createTableTitle() );
    m_matrixPlotWidget->setRowTitle( "Producer wells" );
    m_matrixPlotWidget->setColumnTitle( "Injection wells" );
    m_matrixPlotWidget->setPlotTitleFontSize( titleFontSize() );
    m_matrixPlotWidget->setLegendFontSize( legendFontSize() );
    m_matrixPlotWidget->setAxisTitleFontSize( axisTitleFontSize() );
    m_matrixPlotWidget->setAxisLabelFontSize( axisLabelFontSize() );

    m_matrixPlotWidget->createPlot();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimWellConnectivityTable::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    QList<caf::PdmOptionItemInfo> options = RimPlotWindow::calculateValueOptions( fieldNeedingOptions );
    if ( !options.empty() )
    {
        return options;
    }

    if ( fieldNeedingOptions == &m_case )
    {
        auto resultCases = RimEclipseCaseTools::eclipseResultCases();
        for ( RimEclipseResultCase* c : resultCases )
        {
            options.push_back( caf::PdmOptionItemInfo( c->caseUserDescription(), c, false, c->uiIconProvider() ) );
        }
    }
    else if ( fieldNeedingOptions == &m_cellFilterView && m_case() )
    {
        options.push_back( caf::PdmOptionItemInfo( "Disabled", nullptr ) );
        for ( RimEclipseView* view : m_case()->reservoirViews.children() )
        {
            CVF_ASSERT( view && "Really always should have a valid view pointer in ReservoirViews" );
            options.push_back( caf::PdmOptionItemInfo( view->name(), view, false, view->uiIconProvider() ) );
        }
    }
    else if ( m_case && fieldNeedingOptions == &m_flowDiagSolution )
    {
        RimFlowDiagSolution* defaultFlowSolution = m_case->defaultFlowDiagSolution();
        if ( defaultFlowSolution )
        {
            options.push_back( caf::PdmOptionItemInfo( "Default Flow Diag Solution", defaultFlowSolution ) );
        }
    }
    else if ( m_case && ( fieldNeedingOptions == &m_selectedFromTimeStep || fieldNeedingOptions == &m_selectedToTimeStep ||
                          fieldNeedingOptions == &m_selectedTimeStep ) )
    {
        const QString dateFormatStr = dateFormatString();
        const auto    timeSteps     = m_case->timeStepDates();
        for ( const auto& timeStep : timeSteps )
        {
            options.push_back( caf::PdmOptionItemInfo( timeStep.toString( dateFormatStr ), timeStep ) );
        }
    }
    else if ( m_case && ( fieldNeedingOptions == &m_excludeTimeSteps ) )
    {
        const QString dateFormatStr     = dateFormatString();
        const auto    selectedTimeSteps = getSelectedTimeSteps( m_case->timeStepDates() );
        for ( const auto& timeStep : selectedTimeSteps )
        {
            options.push_back( caf::PdmOptionItemInfo( timeStep.toString( dateFormatStr ), timeStep ) );
        }
    }
    else if ( fieldNeedingOptions == &m_selectedInjectorTracersUiField && m_flowDiagSolution() )
    {
        const bool isInjector = true;
        options               = RimFlowDiagnosticsTools::calcOptionsForSelectedTracerField( m_flowDiagSolution(), isInjector );
        if ( !options.empty() )
        {
            options.push_back( caf::PdmOptionItemInfo( RiaDefines::reservoirTracerName(), RiaDefines::reservoirTracerName() ) );
        }
    }
    else if ( fieldNeedingOptions == &m_selectedProducerTracersUiField && m_flowDiagSolution() )
    {
        const bool isInjector = false;
        options               = RimFlowDiagnosticsTools::calcOptionsForSelectedTracerField( m_flowDiagSolution(), isInjector );
    }
    else if ( fieldNeedingOptions == &m_axisTitleFontSize || fieldNeedingOptions == &m_axisLabelFontSize ||
              fieldNeedingOptions == &m_valueLabelFontSize )
    {
        options = caf::FontTools::relativeSizeValueOptions( RiaPreferences::current()->defaultPlotFontSize() );
    }
    return options;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QWidget* RimWellConnectivityTable::viewWidget()
{
    return m_matrixPlotWidget;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QImage RimWellConnectivityTable::snapshotWindowContent()
{
    QImage image;

    if ( m_matrixPlotWidget )
    {
        QPixmap pix = m_matrixPlotWidget->grab();
        image       = pix.toImage();
    }

    return image;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::zoomAll()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QWidget* RimWellConnectivityTable::createViewWidget( QWidget* mainWindowParent )
{
    m_matrixPlotWidget = new RiuMatrixPlotWidget( this, m_legendConfig, mainWindowParent );
    m_matrixPlotWidget->setShowValueLabel( m_showValueLabels );
    m_matrixPlotWidget->setUseInvalidValueColor( true );

    return m_matrixPlotWidget;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::deleteViewWidget()
{
    cleanupBeforeClose();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimWellConnectivityTable::description() const
{
    return QString();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::doRenderWindowContent( QPaintDevice* paintDevice )
{
    return;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RimWellConnectivityTable::axisTitleFontSize() const
{
    return caf::FontTools::absolutePointSize( RiaPreferences::current()->defaultPlotFontSize(), m_axisTitleFontSize() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RimWellConnectivityTable::axisLabelFontSize() const
{
    return caf::FontTools::absolutePointSize( RiaPreferences::current()->defaultPlotFontSize(), m_axisLabelFontSize() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RimWellConnectivityTable::valueLabelFontSize() const
{
    return caf::FontTools::absolutePointSize( RiaPreferences::current()->defaultPlotFontSize(), m_valueLabelFontSize() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RimWellConnectivityTable::getProductionWellNames() const
{
    if ( !m_flowDiagSolution ) return {};

    std::vector<QString> productionWellNames;
    if ( m_timeStepSelection == TimeStepSelection::SINGLE_TIME_STEP )
    {
        const auto timeStepIndex = getTimeStepIndex( m_selectedTimeStep, m_case->timeStepDates() );
        productionWellNames      = RimFlowDiagnosticsTools::producerTracersInTimeStep( m_flowDiagSolution(), timeStepIndex );
    }
    else if ( m_timeStepSelection == TimeStepSelection::TIME_STEP_RANGE )
    {
        const auto selectedTimeSteps = getSelectedTimeSteps( m_case->timeStepDates() );
        productionWellNames          = getProductionWellNamesAtTimeSteps( selectedTimeSteps );
    }
    return productionWellNames;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RimWellConnectivityTable::getProductionWellNamesAtTimeSteps( const std::set<QDateTime>& timeSteps ) const
{
    if ( !m_flowDiagSolution ) return {};

    std::set<QString> productionWellNames;
    const auto        allTimeSteps = m_case->timeStepDates();
    for ( const auto& timeStep : timeSteps )
    {
        const auto timeStepIndex = getTimeStepIndex( timeStep, allTimeSteps );

        if ( timeStepIndex < 0 ) continue;

        const auto producers = RimFlowDiagnosticsTools::producerTracersInTimeStep( m_flowDiagSolution(), timeStepIndex );
        for ( const auto& producer : producers )
        {
            productionWellNames.insert( producer );
        }
    }

    return std::vector<QString>( productionWellNames.begin(), productionWellNames.end() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimWellConnectivityTable::createTableTitle() const
{
    RiaDefines::EclipseUnitSystem     unitSet   = m_case->eclipseCaseData()->unitsType();
    RimWellLogFile::WellFlowCondition condition = RimWellLogFile::WELL_FLOW_COND_RESERVOIR;

    auto timeSampleValueTypeText = [&]() -> QString {
        if ( m_timeSampleValueType == TimeSampleValueType::FLOW_RATE_PERCENTAGE )
        {
            return QString( "Percentage of Total Reservoir Flow Rate [%]" );
        }
        if ( m_timeSampleValueType == TimeSampleValueType::FLOW_RATE_FRACTION )
        {
            return QString( "Fraction of Total Reservoir Flow Rate" );
        }
        if ( m_timeSampleValueType == TimeSampleValueType::FLOW_RATE )
        {
            return "Total " + RimWellPlotTools::flowPlotAxisTitle( condition, unitSet );
        }
        return QString();
    };

    auto timeRangeValueTypeText = [&]() -> QString {
        if ( m_timeRangeValueType() == TimeRangeValueType::ACCUMULATED_FLOW_VOLUME )
        {
            return "Accumulated Total " + RimWellPlotTools::flowVolumePlotAxisTitle( condition, unitSet );
        }
        if ( m_timeRangeValueType() == TimeRangeValueType::ACCUMULATED_FLOW_VOLUME_PERCENTAGE )
        {
            return QString( "Accumulated Total Reservoir Flow Volume Allocation [%]" );
        }
        if ( m_timeRangeValueType() == TimeRangeValueType::ACCUMULATED_FLOW_VOLUME_FRACTION )
        {
            return QString( "Accumulated Total Reservoir Flow Volume Allocation Fraction" );
        }
        return QString();
    };

    if ( m_timeStepSelection() == TimeStepSelection::SINGLE_TIME_STEP )
    {
        return QString( "%1 (%2)<br>Date: %3</br>" )
            .arg( timeSampleValueTypeText() )
            .arg( m_case->caseUserDescription() )
            .arg( m_selectedTimeStep().toString( dateFormatString() ) );
    }
    if ( m_timeStepSelection() == TimeStepSelection::TIME_STEP_RANGE )
    {
        return QString( "%1 (%2)<br>Date range: %3 - %4, Number of time steps: %5</br>" )
            .arg( timeRangeValueTypeText() )
            .arg( m_case->caseUserDescription() )
            .arg( m_selectedFromTimeStep().toString( dateFormatString() ) )
            .arg( m_selectedToTimeStep().toString( dateFormatString() ) )
            .arg( m_timeStepCount() );
    }

    return "";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::pair<double, double> RimWellConnectivityTable::createLegendMinMaxValues( const double maxTableValue ) const
{
    const bool isSingleTimeStep = m_timeStepSelection == TimeStepSelection::SINGLE_TIME_STEP;
    const bool isTimeStepRange  = m_timeStepSelection == TimeStepSelection::TIME_STEP_RANGE;
    if ( ( isSingleTimeStep && m_timeSampleValueType == TimeSampleValueType::FLOW_RATE_PERCENTAGE ) ||
         ( isTimeStepRange && m_timeRangeValueType == TimeRangeValueType::ACCUMULATED_FLOW_VOLUME_PERCENTAGE ) )
    {
        return std::make_pair( 0.0, 100.0 );
    }
    if ( ( isSingleTimeStep && m_timeSampleValueType == TimeSampleValueType::FLOW_RATE_FRACTION ) ||
         ( isTimeStepRange && m_timeRangeValueType == TimeRangeValueType::ACCUMULATED_FLOW_VOLUME_FRACTION ) )
    {
        return std::make_pair( 0.0, 1.0 );
    }
    return std::make_pair( 0.0, maxTableValue );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::setValidTimeStepSelectionsForCase()
{
    setValidSingleTimeStepForCase();
    setValidTimeStepRangeForCase();
}

//--------------------------------------------------------------------------------------------------
/// Update selected "Time Step" according to selected case.
/// If selected time steps exist for case, keep as is, otherwise select first time step in case.
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::setValidSingleTimeStepForCase()
{
    if ( !m_case || m_case->timeStepDates().empty() )
    {
        return;
    }
    if ( m_selectedTimeStep().isValid() && isTimeStepInCase( m_selectedTimeStep() ) )
    {
        return;
    }

    m_selectedTimeStep = m_case->timeStepDates().front();
}

//--------------------------------------------------------------------------------------------------
/// Update selected "From Time Step" and "To Time Step" according to selected case.
/// If both selected time steps exist for case, keep as is, otherwise select first and last time
/// step in case.
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::setValidTimeStepRangeForCase()
{
    if ( !m_case || m_case->timeStepDates().empty() )
    {
        return;
    }
    if ( m_selectedFromTimeStep().isValid() && isTimeStepInCase( m_selectedFromTimeStep() ) && m_selectedToTimeStep().isValid() &&
         isTimeStepInCase( m_selectedToTimeStep() ) )
    {
        return;
    }

    m_selectedFromTimeStep = m_case->timeStepDates().front();
    m_selectedToTimeStep   = m_case->timeStepDates().back();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimWellConnectivityTable::isTimeStepInCase( const QDateTime& timeStep ) const
{
    if ( !m_case ) return false;

    return std::find( m_case->timeStepDates().cbegin(), m_case->timeStepDates().cend(), timeStep ) != m_case->timeStepDates().cend();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RimWellConnectivityTable::getTimeStepIndex( const QDateTime timeStep, const std::vector<QDateTime> timeSteps ) const
{
    auto itr = std::find( timeSteps.begin(), timeSteps.end(), timeStep );
    if ( itr == timeSteps.end() ) return -1;
    return static_cast<int>( std::distance( timeSteps.begin(), itr ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::setSelectedProducersAndInjectorsForSingleTimeStep()
{
    if ( !m_case || !m_flowDiagSolution || !m_selectedTimeStep().isValid() ) return;

    const int timeStepIndex = getTimeStepIndex( m_selectedTimeStep, m_case->timeStepDates() );
    if ( timeStepIndex == -1 )
    {
        m_selectedProducerTracersUiField = std::vector<QString>();
        m_selectedInjectorTracersUiField = std::vector<QString>();
    }

    const std::vector<QString> producerVec = RimFlowDiagnosticsTools::producerTracersInTimeStep( m_flowDiagSolution, timeStepIndex );
    std::vector<QString>       injectorVec = RimFlowDiagnosticsTools::injectorTracersInTimeStep( m_flowDiagSolution, timeStepIndex );
    injectorVec.push_back( RiaDefines::reservoirTracerName() );

    // No filtering if all producers/injectors are selected and assign to UI-elements
    m_selectedProducerTracersUiField = producerVec;
    m_selectedInjectorTracersUiField = injectorVec;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::setSelectedProducersAndInjectorsForTimeStepRange()
{
    if ( !m_case || !m_flowDiagSolution || !m_selectedTimeStep().isValid() ) return;

    const auto                allTimeSteps      = m_case->timeStepDates();
    const std::set<QDateTime> selectedTimeSteps = getSelectedTimeSteps( allTimeSteps );
    const std::set<QDateTime> excludedTimeSteps = std::set( m_excludeTimeSteps().begin(), m_excludeTimeSteps().end() );

    std::set<QString> producerSet;
    std::set<QString> injectorSet;
    for ( const auto& timeStep : selectedTimeSteps )
    {
        if ( excludedTimeSteps.contains( timeStep ) ) continue;

        const auto timeStepIndex     = getTimeStepIndex( timeStep, allTimeSteps );
        const auto timeStepProducers = RimFlowDiagnosticsTools::producerTracersInTimeStep( m_flowDiagSolution, timeStepIndex );
        const auto timeStepInjectors = RimFlowDiagnosticsTools::injectorTracersInTimeStep( m_flowDiagSolution, timeStepIndex );

        // Insert vectors into set
        std::copy( timeStepProducers.begin(), timeStepProducers.end(), std::inserter( producerSet, producerSet.end() ) );
        std::copy( timeStepInjectors.begin(), timeStepInjectors.end(), std::inserter( injectorSet, injectorSet.end() ) );
    }
    injectorSet.insert( RiaDefines::reservoirTracerName() );

    // Assign to UI-elements
    m_selectedProducerTracersUiField = std::vector<QString>( producerSet.begin(), producerSet.end() );
    m_selectedInjectorTracersUiField = std::vector<QString>( injectorSet.begin(), injectorSet.end() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::syncSelectedInjectorsFromProducerSelection()
{
    std::vector<QString> injectors;
    if ( m_timeStepSelection == TimeStepSelection::SINGLE_TIME_STEP )
    {
        const auto timeStepIndex = getTimeStepIndex( m_selectedTimeStep, m_case->timeStepDates() );
        const auto newInjectors  = RimFlowDiagnosticsTools::setOfInjectorTracersFromProducers( m_flowDiagSolution(),
                                                                                              m_selectedProducerTracersUiField(),
                                                                                              timeStepIndex );

        injectors = std::vector<QString>( newInjectors.begin(), newInjectors.end() );
        injectors.push_back( RiaDefines::reservoirTracerName() );
    }
    if ( m_timeStepSelection == TimeStepSelection::TIME_STEP_RANGE )
    {
        const auto                allTimeSteps      = m_case->timeStepDates();
        const std::set<QDateTime> selectedTimeSteps = getSelectedTimeSteps( allTimeSteps );
        std::vector<int>          timeStepIndices;
        for ( const auto& timeStep : selectedTimeSteps )
        {
            timeStepIndices.push_back( getTimeStepIndex( timeStep, allTimeSteps ) );
        }
        const auto newInjectors = RimFlowDiagnosticsTools::setOfInjectorTracersFromProducers( m_flowDiagSolution(),
                                                                                              m_selectedProducerTracersUiField(),
                                                                                              timeStepIndices );

        injectors = std::vector<QString>( newInjectors.begin(), newInjectors.end() );
        injectors.push_back( RiaDefines::reservoirTracerName() );
    }

    m_selectedInjectorTracersUiField = injectors;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::syncSelectedProducersFromInjectorSelection()
{
    std::vector<QString> producers;
    if ( m_timeStepSelection == TimeStepSelection::SINGLE_TIME_STEP )
    {
        const auto timeStepIndex = getTimeStepIndex( m_selectedTimeStep, m_case->timeStepDates() );
        const auto newProducers  = RimFlowDiagnosticsTools::setOfProducerTracersFromInjectors( m_flowDiagSolution(),
                                                                                              m_selectedInjectorTracersUiField(),
                                                                                              timeStepIndex );

        producers = std::vector<QString>( newProducers.begin(), newProducers.end() );
    }
    if ( m_timeStepSelection == TimeStepSelection::TIME_STEP_RANGE )
    {
        const auto          allTimeSteps      = m_case->timeStepDates();
        std::set<QDateTime> selectedTimeSteps = getSelectedTimeSteps( allTimeSteps );
        std::vector<int>    timeStepIndices;
        for ( const auto& timeStep : selectedTimeSteps )
        {
            timeStepIndices.push_back( getTimeStepIndex( timeStep, allTimeSteps ) );
        }
        const auto newProducers = RimFlowDiagnosticsTools::setOfProducerTracersFromInjectors( m_flowDiagSolution(),
                                                                                              m_selectedInjectorTracersUiField(),
                                                                                              timeStepIndices );

        producers = std::vector<QString>( newProducers.begin(), newProducers.end() );
        producers.push_back( RiaDefines::reservoirTracerName() );
    }

    m_selectedProducerTracersUiField = producers;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::onCellFiltersChanged( const SignalEmitter* emitter )
{
    onLoadDataAndUpdate();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::connectViewCellFiltersChangedToSlot( RimEclipseView* view )
{
    if ( !view || !view->cellFilterCollection() ) return;

    view->cellFilterCollection()->filtersChanged.connect( this, &RimWellConnectivityTable::onCellFiltersChanged );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::disconnectViewCellFiltersChangedFromSlots( RimEclipseView* view )
{
    if ( !view || !view->cellFilterCollection() ) return;

    view->cellFilterCollection()->filtersChanged.disconnect( this );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::map<QString, RigWellAllocationOverTime>
    RimWellConnectivityTable::createProductionWellsAllocationOverTimeMap( const std::set<QString>& selectedProductionWells ) const
{
    if ( !m_case || !m_flowDiagSolution )
    {
        return std::map<QString, RigWellAllocationOverTime>();
    }

    std::map<QString, RigWellAllocationOverTime> wellAllocationOverTimeMap;

    // Create well allocation over time for each production well
    std::vector<QString> prodWellNames = getProductionWellNames();
    for ( const auto& wellName : prodWellNames )
    {
        if ( !selectedProductionWells.empty() && !selectedProductionWells.contains( wellName ) ) continue;

        const RigSimWellData* simWellData = m_case->eclipseCaseData()->findSimWellData( wellName );
        if ( !simWellData ) continue;

        wellAllocationOverTimeMap.emplace( wellName, createWellAllocationOverTime( simWellData ) );
    }

    return wellAllocationOverTimeMap;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RigWellAllocationOverTime RimWellConnectivityTable::createWellAllocationOverTime( const RigSimWellData* simWellData ) const
{
    if ( !m_case || !simWellData || !m_flowDiagSolution )
    {
        return RigWellAllocationOverTime( {}, {} );
    }

    const std::vector<QDateTime>                  allTimeSteps = m_case->timeStepDates();
    std::map<QDateTime, RigAccWellFlowCalculator> timeStepAndCalculatorPairs;

    if ( m_timeStepSelection() == TimeStepSelection::TIME_STEP_RANGE )
    {
        std::set<QDateTime> selectedTimeSteps = getSelectedTimeSteps( allTimeSteps );
        std::set<QDateTime> excludedTimeSteps = std::set( m_excludeTimeSteps().begin(), m_excludeTimeSteps().end() );
        for ( const auto& timeStep : allTimeSteps )
        {
            if ( !selectedTimeSteps.contains( timeStep ) || excludedTimeSteps.contains( timeStep ) )
            {
                continue;
            }

            const auto timeStepIndex = getTimeStepIndex( timeStep, allTimeSteps );
            createAndEmplaceTimeStepAndCalculatorPairInMap( timeStepAndCalculatorPairs, timeStep, timeStepIndex, simWellData );
        }

        // Create well allocation over time data
        const auto                selectedTimeStepsVector = std::vector( selectedTimeSteps.begin(), selectedTimeSteps.end() );
        RigWellAllocationOverTime wellAllocationOverTime( selectedTimeStepsVector, timeStepAndCalculatorPairs );

        // NB: No threshold when generating calculators - filtering must be done after collecting data
        // for each producer!
        const double smallContributionThreshold = 0.0;
        if ( m_timeRangeValueType == TimeRangeValueType::ACCUMULATED_FLOW_VOLUME )
        {
            wellAllocationOverTime.fillWithAccumulatedFlowVolumeValues( smallContributionThreshold );
        }
        else if ( m_timeRangeValueType == TimeRangeValueType::ACCUMULATED_FLOW_VOLUME_FRACTION )
        {
            wellAllocationOverTime.fillWithAccumulatedFlowVolumeFractionValues( smallContributionThreshold );
        }
        else if ( m_timeRangeValueType == TimeRangeValueType::ACCUMULATED_FLOW_VOLUME_PERCENTAGE )
        {
            wellAllocationOverTime.fillWithAccumulatedFlowVolumePercentageValues( smallContributionThreshold );
        }
        return wellAllocationOverTime;
    }

    if ( m_timeStepSelection() == TimeStepSelection::SINGLE_TIME_STEP )
    {
        const auto timeStepIndex = getTimeStepIndex( m_selectedTimeStep(), allTimeSteps );
        createAndEmplaceTimeStepAndCalculatorPairInMap( timeStepAndCalculatorPairs, m_selectedTimeStep(), timeStepIndex, simWellData );

        // Create well allocation over time data
        RigWellAllocationOverTime wellAllocationOverTime( { m_selectedTimeStep() }, timeStepAndCalculatorPairs );

        if ( m_timeSampleValueType == TimeSampleValueType::FLOW_RATE )
        {
            wellAllocationOverTime.fillWithFlowRateValues();
        }
        else if ( m_timeSampleValueType == TimeSampleValueType::FLOW_RATE_FRACTION )
        {
            wellAllocationOverTime.fillWithFlowRateFractionValues();
        }
        else if ( m_timeSampleValueType == TimeSampleValueType::FLOW_RATE_PERCENTAGE )
        {
            wellAllocationOverTime.fillWithFlowRatePercentageValues();
        }
        return wellAllocationOverTime;
    }

    return RigWellAllocationOverTime( {}, {} );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellConnectivityTable::createAndEmplaceTimeStepAndCalculatorPairInMap( std::map<QDateTime, RigAccWellFlowCalculator>& rTimeStepAndCalculatorPairs,
                                                                               const QDateTime                                timeStep,
                                                                               int                                            timeStepIndex,
                                                                               const RigSimWellData* simWellData ) const
{
    if ( timeStepIndex < 0 ) return;

    // NB: No threshold when generating calculators!
    const double smallContributionThreshold = 0.0;
    const bool   branchDetection            = false;

    auto simWellBranches = RigSimulationWellCenterLineCalculator::calculateWellPipeCenterlineForTimeStep( m_case->eclipseCaseData(),
                                                                                                          simWellData,
                                                                                                          timeStepIndex,
                                                                                                          branchDetection,
                                                                                                          true );

    const auto& [pipeBranchesCLCoords, pipeBranchesCellIds] = RigSimulationWellCenterLineCalculator::extractBranchData( simWellBranches );

    std::map<QString, const std::vector<double>*> tracerFractionCellValues =
        RimWellAllocationTools::findOrCreateRelevantTracerCellFractions( simWellData, m_flowDiagSolution, timeStepIndex );

    if ( !tracerFractionCellValues.empty() && !pipeBranchesCLCoords.empty() )
    {
        bool isProducer = ( simWellData->wellProductionType( timeStepIndex ) == RiaDefines::WellProductionType::PRODUCER ||
                            simWellData->wellProductionType( timeStepIndex ) == RiaDefines::WellProductionType::UNDEFINED_PRODUCTION_TYPE );

        // Retrieve cell visibilities for valid cell filter view
        const auto*               cellVisibilities = m_cellFilterView ? m_cellFilterView->currentTotalCellVisibility().p() : nullptr;
        RigEclCellIndexCalculator cellIdxCalc( m_case->eclipseCaseData()->mainGrid(),
                                               m_case->eclipseCaseData()->activeCellInfo( RiaDefines::PorosityModelType::MATRIX_MODEL ),
                                               cellVisibilities );

        const auto calculator = RigAccWellFlowCalculator( pipeBranchesCLCoords,
                                                          pipeBranchesCellIds,
                                                          tracerFractionCellValues,
                                                          cellIdxCalc,
                                                          smallContributionThreshold,
                                                          isProducer );
        rTimeStepAndCalculatorPairs.emplace( timeStep, calculator );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::set<QDateTime> RimWellConnectivityTable::getSelectedTimeSteps( const std::vector<QDateTime>& timeSteps ) const
{
    const auto timeStepsInRange =
        RiaQDateTimeTools::getTimeStepsWithinSelectedRange( timeSteps, m_selectedFromTimeStep(), m_selectedToTimeStep() );
    return m_timeStepFilterMode == TimeStepRangeFilterMode::TIME_STEP_COUNT
               ? RiaQDateTimeTools::createEvenlyDistributedDates( timeStepsInRange, m_timeStepCount )
               : std::set( timeStepsInRange.begin(), timeStepsInRange.end() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimWellConnectivityTable::dateFormatString() const
{
    return RiaQDateTimeTools::dateFormatString( RiaPreferences::current()->dateFormat(),
                                                RiaDefines::DateFormatComponents::DATE_FORMAT_YEAR_MONTH_DAY );
}