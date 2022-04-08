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

#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

#include <QMainWindow>
#include <tuple>

class QDir;
class QXmlItem;
class QXmlQuery;
class QTreeWidgetItem;
namespace Ui
{
    class CMainWindow;
}
struct SFileInfo;
class CMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    CMainWindow( QWidget * parent = nullptr );
    virtual ~CMainWindow() override;

    bool setQRCFile( const QString & fileName );

    virtual void closeEvent( QCloseEvent * event ) override;

    void setBaseWindowTitle( const QString & title );
    void updateWindowTitle();
public Q_SLOTS:
    void slotOpen();
    bool slotSave();
    bool slotSaveAs();
    void slotRemove();
    void slotAddFiles();
    void slotAddPrefix();

    void slotItemChanged( QTreeWidgetItem * current, QTreeWidgetItem * previous );
    void slotCompAlgoChanged( const QString & algo );
Q_SIGNALS:
private:
    void setFileName( const QString & fileName );
    void setModified( bool modified, bool force = false );
    bool set( QTreeWidgetItem * item, int column, QString newText, const QString & blankValue = QString() );
    bool set( QTreeWidgetItem * item, int column, int newValue, int blankValue = -1 );
    void loadFromItem( QTreeWidgetItem * item );
    void saveToItem( QTreeWidgetItem * item );
    void loadPrefix( const QDir & relToDir, QXmlQuery & query, int pos );
    void loadFile( const QDir & relToDir, const QString & prefix, const QString & lang, const SFileInfo & fileInfo );
    void loadFile( const QDir & relToDir, const QString & prefix, const QString & lang, const QString & path );

    // algorithm, threshold, compression level
    std::tuple< QString, QString, QString > getCompressionInfo( QTreeWidgetItem * item ) const;

    QTreeWidgetItem * addPrefix( const QString & prefix, const QString & lang );

    bool canSave();
    std::unique_ptr< Ui::CMainWindow > fImpl;

    bool fModified{ false };
    QString fFileName;
    QString fBaseWindowTitle;
    int fDefaultCompLevel{ -1 };

    std::unordered_map< QString, std::unordered_map< QString, QTreeWidgetItem * > > fPrefixMap;
};
#endif 
