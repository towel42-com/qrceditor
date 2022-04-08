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
#include "MainWindow/MainWindow.h"
#include "Version.h"

#include <QApplication>
#include "SABUtils/SABUtilsResources.h"

int main( int argc, char ** argv )
{
    Q_INIT_RESOURCE( application );
    NSABUtils::initResources();

    QApplication::setAttribute( Qt::AA_EnableHighDpiScaling ); 
    QApplication::setAttribute( Qt::AA_UseHighDpiPixmaps );
    QApplication appl( argc, argv );
    appl.setApplicationName( QString::fromStdString( NVersion::APP_NAME ) );
    appl.setApplicationVersion(QString::fromStdString(NVersion::getVersionString( true ) ) );
    appl.setOrganizationName(QString::fromStdString(NVersion::VENDOR ) );
    appl.setOrganizationDomain(QString::fromStdString(NVersion::HOMEPAGE ));


    QString qrcFile;
    for ( int ii = 1; ii < argc; ++ii )
    {
        QString name = argv[ii];
        if ( name.toLower().endsWith( ".qrc" ) )
        {
            qrcFile = name;
            break;
        }
    }
    CMainWindow mainWindow;
    mainWindow.setBaseWindowTitle( QString( "%1 v%2 - http://%3" ).arg( QString::fromStdString( NVersion::APP_NAME ) ).arg( QString::fromStdString( NVersion::getVersionString( true ) ) ).arg( QString::fromStdString( NVersion::HOMEPAGE ) ) );
    mainWindow.show();
    if ( !qrcFile.isEmpty() && !mainWindow.setQRCFile( qrcFile ) )
        return -1;
    return appl.exec();
}
