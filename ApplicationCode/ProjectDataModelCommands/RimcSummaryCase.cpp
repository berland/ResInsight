/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2020- Equinor ASA
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

#include "RimcSummaryCase.h"

#include "RifSummaryReaderInterface.h"
#include "RimSummaryCase.h"
#include "RimcDataContainerDouble.h"

#include "cafPdmFieldIOScriptability.h"

CAF_PDM_OBJECT_METHOD_SOURCE_INIT( RimSummaryCase, RimcSummaryCase_summaryVectorValues, "SummaryVectorValues" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimcSummaryCase_summaryVectorValues::RimcSummaryCase_summaryVectorValues( caf::PdmObjectHandle* self )
    : caf::PdmObjectMethod( self )
{
    CAF_PDM_InitObject( "Create Summary Plot", "", "", "Create a new Summary Plot" );
    CAF_PDM_InitScriptableFieldWithIONoDefault( &m_addressString,
                                                "Address",
                                                "",
                                                "",
                                                "",
                                                "Formatted address specifying the summary vector" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmObjectHandle* RimcSummaryCase_summaryVectorValues::execute()
{
    QStringList addressStrings = m_addressString().split( ";", QString::SkipEmptyParts );

    auto*                      summaryCase = self<RimSummaryCase>();
    RifSummaryReaderInterface* sumReader   = summaryCase->summaryReader();

    auto adr = RifEclipseSummaryAddress::fromEclipseTextAddress( m_addressString().toStdString() );

    std::vector<double> values;
    bool                isOk = sumReader->values( adr, &values );

    if ( isOk )
    {
        auto dataObject            = new RimcDataContainerDouble();
        dataObject->m_doubleValues = values;

        return dataObject;
    }

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimcSummaryCase_summaryVectorValues::resultIsPersistent() const
{
    return false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::unique_ptr<caf::PdmObjectHandle> RimcSummaryCase_summaryVectorValues::defaultResult() const
{
    return std::unique_ptr<caf::PdmObjectHandle>( new RimcDataContainerDouble );
}