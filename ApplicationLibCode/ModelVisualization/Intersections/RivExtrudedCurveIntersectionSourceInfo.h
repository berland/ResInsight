/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Statoil ASA
//  Copyright (C) Ceetron Solutions AS
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

#pragma once

#include "cvfArray.h"
#include "cvfObject.h"
#include <array>

class RivExtrudedCurveIntersectionGeometryGenerator;
class RimExtrudedCurveIntersection;

class RivExtrudedCurveIntersectionSourceInfo : public cvf::Object
{
public:
    explicit RivExtrudedCurveIntersectionSourceInfo( RivExtrudedCurveIntersectionGeometryGenerator* geometryGenerator );

    const std::vector<size_t>&    triangleToCellIndex() const;
    std::array<cvf::Vec3f, 3>     triangle( int triangleIdx ) const;
    RimExtrudedCurveIntersection* intersection() const;

private:
    cvf::cref<RivExtrudedCurveIntersectionGeometryGenerator> m_intersectionGeometryGenerator;
};
