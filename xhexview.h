/* Copyright (c) 2019-2024 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef XHEXVIEW_H
#define XHEXVIEW_H

#include <QTextBoundaryFinder>
#include <QPixmapCache>

#include "dialoghexedit.h"
#include "xdevicetableeditview.h"

// TODO if cursor moved -> highlight location and header
// TODO modes symbols/disasm/types
class XHexView : public XDeviceTableEditView {
    Q_OBJECT

    enum SC {
        SC_DATAINSPECTOR = 0,
        SC_DATACONVERTOR,
        SC_MULTISEARCH,
        SC_GOTO_OFFSET,
        SC_GOTO_ADDRESS,
        SC_DUMPTOFILE,
        SC_SELECTALL,
        SC_COPYASDATA,
        SC_COPYOFFSET,
        SC_COPYADDRESS,
        SC_FINDSTRING,
        SC_FINDSIGNATURE,
        SC_FINDVALUE,
        SC_FINDNEXT,
        SC_SIGNATURE,
        SC_DISASM,
        SC_MEMORYMAP,
        SC_MAINHEX,
        SC_EDIT_HEX,
        SC_EDIT_REMOVE,
        SC_EDIT_RESIZE,
        __SC_SIZE
    };

public:
    // TODO setOptions ???
    struct OPTIONS {
        XADDR nStartAddress;
        qint64 nStartSelectionOffset;  // -1 no selection
        qint64 nSizeOfSelection;
        bool bMenu_Disasm;
        bool bMenu_MemoryMap;
        bool bMenu_MainHex;
        QString sTitle;  // For dialogs
        LOCMODE addressMode;
        XBinary::_MEMORY_MAP memoryMapRegion;
    };

    explicit XHexView(QWidget *pParent = nullptr);

    void _adjustView();
    virtual void adjustView();
    void setData(QIODevice *pDevice, const OPTIONS &options, bool bReload);
    void goToAddress(XADDR nAddress);
    void goToOffset(qint64 nOffset);
    XADDR getStartAddress();  // TODO Check mb remove
    XADDR getSelectionInitAddress();

private:
    enum COLUMN {
        COLUMN_LOCATION = 0,
        COLUMN_ELEMENTS,
        COLUMN_SYMBOLS
    };

    struct LOCATIONRECORD {  // TODO move
        QString sLocation;
        quint64 nLocation;
    };

    struct SHOWRECORD {
        qint32 nRow;
        qint32 nViewPos;
        qint32 nRowViewOffset;
        qint32 nSize;
        bool bFirstRowSymbol;
        bool bLastRowSymbol;
        QString sElement;
        QString sSymbol;
        bool bIsBold;
        bool bIsHighlighted;
        bool bIsSymbolError;
        QColor colBackground;
        QColor colBackgroundSelected;
        //        bool bIsSelected;
    };

    enum MODE {
        MODE_HEX = 0,
        MODE_BYTE,
        MODE_WORD,
        MODE_DWORD,
        MODE_QWORD,
        MODE_UINT8,
        MODE_INT8,
        MODE_UINT16,
        MODE_INT16,
        MODE_UINT32,
        MODE_INT32,
        MODE_UINT64,
        MODE_INT64,
    };

    SHOWRECORD _getShowRecordByOffset(qint64 nOffset);

protected:
    virtual OS cursorPositionToOS(const CURSOR_POSITION &cursorPosition);
    virtual void updateData();
    virtual void paintMap(QPainter *pPainter, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight);
    virtual void paintCell(QPainter *pPainter, qint32 nRow, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight);
    virtual void paintColumn(QPainter *pPainter, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight);
    virtual void paintTitle(QPainter *pPainter, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight, const QString &sTitle);
    virtual void contextMenu(const QPoint &pos);
    virtual void wheelEvent(QWheelEvent *pEvent);
    virtual void keyPressEvent(QKeyEvent *pEvent);
    virtual qint64 getCurrentViewPosFromScroll();
    virtual void setCurrentViewPosToScroll(qint64 nOffset);
    virtual void adjustColumns();
    virtual void registerShortcuts(bool bState);
    virtual void adjustHeader();
    virtual void _headerClicked(qint32 nColumn);
    virtual void _cellDoubleClicked(qint32 nRow, qint32 nColumn);
    virtual void adjustScrollCount();
    virtual void adjustMap();
    //    SMODE getSmode();
    //    void setSmode(SMODE smode);

private slots:
    void _disasmSlot();
    void _memoryMapSlot();
    void _mainHexSlot();
    void _setCodePage(const QString &sCodePage);
    void changeWidth();
    void changeModeView();
    void _setMode(MODE mode);

signals:
    void showOffsetDisasm(qint64 nOffset);
    void showOffsetMemoryMap(qint64 nOffset);
    void showOffsetMainHex(qint64 nOffset, qint64 nSize);

private:
    OPTIONS g_hexOptions;
    qint32 g_nBytesProLine;
    qint32 g_nPrintsProElement;
    qint32 g_nElementByteSize;
    qint32 g_nSymbolByteSize;
    MODE g_mode;
    qint32 g_nDataBlockSize;
    QList<HIGHLIGHTREGION> g_listHighlightsRegion;
    qint32 g_nViewStartDelta;
    QByteArray g_baDataBuffer;
    QList<LOCATIONRECORD> g_listLocationRecords;
    QList<SHOWRECORD> g_listShowRecords;
    QShortcut *g_shortCuts[__SC_SIZE];
    qint32 g_nAddressWidth;
    qint64 g_nThisBase;
    //    SMODE g_smode;
    bool g_bIsLocationColon;

    QString g_sCodePage;
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
    QTextCodec *g_pCodec;
    QMenu *g_pCodePageMenu;
    XOptions g_xCodePageOptions;
#endif
    QPixmapCache g_pixmapCache;
};

#endif  // XHEXVIEW_H
