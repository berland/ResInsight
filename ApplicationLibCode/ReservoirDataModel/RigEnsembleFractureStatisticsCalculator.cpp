/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2020 Equinor ASA
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

#include "RigEnsembleFractureStatisticsCalculator.h"
#include "RigHistogramData.h"
#include "RigStatisticsMath.h"
#include "RigStimPlanFractureDefinition.h"

#include "RimEnsembleFractureStatistics.h"

#include "cafAppEnum.h"

#include "cvfObject.h"

#include <limits>

namespace caf
{
template <>
void caf::AppEnum<RigEnsembleFractureStatisticsCalculator::PropertyType>::setUp()
{
    addItem( RigEnsembleFractureStatisticsCalculator::PropertyType::HEIGHT, "HEIGHT", "Height" );
    addItem( RigEnsembleFractureStatisticsCalculator::PropertyType::AREA, "AREA", "Area" );
    setDefault( RigEnsembleFractureStatisticsCalculator::PropertyType::HEIGHT );
}
}; // namespace caf

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RigHistogramData RigEnsembleFractureStatisticsCalculator::createStatisticsData( RimEnsembleFractureStatistics* esf,
                                                                                PropertyType propertyType )
{
    std::vector<cvf::ref<RigStimPlanFractureDefinition>> defs = esf->readFractureDefinitions();

    std::vector<double> samples;
    for ( auto def : defs )
    {
        double height = def->ys().front() - def->ys().back();
        samples.push_back( height );
    }

    RigHistogramData histogramData;

    double sum;
    double range;
    double dev;
    RigStatisticsMath::calculateBasicStatistics( samples,
                                                 &histogramData.min,
                                                 &histogramData.max,
                                                 &sum,
                                                 &range,
                                                 &histogramData.mean,
                                                 &dev );

    double p50;
    double mean;
    RigStatisticsMath::calculateStatisticsCurves( samples, &histogramData.p10, &p50, &histogramData.p90, &mean );

    // TODO: this leaks memory: api assume the the histogram is owned by someone else (which is kind-of-strange)
    std::vector<size_t>*   histogram = new std::vector<size_t>();
    RigHistogramCalculator histogramCalculator( histogramData.min, histogramData.max, 20, histogram );
    for ( auto s : samples )
        histogramCalculator.addValue( s );

    histogramData.histogram = histogram;

    return histogramData;
}
