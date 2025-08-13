/* Copyright (c) 2019-2025 hors<horsicq@gmail.com>
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
// #include "xformatwidget.h"
#include "xdevicetableeditview.h"
// #include "dialogsetgenericwidget.h"

// TODO if cursor moved -> highlight location and header
// TODO modes symbols/disasm/types
class XHexView : public XDeviceTableEditView {
    Q_OBJECT

public:
    // TODO setOptions ???
    struct OPTIONS {
        qint64 nStartOffset;
        qint64 nTotalSize;
        qint64 nStartSelectionOffset;  // -1 no selection
        qint64 nSizeOfSelection;
        bool bMenu_Disasm;
        bool bMenu_MemoryMap;
        bool bMenu_MainHex;
        QString sTitle;  // For dialogs
        LOCMODE addressMode;
    };

    explicit XHexView(QWidget *pParent = nullptr);

    void _adjustView();
    virtual void adjustView();
    void setData(QIODevice *pDevice, const OPTIONS &options, bool bReload);
    void goToOffset(qint64 nOffset);
    // XADDR getStartLocation();  // TODO Check mb remove
    // XADDR getSelectionInitLocation();
    void setBytesProLine(qint32 nBytesProLine);
    virtual QList<XShortcuts::MENUITEM> getMenuItems();

private:
    enum COLUMN {
        COLUMN_LOCATION = 0,
        COLUMN_ELEMENTS,
        COLUMN_SYMBOLS
    };

    struct LOCATIONRECORD {  // TODO move
        QString sLocation;
        quint64 nViewPos;
    };

    struct SHOWRECORD {
        qint32 nRow;
        qint32 nViewPos;
        qint32 nRowViewPos;
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

    enum ELEMENT_MODE {
        ELEMENT_MODE_HEX = 0,
        ELEMENT_MODE_BYTE,
        ELEMENT_MODE_WORD,
        ELEMENT_MODE_DWORD,
        ELEMENT_MODE_QWORD,
        ELEMENT_MODE_UINT8,
        ELEMENT_MODE_INT8,
        ELEMENT_MODE_UINT16,
        ELEMENT_MODE_INT16,
        ELEMENT_MODE_UINT32,
        ELEMENT_MODE_INT32,
        ELEMENT_MODE_UINT64,
        ELEMENT_MODE_INT64,
    };

    SHOWRECORD _getShowRecordByViewPos(qint64 nOffset);

protected:
    virtual OS cursorPositionToOS(const CURSOR_POSITION &cursorPosition);
    virtual void updateData();
    virtual void paintMap(QPainter *pPainter, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight);
    virtual void paintCell(QPainter *pPainter, qint32 nRow, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight);
    virtual void paintColumn(QPainter *pPainter, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight);
    virtual void paintTitle(QPainter *pPainter, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight, const QString &sTitle);
    virtual void wheelEvent(QWheelEvent *pEvent);
    virtual void keyPressEvent(QKeyEvent *pEvent);
    virtual qint64 getCurrentViewPosFromScroll();
    virtual void setCurrentViewPosToScroll(qint64 nOffset);
    virtual void adjustColumns();
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
    void changeElementWidth();
    void changeElementMode();
    void _setMode(ELEMENT_MODE mode);

private:
    OPTIONS g_hexOptions;
    qint32 g_nBytesProLine;
    qint32 g_nPrintsProElement;
    qint32 g_nElementByteSize;
    qint32 g_nSymbolByteSize;
    ELEMENT_MODE g_mode;
    qint32 g_nDataBlockSize;
    QList<HIGHLIGHTREGION> g_listHighlightsRegion;
    qint32 g_nViewStartDelta;
    QByteArray g_baDataBuffer;
    QList<LOCATIONRECORD> g_listLocationRecords;
    QList<SHOWRECORD> g_listShowRecords;
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
