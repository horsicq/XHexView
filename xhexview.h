/* Copyright (c) 2019-2022 hors<horsicq@gmail.com>
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

#include "dialoghexedit.h"
#include "xdevicetableeditview.h"

class XHexView : public XDeviceTableEditView {
    Q_OBJECT

    enum SC {
        SC_GOTO_OFFSET = 0,
        SC_GOTO_ADDRESS,
        SC_DUMPTOFILE,
        SC_SELECTALL,
        SC_COPYHEX,
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
        __SC_SIZE
    };

    //    enum SMODE
    //    {
    //        SMODE_ANSI=0,
    //        SMODE_CODEPAGE
    //    };

public:
    // TODO setOptions
    // TODO follow functions
    struct OPTIONS {
        XADDR nStartAddress;
        qint64 nStartSelectionOffset;
        qint64 nSizeOfSelection;
        bool bMenu_Disasm;
        bool bMenu_MemoryMap;
        bool bMenu_MainHex;
        QString sTitle;  // For dialogs
        bool bIsOffsetTitle;
    };

    explicit XHexView(QWidget *pParent = nullptr);

    void _adjustView();
    void adjustView();

    void setData(QIODevice *pDevice, OPTIONS options, bool bReload = true);
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

    struct RECORD {
        QString sAddress;
        XADDR nAddress;
    };

    //    QChar filterSymbol(QChar cChar,SMODE smode);

protected:
    virtual OS cursorPositionToOS(CURSOR_POSITION cursorPosition);
    virtual void updateData();
    virtual void paintCell(QPainter *pPainter, qint32 nRow, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight);
    virtual void paintTitle(QPainter *pPainter, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight, QString sTitle);
    virtual void contextMenu(const QPoint &pos);
    virtual void wheelEvent(QWheelEvent *pEvent);
    virtual void keyPressEvent(QKeyEvent *pEvent);
    virtual qint64 getScrollValue();
    virtual void setScrollValue(qint64 nOffset);
    virtual void adjustColumns();
    virtual void registerShortcuts(bool bState);
    virtual void _headerClicked(qint32 nColumn);
    virtual void _cellDoubleClicked(qint32 nRow, qint32 nColumn);
    //    SMODE getSmode();
    //    void setSmode(SMODE smode);

private:
    QString getStringBuffer(QByteArray *pbaData);

private slots:
    void _disasmSlot();
    void _memoryMapSlot();
    void _mainHexSlot();
    void _setCodePage(QString sCodePage);

signals:
    void showOffsetDisasm(qint64 nOffset);
    void showOffsetMemoryMap(qint64 nOffset);
    void showOffsetMainHex(qint64 nOffset, qint64 nSize);

private:
    OPTIONS g_options;
    qint32 g_nBytesProLine;
    qint32 g_nDataBlockSize;
    qint32 g_nViewStartDelta;
    QByteArray g_baDataBuffer;
    QByteArray g_baDataHexBuffer;
    QString g_sStringBuffer;
    QList<RECORD> g_listRecords;
    QShortcut *shortCuts[__SC_SIZE];
    qint32 g_nAddressWidth;
    qint64 g_nThisBase;
    //    SMODE g_smode;
    bool g_bIsAddressColon;
    //    qint32 g_nPieceSize;
    QString g_sCodePage;
    QMenu *g_pCodePageMenu;
    XOptions g_xOptions;
};

#endif  // XHEXVIEW_H
