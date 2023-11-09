/* Copyright (c) 2019-2023 hors<horsicq@gmail.com>
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

class XHexView : public XDeviceTableEditView {
    Q_OBJECT

    enum SC {
        SC_DATAINSPECTOR = 0,
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
        SC_EDITHEX,
        SC_EDITREMOVE,
        SC_EDITRESIZE,
        __SC_SIZE
    };

public:
    // TODO setOptions ???
    struct OPTIONS {
        XADDR nStartAddress;
        qint64 nStartSelectionOffset;
        qint64 nSizeOfSelection;
        bool bMenu_Disasm;
        bool bMenu_MemoryMap;
        bool bMenu_MainHex;
        QString sTitle;  // For dialogs
        LOCMODE addressMode;
    };

    explicit XHexView(QWidget *pParent = nullptr);

    void _adjustView();
    void adjustView();
    void setData(QIODevice *pDevice, const OPTIONS &options, bool bReload);
    void goToAddress(XADDR nAddress);
    void goToOffset(qint64 nOffset);
    XADDR getStartAddress();
    XADDR getSelectionInitAddress();

private:
    enum COLUMN {
        COLUMN_ADDRESS = 0,
        COLUMN_HEX,
        COLUMN_SYMBOLS
    };

    struct LOCATIONRECORD {
        QString sLocation;
        quint64 nLocation;
    };

    struct BYTERECORD {
        QString sHex;
        QString sChar;
        bool bIsBold;
        bool bIsHighlighted;
        QColor colBackground;
        QColor colBackgroundSelected;
        //        bool bIsSelected;
    };

    QString getStringBuffer(QByteArray *pbaData);  // TODO QList

protected:
    virtual OS cursorPositionToOS(CURSOR_POSITION cursorPosition);
    virtual void updateData();
    virtual void paintCell(QPainter *pPainter, qint32 nRow, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight);
    virtual void paintColumn(QPainter *pPainter, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight);
    virtual void paintTitle(QPainter *pPainter, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight, const QString &sTitle);
    virtual void contextMenu(const QPoint &pos);
    virtual void wheelEvent(QWheelEvent *pEvent);
    virtual void keyPressEvent(QKeyEvent *pEvent);
    virtual qint64 getCurrentViewOffsetFromScroll();
    virtual void setCurrentViewOffsetToScroll(qint64 nOffset);
    virtual void adjustColumns();
    virtual void registerShortcuts(bool bState);
    virtual void adjustHeader();
    virtual void _headerClicked(qint32 nColumn);
    virtual void _cellDoubleClicked(qint32 nRow, qint32 nColumn);
    virtual void adjustScrollCount();
    //    SMODE getSmode();
    //    void setSmode(SMODE smode);

private slots:
    void _disasmSlot();
    void _memoryMapSlot();
    void _mainHexSlot();
    void _setCodePage(const QString &sCodePage);
    void changeWidth();

signals:
    void showOffsetDisasm(qint64 nOffset);
    void showOffsetMemoryMap(qint64 nOffset);
    void showOffsetMainHex(qint64 nOffset, qint64 nSize);

private:
    OPTIONS g_hexOptions;
    qint32 g_nBytesProLine;
    qint32 g_nDataBlockSize;
    QList<HIGHLIGHTREGION> g_listHighlightsRegion;
    qint32 g_nViewStartDelta;
    QByteArray g_baDataBuffer;
    QByteArray g_baDataHexBuffer;
    QString g_sStringBuffer;
    QList<LOCATIONRECORD> g_listLocationRecords;
    QList<BYTERECORD> g_listByteRecords;
    QShortcut *g_shortCuts[__SC_SIZE];
    qint32 g_nAddressWidth;
    qint64 g_nThisBase;
    //    SMODE g_smode;
    bool g_bIsAddressColon;
    QString g_sCodePage;
    QTextCodec *g_pCodec;
    QMenu *g_pCodePageMenu;
    XOptions g_xCodePageOptions;
    QPixmapCache g_pixmapCache;
};

#endif  // XHEXVIEW_H
