// The MIT License( MIT )
//
// Copyright( c ) 2020-2021 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "MainWindow.h"
#include "../Version.h"

#include "ui_MainWindow.h"
#include "SABUtils/QtUtils.h"
#include "SABUtils/FileUtils.h"
#include "SABUtils/ButtonEnabler.h"

#include <QKeySequence>
#include <QFileDialog>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QXmlQuery>
#include <QXmlResultItems>
#include <QXmlStreamWriter>
#include <QCloseEvent>
#include <QFile>
#include <QDebug>
#include <QFileIconProvider>

#include <set>


CMainWindow::CMainWindow( QWidget * parent )
    : QMainWindow( parent ),
    fImpl( new Ui::CMainWindow )
{
    fImpl->setupUi( this );
    //setWindowIcon( style()->standardIcon( QStyle::SP_DialogSaveButton )
    fImpl->actionSave->setIcon( style()->standardIcon( QStyle::SP_DialogSaveButton ) );
    fImpl->actionOpen->setIcon( style()->standardIcon( QStyle::SP_DialogOpenButton ) );
    fImpl->actionExit->setShortcut( QKeySequence( tr( "Alt+F4" ) ) );

    new NSABUtils::CButtonEnabler( fImpl->files, fImpl->removeBtn );

    connect( fImpl->actionOpen, &QAction::triggered, this, &CMainWindow::slotOpen );
    connect( fImpl->actionSave, &QAction::triggered, this, &CMainWindow::slotSave );
    connect( fImpl->actionSaveAs, &QAction::triggered, this, &CMainWindow::slotSaveAs );
    connect( fImpl->actionAddFiles, &QAction::triggered, this, &CMainWindow::slotAddFiles );
    connect( fImpl->actionAddPrefix, &QAction::triggered, this, &CMainWindow::slotAddPrefix );
    connect( fImpl->removeBtn, &QPushButton::clicked, this, &CMainWindow::slotRemove );

    connect( fImpl->actionAbout, &QAction::triggered,
             [this]()
             {
                 QString msg = QString( R"(<center>%1<br/>Version: v%2<br/><a href="http://%3">%3</a><br/><a href="http://%4">%4</a></center>)" )
                     .arg( QString::fromStdString( NVersion::APP_NAME ) )
                     .arg( QString::fromStdString( NVersion::getVersionString( true ) ) )
                     .arg( QString::fromStdString( NVersion::HOMEPAGE ) )
                     .arg( QString::fromStdString( NVersion::PRODUCT_HOMEPAGE ) )
                     ;
                 QMessageBox::about( this, tr( "About" ), msg );
             } );
    connect( fImpl->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt );

    connect( fImpl->compression, &QComboBox::currentTextChanged, this, &CMainWindow::slotCompAlgoChanged );
    connect( fImpl->files, &QTreeWidget::currentItemChanged, this, &CMainWindow::slotItemChanged );

    auto menu = new QMenu( fImpl->addButton );
    menu->addAction( fImpl->actionAddFiles );
    menu->addAction( fImpl->actionAddPrefix );
    fImpl->addButton->setMenu( menu );

    slotItemChanged( nullptr, nullptr );
    slotCompAlgoChanged( fImpl->compression->currentText() );
}

CMainWindow::~CMainWindow()
{
}

void CMainWindow::slotCompAlgoChanged( const QString & algo )
{
    fImpl->level->setEnabled( algo != tr( "Best" ) );
    fDefaultCompLevel = 0;
    if ( algo == tr( "zstd" ) )
    {
        fImpl->level->setMinimum( 0 );
        fImpl->level->setMaximum( 19 );
        fImpl->level->setSpecialValueText( "Special" );
        fImpl->level->setValue( 14 );
        fDefaultCompLevel = 14;
    }
    else if ( algo == tr( "zlib" ) )
    {
        fImpl->level->setMinimum( 1 );
        fImpl->level->setMaximum( 9 );
        fImpl->level->setValue( 9 );
        fDefaultCompLevel = 6;
    }
}

void CMainWindow::slotItemChanged( QTreeWidgetItem * current, QTreeWidgetItem * prev )
{
    saveToItem( prev );
    loadFromItem( current );
    fImpl->actionAddFiles->setEnabled( current );
    fImpl->properties->setEnabled( current );
}

void CMainWindow::loadFromItem( QTreeWidgetItem * item )
{
    bool isFile = item && item->parent();
    bool isPrefix = item && !item->parent();

    fImpl->properties->setVisible( isFile || isPrefix );

    fImpl->alias->setVisible( isFile );
    fImpl->aliasLabel->setVisible( isFile );
    fImpl->fileName->setVisible( isFile );
    fImpl->fileNameLabel->setVisible( isFile );
    fImpl->prefix->setVisible( isPrefix );
    fImpl->prefixLabel->setVisible( isPrefix );
    fImpl->language->setVisible( isPrefix );
    fImpl->languageLabel->setVisible( isPrefix );
    fImpl->compressionEnabled->setVisible( isFile );
    fImpl->resourcePath->setVisible( isFile );
    fImpl->resourcePathLabel->setVisible( isFile );
    fImpl->resourceURL->setVisible( isFile );
    fImpl->resourceURLLabel->setVisible( isFile );

    if ( isPrefix )
    {
        fImpl->prefix->setText( item->text( 0 ) );
        fImpl->language->setText( item->text( 1 ) );
    }
    if ( isFile )
    {
        fImpl->fileName->setText( item->text( 0 ) );
        fImpl->alias->setText( item->text( 2 ) );
        auto compression = item->text( 4 );
        if ( compression.isEmpty() )
            compression = "Best";
        fImpl->compressionEnabled->setChecked( compression != tr( "none" ) );
        auto pos = fImpl->compression->findText( compression );
        if ( pos != -1 )
            fImpl->compression->setCurrentIndex( pos );

        auto level = item->text( 5 );
        if ( level.isEmpty() )
            level = "0";
        fImpl->level->setValue( level.toInt() );

        auto threshold = item->text( 6 );
        if ( threshold.isEmpty() )
            threshold = "70";
        fImpl->threshold->setValue( threshold.toInt() );

        auto fnName = item->parent()->text( 0 );
        auto currName = item->text( 2 );
        if ( currName.isEmpty() )
            currName = item->text( 0 );
        if ( currName.startsWith( "./" ) || currName.startsWith( ".\\" ) )
            currName = currName.mid( 2 );
        fnName += currName;

        fImpl->resourcePath->setText( QString( ":%1" ).arg( fnName ) );
        fImpl->resourceURL->setText( QString( "qrc://%1" ).arg( fnName ) );
    }
}

void CMainWindow::setModified( bool modified, bool force )
{
    if ( force || ( fModified != modified ) )
    {
        fModified = modified;
        updateWindowTitle();
    }
}

bool CMainWindow::set( QTreeWidgetItem * item, int column, QString newText, const QString & blankValue )
{
    auto curr = item->text( column );
    if ( newText == blankValue )
        newText.clear();

    if ( curr != newText )
    {
        item->setText( column, newText );
        return true;
    }
    return false;
}

bool CMainWindow::set( QTreeWidgetItem * item, int column, int newValue, int blankValue )
{
    auto curr = item->text( column );
    QString newText = QString::number( newValue );
    if ( newValue == blankValue )
        newText.clear();

    if ( curr != newText )
    {
        item->setText( column, newText );
        return true;
    }
    return false;
}

void CMainWindow::saveToItem( QTreeWidgetItem * prev )
{
    if ( !prev )
        return;

    bool isFile = prev && prev->parent();
    bool isPrefix = prev && !prev->parent();

    bool changed = false;
    if ( isPrefix )
    {
        changed = set( prev, 0, fImpl->prefix->text() ) || changed;
        changed = set( prev, 1, fImpl->language->text() ) || changed;
    }
    if ( isFile )
    {
        changed = set( prev, 2, fImpl->alias->text() ) || changed;

        bool compEnabled = fImpl->compressionEnabled->isChecked();
        changed = set( prev, 4, compEnabled ? fImpl->compression->currentText() : tr( "none" ), tr( "Best" ) ) || changed;
        changed = set( prev, 5, compEnabled ? fImpl->level->value() : fDefaultCompLevel, fDefaultCompLevel ) || changed;
        changed = set( prev, 6, compEnabled ? fImpl->threshold->value() : 70, 70 ) || changed;
    }
    setModified( fModified || changed );
}

void CMainWindow::slotOpen()
{
    if ( !canSave() )
        return;

    auto fn = QFileDialog::getOpenFileName( this, tr( "Choose Resource File" ), QString(), tr( "Resource Files (*.qrc)" ) );
    if ( fn.isEmpty() )
        return;
    setQRCFile( fn );
}

QString getString( QXmlQuery & query, const QString & queryString )
{
    query.setQuery( queryString );
    Q_ASSERT( query.isValid() );

    QString retVal;
    query.evaluateTo( &retVal );
    retVal = retVal.trimmed();
    return retVal;
}

int getInt( QXmlQuery & query, const QString & queryString, bool needsCount )
{
    auto stringValue = getString( query, needsCount ? QString( "count(%1)" ).arg( queryString ) : queryString );
    return stringValue.toInt();
}

QString getAliasedPath( const QDir & relToDir, const QString & alias, const QString & fn )
{
    auto retVal = alias;
    if ( retVal.isEmpty() )
        retVal = fn;
    return relToDir.absoluteFilePath( retVal );
}

QString getAliasedPath( const QDir & relToDir, QTreeWidgetItem * item )
{
    if ( !item )
        return QString();

    return getAliasedPath( relToDir, item->text( 2 ), item->text( 0 ) );
}

struct SFileInfo
{
    explicit SFileInfo( const QString & path, const QDir & relToDir )
    {
        fFileName = relToDir.relativeFilePath( path );
    }


    SFileInfo( QXmlQuery & query, const QString & fileQueryString, int fileNum )
    {
        auto fileQuery = QString( "%1[%2]" ).arg( fileQueryString ).arg( fileNum );

        fAlias = getString( query, QString( "%1/@alias/string()" ).arg( fileQuery ) );
        fThreshold = getString( query, QString( "%1/@threshold/string()" ).arg( fileQuery ) );
        fAlgo = getString( query, QString( "%1/@compress-algo/string()" ).arg( fileQuery ) );
        fLevel = getString( query, QString( "%1/@compress/string()" ).arg( fileQuery ) );
        fFileName = getString( query, QString( "%1/string()" ).arg( fileQuery ) );
    }

    QTreeWidgetItem * addFile( const QDir & relToDir, QTreeWidgetItem * parent ) const
    {
        if ( !parent )
            return nullptr;
        
        auto searchPath = getAliasedPath( relToDir, fAlias, fFileName );
        for ( int ii = 0; ii < parent->childCount(); ++ii )
        {
            auto child = parent->child( ii );
            if ( !child )
                continue;
            auto aliasPath = getAliasedPath( relToDir, child );
            if ( searchPath == aliasPath )
                return nullptr;
        }

        auto absPath = relToDir.absoluteFilePath( fFileName );
        auto fi = QFileInfo( absPath );

        QFileIconProvider fip;

        auto retVal = new QTreeWidgetItem( parent, QStringList() << QString( "./%1" ).arg( relToDir.relativeFilePath( absPath ) ) << QString() << fAlias << NSABUtils::NFileUtils::fileSizeString( fi ) << fAlgo << fLevel << fThreshold );
        retVal->setIcon( 0, fip.icon( fi ) );

        if ( !fi.exists() )
        {
            retVal->setBackground( 0, Qt::red );
            retVal->setToolTip( 0, QObject::tr( "Warning: File does not exist" ) );
        }

        return retVal;
    }

    QString fAlias;
    QString fThreshold;
    QString fAlgo;
    QString fLevel;
    QString fFileName;
};

void CMainWindow::loadPrefix( const QDir & relToDir, QXmlQuery & query, int pos )
{
    auto prefix = getString( query, QString( "/RCC/qresource[%1]/@prefix/string()" ).arg( pos ) );
    if ( prefix.isEmpty() )
        prefix = "/";

    auto lang = getString( query, QString( "/RCC/qresource[%1]/@lang/string()" ).arg( pos ) );

    auto filesQueryString = QString( "/RCC/qresource[%1]/file" ).arg( pos );

    auto cnt = getInt( query, filesQueryString, true );

    for ( int ii = 1; ii <= cnt; ++ii )
    {
        auto file = SFileInfo( query, filesQueryString, ii );
        loadFile( relToDir, prefix, lang, file );
    }
}


void CMainWindow::loadFile( const QDir & relToDir, const QString & prefix, const QString & lang, const QString & path )
{
    SFileInfo fi( path, relToDir );
    return loadFile( relToDir, prefix, lang, fi );
}

void CMainWindow::loadFile( const QDir & relToDir, const QString & prefix, const QString & lang, const SFileInfo & fileInfo )
{
    auto prefixItem = addPrefix( prefix, lang );

    fileInfo.addFile( relToDir, prefixItem );
    prefixItem->setExpanded( true );
    NSABUtils::autoSize( fImpl->files );
}

QTreeWidgetItem * CMainWindow::addPrefix( const QString & prefix, const QString & lang )
{
    auto pos = fPrefixMap.find( prefix );
    QTreeWidgetItem * prefixItem = nullptr;
    if ( pos != fPrefixMap.end() )
    {
        auto pos2 = ( *pos ).second.find( lang );
        if ( pos2 != ( *pos ).second.end() )
        {
            prefixItem = ( *pos2 ).second;
        }
    }

    if ( !prefixItem )
    {
        prefixItem = new QTreeWidgetItem( fImpl->files, QStringList() << prefix << lang );
        fPrefixMap[ prefix ][ lang ] = prefixItem;
    }
    return prefixItem;
}

bool CMainWindow::setQRCFile( const QString & fileName )
{
    QFile file( fileName );
    if ( !file.open( QFile::ReadOnly | QFile::Text ) )
    {
        QMessageBox::critical( this, tr( "Could not open Resource File" ), tr( "Could not open Resource File '%1'" ).arg( fileName ) );
        return false;
    }

    auto dir = QFileInfo( fileName ).absoluteDir();

    fPrefixMap.clear();
    fFileName.clear();
    fImpl->files->clear();

    QXmlQuery query;
    query.setFocus( &file );

    auto cnt = getInt( query, "/RCC/qresource", true );
    for ( int ii = 1; ii <= cnt; ++ii )
    {
        loadPrefix( dir, query, ii );
    }

    fFileName = fileName;
    setModified( false, true );
    return true;
}

void CMainWindow::setFileName( const QString & fileName )
{
    auto oldRelToDir = QFileInfo( fFileName ).absoluteDir();

    fFileName = fileName;
    updateWindowTitle();

    auto relToDir = QFileInfo( fFileName ).absoluteDir();

    for ( int ii = 0; ii < fImpl->files->topLevelItemCount(); ++ii )
    {
        auto prefixItem = fImpl->files->topLevelItem( ii );
        if ( !prefixItem )
            continue;
        for ( int jj = 0; jj < prefixItem->childCount(); ++jj )
        {
            auto item = prefixItem->child( jj );
            if ( !item )
                continue;

            auto absPath = oldRelToDir.absoluteFilePath( item->text( 0 ) );
            auto relPath = QString( "./%1" ).arg( relToDir.relativeFilePath( absPath ) );

            item->setText( 0, relPath );
        }
    }
}

bool CMainWindow::slotSaveAs()
{
    saveToItem( fImpl->files->currentItem() );

    auto fileName = QFileDialog::getSaveFileName( this, tr( "Save Resource File As" ), QString(), tr( "Resource Files (*.qrc)" ) );
    if ( fileName.isEmpty() )
        return false;
    setFileName( fileName );
    return slotSave();
}

bool CMainWindow::slotSave()
{
    saveToItem( fImpl->files->currentItem() );

    if ( fFileName.isEmpty() )
    {
        if ( !slotSaveAs() )
            return false;
    }

    if ( QFile( fFileName ).exists() )
    {
        auto backup = fFileName + ".bak";
        if ( QFile( backup ).exists() )
            QFile::remove( backup );
        if ( !QFile::rename( fFileName, backup ) )
            QMessageBox::warning( this, tr( "Could not backup Resource File" ), tr( "Could not backup Resource File '%1' to '%2' for writing" ).arg( fFileName ).arg( backup ) );
    }
    auto file = QFile( fFileName );
    if ( !file.open( QFile::WriteOnly | QFile::Truncate | QFile::Text ) )
    {
        QMessageBox::critical( this, tr( "Could not open Resource File" ), tr( "Could not open Resource File '%1' for writing" ).arg( fFileName ) );
        return false;
    }
    QXmlStreamWriter writer( &file );
    writer.setAutoFormatting( true );
    writer.writeStartDocument();
    writer.writeStartElement( "RCC" );

    for ( int ii = 0; ii < fImpl->files->topLevelItemCount(); ++ii )
    {
        auto prefixItem = fImpl->files->topLevelItem( ii );
        if ( !prefixItem )
            continue;

        writer.writeStartElement( "qresource" );
            if ( !prefixItem->text( 1 ).isEmpty() )
                writer.writeAttribute( "lang", prefixItem->text( 1 ) );
            auto prefix = prefixItem->text( 0 );
            if ( prefix.isEmpty() )
                prefix = "/";
            writer.writeAttribute( "prefix", prefix );

            for ( int jj = 0; jj < prefixItem->childCount(); ++jj )
            {
                auto fileItem = prefixItem->child( jj );
                if ( !fileItem )
                    continue;

                writer.writeStartElement( "file" );
                    if ( !fileItem->text( 2 ).isEmpty() )
                        writer.writeAttribute( "alias", fileItem->text( 2 ) );

                    QString algo;
                    QString threshold;
                    QString level;
                    std::tie( algo, threshold, level ) = getCompressionInfo( fileItem );

                    if ( !algo.isEmpty() )
                    {
                        writer.writeAttribute( "compress-algo", algo );
                        if ( !level.isEmpty() )
                            writer.writeAttribute( "compress", threshold );
                    }
                    if ( !threshold.isEmpty() )
                        writer.writeAttribute( "threshold", threshold );
                    auto fn = fileItem->text( 0 );
                    if ( fn.startsWith( "./" ) || fn.startsWith( ".\\" ) )
                        fn = fn.mid( 2 );

                    writer.writeCharacters( fn );
                writer.writeEndElement();
            }
        writer.writeEndElement();
    }

    writer.writeEndElement();
    writer.writeEndDocument();

    setModified( false );
    return true;
}

std::tuple< QString, QString, QString > CMainWindow::getCompressionInfo( QTreeWidgetItem * item ) const
{
    if ( !item )
        return {};

    auto algo = item->text( 4 ).toLower();
    if ( algo == tr( "none" ) )
        return std::make_tuple( "none", QString(), QString() );

    auto threshold = item->text( 6 );
    if ( threshold == "70" )
        threshold.clear();

    auto level = item->text( 5 );
    if ( algo == "best" )
        level.clear();
    else if ( algo == "zstd" )
    {
        if ( level == "14" )
            level.clear();
    }
    else if ( algo == "zlib" )
    {
        if ( level == "6" )
            level.clear();
    }
    return std::make_tuple( algo, threshold, level );
}


void CMainWindow::slotRemove()
{
    auto curr = fImpl->files->currentItem();
    if ( !curr )
        return;
    delete curr;
    setModified( true );
}

void CMainWindow::slotAddFiles()
{
    QDir relToDir;
    if ( !fFileName.isEmpty() )
        relToDir = QFileInfo( fFileName ).absoluteDir();
    auto prefix = fImpl->files->currentItem();
    if ( prefix && prefix->parent() )
        prefix = prefix->parent();
    if ( !prefix )
        return;

    auto fileNames = QFileDialog::getOpenFileNames( this, tr( "Select Files" ), QString(), tr( "All Files (*)" ) );

    for ( auto && ii : fileNames )
    {
        loadFile( relToDir, prefix->text( 0 ), prefix->text( 1 ), ii );
    }
    setModified( true );
}

void CMainWindow::slotAddPrefix()
{
    auto basePrefix = QString( "/new/prefix%1" );
    QString prefix;
    int curr = 1;
    do 
    {
        prefix = basePrefix.arg( curr );
        curr++;
    } while ( fPrefixMap.find( prefix ) != fPrefixMap.end() );

    auto item = addPrefix( prefix , QString() );
    fImpl->files->setCurrentItem( item );
    setModified( true );
}

void CMainWindow::closeEvent( QCloseEvent * event )
{
    if ( !canSave() )
        event->ignore();
}

void CMainWindow::setBaseWindowTitle( const QString & title )
{
    fBaseWindowTitle = title;
    updateWindowTitle();
}

void CMainWindow::updateWindowTitle()
{
    QString title = fBaseWindowTitle;

    auto fileName = fFileName;
    if ( fileName.isEmpty() )
        fileName = "<UNNAMED>";

    if ( !title.isEmpty() && !fileName.isEmpty() )
        title += " - ";
    title += fileName;
    if ( fModified )
        title += "*";
    setWindowTitle( title );
}

bool CMainWindow::canSave()
{
    saveToItem( fImpl->files->currentItem() );
    if ( !fModified )
        return true;

    auto aOK = QMessageBox::critical( this, tr( "Resource File Modified" ), tr( "Your qrc file has been modified.\nDo you want to save the changes?" ), QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No, QMessageBox::StandardButton::Cancel );
    if ( aOK == QMessageBox::StandardButton::Yes )
    {
        if ( !slotSave() )
            return false;
    }
    if ( aOK == QMessageBox::StandardButton::Cancel )
        return false;
    return true;
}