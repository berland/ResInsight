#include "RimFileWellPath.h"

#include "RicfCommandObject.h"

#include "RifWellPathImporter.h"

#include "RimProject.h"
#include "RimTools.h"

#include "cafUtils.h"

#include "QDir"
#include "QFileInfo"

CAF_PDM_SOURCE_INIT( RimFileWellPath, "WellPath" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimFileWellPath::RimFileWellPath()
{
    CAF_PDM_InitScriptableObjectWithNameAndComment( "File Well Path",
                                                    ":/Well.svg",
                                                    "",
                                                    "",
                                                    "FileWellPath",
                                                    "Well Paths Loaded From File" );

    CAF_PDM_InitFieldNoDefault( &id, "WellPathId", "Id" );
    id.uiCapability()->setUiReadOnly( true );
    id.xmlCapability()->disableIO();
    CAF_PDM_InitFieldNoDefault( &sourceSystem, "SourceSystem", "Source System" );
    sourceSystem.uiCapability()->setUiReadOnly( true );
    sourceSystem.xmlCapability()->disableIO();
    CAF_PDM_InitFieldNoDefault( &utmZone, "UTMZone", "UTM Zone" );
    utmZone.uiCapability()->setUiReadOnly( true );
    utmZone.xmlCapability()->disableIO();
    CAF_PDM_InitFieldNoDefault( &updateDate, "WellPathUpdateDate", "Update Date" );
    updateDate.uiCapability()->setUiReadOnly( true );
    updateDate.xmlCapability()->disableIO();
    CAF_PDM_InitFieldNoDefault( &updateUser, "WellPathUpdateUser", "Update User" );
    updateUser.uiCapability()->setUiReadOnly( true );
    updateUser.xmlCapability()->disableIO();
    CAF_PDM_InitFieldNoDefault( &m_surveyType, "WellPathSurveyType", "Survey Type" );
    m_surveyType.uiCapability()->setUiReadOnly( true );
    m_surveyType.xmlCapability()->disableIO();

    CAF_PDM_InitFieldNoDefault( &m_filePath, "WellPathFilepath", "File Path" );
    m_filePath.uiCapability()->setUiReadOnly( true );
    CAF_PDM_InitFieldNoDefault( &m_filePathInCache, "WellPathFilePathInCache", "File Name" );
    m_filePathInCache.uiCapability()->setUiReadOnly( true );

    CAF_PDM_InitField( &m_wellPathIndexInFile, "WellPathNumberInFile", -1, "Well Number in File" );
    m_wellPathIndexInFile.uiCapability()->setUiReadOnly( true );

    CAF_PDM_InitField( &m_useAutoGeneratedPointAtSeaLevel,
                       "UseAutoGeneratedPointAtSeaLevel",
                       false,
                       "Generate Point at Sea Level" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimFileWellPath::~RimFileWellPath()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimFileWellPath::filePath() const
{
    if ( isStoredInCache() || m_filePath().path().isEmpty() )
    {
        return m_filePathInCache();
    }
    else
    {
        return m_filePath().path();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFileWellPath::setFilepath( const QString& path )
{
    m_filePath = path;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RimFileWellPath::wellPathIndexInFile() const
{
    return m_wellPathIndexInFile();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFileWellPath::setWellPathIndexInFile( int index )
{
    m_wellPathIndexInFile = index;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFileWellPath::setSurveyType( QString surveyType )
{
    m_surveyType = surveyType;
    if ( m_surveyType == "PLAN" )
        setWellPathColor( cvf::Color3f( 0.999f, 0.333f, 0.0f ) );
    else if ( m_surveyType == "PROTOTYPE" )
        setWellPathColor( cvf::Color3f( 0.0f, 0.333f, 0.999f ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFileWellPath::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    RimWellPath::defineUiOrdering( uiConfigName, uiOrdering );

    uiOrdering.add( &m_useAutoGeneratedPointAtSeaLevel );

    caf::PdmUiGroup* fileInfoGroup = uiOrdering.createGroupBeforeGroup( "Simulation Well", "File" );

    if ( isStoredInCache() )
    {
        fileInfoGroup->add( &m_filePathInCache );
    }
    else
    {
        fileInfoGroup->add( &m_filePath );
    }

    fileInfoGroup->add( &m_wellPathIndexInFile );

    if ( !id().isEmpty() ) uiOrdering.insertBeforeItem( m_datumElevation.uiCapability(), &id );
    if ( !sourceSystem().isEmpty() ) uiOrdering.insertBeforeItem( m_datumElevation.uiCapability(), &sourceSystem );
    if ( !utmZone().isEmpty() ) uiOrdering.insertBeforeItem( m_datumElevation.uiCapability(), &utmZone );
    if ( !updateDate().isEmpty() ) uiOrdering.insertBeforeItem( m_datumElevation.uiCapability(), &updateDate );
    if ( !updateUser().isEmpty() ) uiOrdering.insertBeforeItem( m_datumElevation.uiCapability(), &updateUser );
    if ( !m_surveyType().isEmpty() ) uiOrdering.insertBeforeItem( m_datumElevation.uiCapability(), &m_surveyType );
}

//--------------------------------------------------------------------------------------------------
/// Read JSON or ascii file containing well path data
//--------------------------------------------------------------------------------------------------
bool RimFileWellPath::readWellPathFile( QString* errorMessage, RifWellPathImporter* wellPathImporter, bool setWellNameForExport )
{
    if ( caf::Utils::fileExists( this->filePath() ) )
    {
        RifWellPathImporter::WellData wellData =
            wellPathImporter->readWellData( this->filePath(), m_wellPathIndexInFile() );

        RifWellPathImporter::WellMetaData wellMetaData =
            wellPathImporter->readWellMetaData( this->filePath(), m_wellPathIndexInFile() );
        // General well info

        if ( setWellNameForExport )
        {
            setName( wellData.m_name );
        }
        else
        {
            setNameNoUpdateOfExportName( wellData.m_name );
        }

        id           = wellMetaData.m_id;
        sourceSystem = wellMetaData.m_sourceSystem;
        utmZone      = wellMetaData.m_utmZone;
        updateUser   = wellMetaData.m_updateUser;
        setSurveyType( wellMetaData.m_surveyType );
        updateDate = wellMetaData.m_updateDate.toString( "d MMMM yyyy" );

        if ( m_useAutoGeneratedPointAtSeaLevel )
        {
            ensureWellPathStartAtSeaLevel( wellData.m_wellPathGeometry.p() );
        }

        setWellPathGeometry( wellData.m_wellPathGeometry.p() );

        // Now that the data is read, we know if this is an SSIHUB wellpath that needs to be stored in the
        // cache folder along with the project file. If it is, move the pathfile reference to the m_filePathInCache
        // in order to avoid it being handled as an externalFilePath by the RimProject class

        if ( isStoredInCache() && !m_filePath().path().isEmpty() )
        {
            m_filePathInCache = m_filePath().path();
            m_filePath        = QString( "" );
        }

        return true;
    }
    else
    {
        if ( errorMessage ) ( *errorMessage ) = "Could not find the well path file: " + this->filePath();
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimFileWellPath::getCacheDirectoryPath()
{
    QString cacheDirPath = RimTools::getCacheRootDirectoryPathFromProject();
    cacheDirPath += "_wellpaths";
    return cacheDirPath;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimFileWellPath::getCacheFileName()
{
    if ( m_filePathInCache().isEmpty() )
    {
        return "";
    }

    QString cacheFileName;

    // Make the path correct related to the possibly new project filename

    QString   newCacheDirPath = getCacheDirectoryPath();
    QFileInfo oldCacheFile( m_filePathInCache() );

    cacheFileName = newCacheDirPath + "/" + oldCacheFile.fileName();

    return cacheFileName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFileWellPath::setupBeforeSave()
{
    // Copy the possibly "cached" SSIHUB wellpath, stored in the folder along the project file
    // SSIHUB is the only source for populating Id, use text in this field to decide if the cache file must be copied to
    // new project cache location
    if ( !isStoredInCache() )
    {
        return;
    }

    if ( m_filePathInCache().isEmpty() )
    {
        return;
    }

    QDir::root().mkpath( getCacheDirectoryPath() );

    QString newCacheFileName = getCacheFileName();

    // Use QFileInfo to get same string representation to avoid issues with mix of forward and backward slashes
    QFileInfo prevFileInfo( m_filePathInCache() );
    QFileInfo currentFileInfo( newCacheFileName );

    if ( prevFileInfo.absoluteFilePath().compare( currentFileInfo.absoluteFilePath() ) != 0 )
    {
        QFile::copy( m_filePathInCache(), newCacheFileName );

        m_filePathInCache = newCacheFileName;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimFileWellPath::isStoredInCache() const
{
    // SSIHUB is the only source for populating Id, use text in this field to decide if the cache file must be copied to
    // new project cache location
    return !id().isEmpty();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFileWellPath::updateFilePathsFromProjectPath( const QString& newProjectPath, const QString& oldProjectPath )
{
    QString newCacheFileName = getCacheFileName();

    if ( caf::Utils::fileExists( newCacheFileName ) )
    {
        m_filePathInCache = newCacheFileName;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFileWellPath::ensureWellPathStartAtSeaLevel( RigWellPath* wellPath )
{
    std::vector<cvf::Vec3d> wellPathPoints = wellPath->wellPathPoints();
    std::vector<double>     measuredDepths = wellPath->measuredDepths();
    if ( wellPathPoints.empty() || measuredDepths.empty() || wellPathPoints.size() != measuredDepths.size() ) return;

    cvf::Vec3d   firstPoint           = wellPathPoints[0];
    const double epsilon              = 1e-3;
    bool         firstPointAtSeaLevel = std::abs( firstPoint.z() ) < epsilon;
    if ( !firstPointAtSeaLevel )
    {
        // Insert a new point on sea level straight above the
        // first point in the read well path.
        std::vector<cvf::Vec3d> newPoints = { cvf::Vec3d( firstPoint.x(), firstPoint.y(), 0.0 ) };
        newPoints.insert( newPoints.end(), wellPathPoints.begin(), wellPathPoints.end() );
        wellPath->setWellPathPoints( newPoints );

        // Use rkbDiff as MD for the point at sea level
        double mdAtSeaLevel = 0.0;
        if ( wellPath->datumElevation() > 0.0 )
        {
            mdAtSeaLevel = wellPath->datumElevation();
        }
        else if ( this->airGap() != std::numeric_limits<double>::infinity() && this->airGap() > 0.0 )
        {
            mdAtSeaLevel = this->airGap();
        }

        std::vector<double> newMeasuredDepths = { mdAtSeaLevel };
        newMeasuredDepths.insert( newMeasuredDepths.end(), measuredDepths.begin(), measuredDepths.end() );

        wellPath->setMeasuredDepths( newMeasuredDepths );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFileWellPath::fieldChangedByUi( const caf::PdmFieldHandle* changedField,
                                        const QVariant&            oldValue,
                                        const QVariant&            newValue )
{
    RimWellPath::fieldChangedByUi( changedField, oldValue, newValue );

    if ( changedField == &m_useAutoGeneratedPointAtSeaLevel )
    {
        RifWellPathImporter wellPathImporter;
        QString             errorMessage;
        if ( readWellPathFile( &errorMessage, &wellPathImporter, false ) )
        {
            RimProject::current()->scheduleCreateDisplayModelAndRedrawAllViews();
        }
    }
}