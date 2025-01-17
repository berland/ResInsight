/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2011-2012 Statoil ASA, Ceetron AS
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

#include "cafPdmChildArrayField.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"

#include <QItemSelection>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QUrl>
#include <QWizard>

class QFile;
class QProgressDialog;
class QLabel;
class QTextEdit;

class RimWellPathImport;
class RimOilFieldEntry;
class RimWellPathEntry;

namespace caf
{
class PdmUiTreeView;
class PdmUiListView;
class PdmUiPropertyView;
class PdmObjectCollection;
} // namespace caf

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class AuthenticationPage : public QWizardPage
{
    Q_OBJECT

public:
    AuthenticationPage( const QString& webServiceAddress, QWidget* parent = nullptr );

    void initializePage() override;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class FieldSelectionPage : public QWizardPage
{
    Q_OBJECT

public:
    FieldSelectionPage( RimWellPathImport* wellPathImport, QWidget* parent = nullptr );
    ~FieldSelectionPage() override;

    void initializePage() override;

private:
    caf::PdmUiPropertyView* m_propertyView;
};

//--------------------------------------------------------------------------------------------------
/// Container class used to define column headers
//--------------------------------------------------------------------------------------------------
class ObjectGroupWithHeaders : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    ObjectGroupWithHeaders()
    {
        CAF_PDM_InitFieldNoDefault( &objects, "PdmObjects", "", "", "", "" );

        CAF_PDM_InitField( &m_isChecked, "IsChecked", true, "Active", "", "", "" );
        m_isChecked.uiCapability()->setUiHidden( true );
    };

    void defineObjectEditorAttribute( QString uiConfigName, caf::PdmUiEditorAttribute* attribute ) override;

public:
    caf::PdmChildArrayField<PdmObjectHandle*> objects;

protected:
    caf::PdmFieldHandle* objectToggleField() override { return &m_isChecked; }

protected:
    caf::PdmField<bool> m_isChecked;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class DownloadEntity
{
public:
    QString name;
    QString requestUrl;
    QString responseFilename;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class SummaryPageDownloadEntity : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    SummaryPageDownloadEntity();

    caf::PdmField<QString> name;
    caf::PdmField<QString> requestUrl;
    caf::PdmField<QString> responseFilename;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class WellSelectionPage : public QWizardPage
{
    Q_OBJECT

public:
    WellSelectionPage( RimWellPathImport* wellPathImport, QWidget* parent = nullptr );
    ~WellSelectionPage() override;

    void initializePage() override;
    void buildWellTreeView();

    void selectedWellPathEntries( std::vector<DownloadEntity>& downloadEntities, caf::PdmObjectHandle* objHandle );

private:
    void sortObjectsByDescription( caf::PdmObjectCollection* objects );

private slots:
    void customMenuRequested( const QPoint& pos );

private:
    ObjectGroupWithHeaders* m_regionsWithVisibleWells;
    RimWellPathImport*      m_wellPathImportObject;
    caf::PdmUiTreeView*     m_wellSelectionTreeView;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class WellSummaryPage : public QWizardPage
{
    Q_OBJECT

public:
    WellSummaryPage( RimWellPathImport* wellPathImport, QWidget* parent = nullptr );

    void initializePage() override;

    void updateSummaryPage();

private slots:
    void slotShowDetails();

private:
    RimWellPathImport*        m_wellPathImportObject;
    QTextEdit*                m_textEdit;
    caf::PdmUiListView*       m_listView;
    caf::PdmObjectCollection* m_objectGroup;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class RiuWellImportWizard : public QWizard
{
    Q_OBJECT

public:
    enum DownloadState
    {
        DOWNLOAD_FIELDS,
        DOWNLOAD_WELLS,
        DOWNLOAD_WELL_PATH,
        DOWNLOAD_UNDEFINED
    };

public:
    RiuWellImportWizard( const QString&     webServiceAddress,
                         const QString&     downloadFolder,
                         RimWellPathImport* wellPathImportObject,
                         QWidget*           parent = nullptr );
    ~RiuWellImportWizard() override;

    void        setCredentials( const QString& username, const QString& password );
    QStringList absoluteFilePathsToWellPaths() const;

    // Methods used from the wizard pages
    void resetAuthenticationCount();

public slots:
    void downloadWellPaths();
    void downloadWells();
    void downloadFields();

    void checkDownloadQueueAndIssueRequests();

    void issueHttpRequestToFile( QString completeUrlText, QString destinationFileName );
    void cancelDownload();

    void httpFinished();
    void httpReadyRead();

    void slotAuthenticationRequired( QNetworkReply* networkReply, QAuthenticator* authenticator );

    int wellSelectionPageId();

#if !defined(QT_NO_OPENSSL) && !defined(CVF_OSX)
    void sslErrors( QNetworkReply*, const QList<QSslError>& errors );
#endif

private slots:
    void slotCurrentIdChanged( int currentId );

private:
    void startRequest( QUrl url );
    void setUrl( const QString& httpAddress );

    QString jsonFieldsFilePath();
    QString jsonWellsFilePath();

    void updateFieldsModel();
    void parseWellsResponse( RimOilFieldEntry* oilFieldEntry );

    QString getValue( const QString& key, const QString& stringContent );

    QProgressDialog* progressDialog();
    void             hideProgressDialog();

private:
    QString m_webServiceAddress;
    QString m_destinationFolder;

    RimWellPathImport*  m_wellPathImportObject;
    caf::PdmUiTreeView* m_pdmTreeView;

    QProgressDialog* m_myProgressDialog;

    QUrl                  m_url;
    QNetworkAccessManager m_networkAccessManager;
    QNetworkReply*        m_reply;
    QFile*                m_file;
    bool                  m_httpRequestAborted;

    bool m_firstTimeRequestingAuthentication;

    QList<DownloadEntity> m_wellRequestQueue;

    DownloadState m_currentDownloadState;

    int m_fieldSelectionPageId;
    int m_wellSelectionPageId;
    int m_wellSummaryPageId;
};
