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

#include "RicCreateDepthAdjustedLasFilesUi.h"

#include "RiaApplication.h"

#include "RimCase.h"
#include "RimOilField.h"
#include "RimProject.h"
#include "RimTools.h"
#include "RimWellLogFile.h"
#include "RimWellLogFileChannel.h"
#include "RimWellPath.h"
#include "RimWellPathCollection.h"

#include "cafPdmUiCheckBoxEditor.h"
#include "cafPdmUiFilePathEditor.h"
#include "cafPdmUiOrdering.h"
#include "cafPdmUiTreeSelectionEditor.h"

CAF_PDM_SOURCE_INIT( RicCreateDepthAdjustedLasFilesUi, "RicCreateDepthAdjustedLasFilesUi" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RicCreateDepthAdjustedLasFilesUi::RicCreateDepthAdjustedLasFilesUi()
{
    CAF_PDM_InitField( &exportFolder, "ExportFolder", QString(), "Export Folder" );
    exportFolder.uiCapability()->setUiEditorTypeName( caf::PdmUiFilePathEditor::uiEditorTypeName() );

    CAF_PDM_InitFieldNoDefault( &selectedCase, "SelectedCase", "Select Case" );
    CAF_PDM_InitFieldNoDefault( &sourceWell, "SourceWell", "Source Well" );
    CAF_PDM_InitFieldNoDefault( &selectedResultProperties, "SelectedResultProperties", "Selected Result Properties" );
    selectedResultProperties.uiCapability()->setUiEditorTypeName( caf::PdmUiTreeSelectionEditor::uiEditorTypeName() );
    CAF_PDM_InitFieldNoDefault( &destinationWells, "DestinationWells", "Destination Wells" );
    destinationWells.uiCapability()->setUiEditorTypeName( caf::PdmUiTreeSelectionEditor::uiEditorTypeName() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RicCreateDepthAdjustedLasFilesUi::~RicCreateDepthAdjustedLasFilesUi()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo>
    RicCreateDepthAdjustedLasFilesUi::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    QList<caf::PdmOptionItemInfo> options;

    if ( fieldNeedingOptions == &selectedCase )
    {
        RimTools::caseOptionItems( &options );
    }
    if ( fieldNeedingOptions == &sourceWell )
    {
        RimProject* proj = RimProject::current();
        if ( proj )
        {
            std::vector<RimWellPath*> allWellPaths = proj->activeOilField()->wellPathCollection->allWellPaths();
            for ( auto* wellPath : allWellPaths )
            {
                if ( !wellPath->wellLogFiles().empty() )
                {
                    options.push_back( caf::PdmOptionItemInfo( wellPath->name(), wellPath ) );
                }
            }
        }
    }
    if ( fieldNeedingOptions == &selectedResultProperties )
    {
        if ( sourceWell && !sourceWell->wellLogFiles().empty() )
        {
            auto* firstWellLogFile = sourceWell->wellLogFiles()[0];
            for ( auto* property : firstWellLogFile->wellLogChannels() )
            {
                if ( !m_depthProperties.contains( property->name() ) )
                {
                    options.push_back( caf::PdmOptionItemInfo( property->name(), property->name() ) );
                }
            }
        }
    }
    if ( fieldNeedingOptions == &destinationWells )
    {
        RimProject* proj = RimProject::current();
        if ( proj )
        {
            std::vector<RimWellPath*> allWellPaths = proj->activeOilField()->wellPathCollection->allWellPaths();
            for ( auto* wellPath : allWellPaths )
            {
                if ( wellPath != sourceWell )
                {
                    options.push_back( caf::PdmOptionItemInfo( wellPath->name(), wellPath ) );
                }
            }
        }
    }

    return options;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCreateDepthAdjustedLasFilesUi::fieldChangedByUi( const caf::PdmFieldHandle* changedField,
                                                         const QVariant&            oldValue,
                                                         const QVariant&            newValue )
{
    if ( changedField == &sourceWell )
    {
        selectedResultProperties.v().clear();
        destinationWells.clearWithoutDelete();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCreateDepthAdjustedLasFilesUi::defineEditorAttribute( const caf::PdmFieldHandle* field,
                                                              QString                    uiConfigName,
                                                              caf::PdmUiEditorAttribute* attribute )
{
    if ( field == &exportFolder )
    {
        caf::PdmUiFilePathEditorAttribute* myAttr = dynamic_cast<caf::PdmUiFilePathEditorAttribute*>( attribute );
        if ( myAttr )
        {
            myAttr->m_selectDirectory = true;
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCreateDepthAdjustedLasFilesUi::setDefaultValues()
{
    // Default folder directory
    QString defaultDir =
        RiaApplication::instance()->lastUsedDialogDirectoryWithFallbackToProjectFolder( "WELL_LOGS_DIR" );
    exportFolder = defaultDir;

    // Default selected case and source well
    RimProject* proj = RimProject::current();
    if ( proj )
    {
        std::vector<RimCase*> allCases;
        proj->allCases( allCases );
        if ( !allCases.empty() ) selectedCase = allCases[0];

        std::vector<RimWellPath*> allWellPaths = proj->activeOilField()->wellPathCollection->allWellPaths();
        for ( auto* wellPath : allWellPaths )
        {
            if ( !wellPath->wellLogFiles().empty() )
            {
                sourceWell = wellPath;
                break;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicCreateDepthAdjustedLasFilesUi::hasValidSelections() const
{
    return !exportFolder().isEmpty() && sourceWell() != nullptr && selectedCase() != nullptr &&
           !selectedResultProperties().empty() && !destinationWells.empty();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RicCreateDepthAdjustedLasFilesUi::invalidSelectionsLogString() const
{
    if ( hasValidSelections() )
    {
        return QString();
    }

    QString logStr;
    if ( exportFolder().isEmpty() )
    {
        logStr += "Selected Export folder is empty!\n";
    }
    if ( selectedCase() == nullptr )
    {
        logStr += "Selected case is not defined!\n";
    }
    if ( sourceWell() == nullptr )
    {
        logStr += "Source well is not defined!\n";
    }
    if ( selectedResultProperties().empty() )
    {
        logStr += "No result properties are selected!\n";
    }
    if ( destinationWells.empty() )
    {
        logStr += "No destination wells are selected!\n";
    }

    return logStr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCreateDepthAdjustedLasFilesUi::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &exportFolder );
    uiOrdering.add( &selectedCase );
    uiOrdering.add( &sourceWell );
    uiOrdering.add( &selectedResultProperties );
    uiOrdering.add( &destinationWells );

    uiOrdering.skipRemainingFields( true );
}