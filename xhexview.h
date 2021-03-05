// copyright (c) 2020-2021 hors<horsicq@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#ifndef XHEXVIEW_H
#define XHEXVIEW_H

#include "xabstracttableview.h"
#include "dialoggotoaddress.h"
#include "dialogsearch.h"
#include "dialogdumpprocess.h"
#include "dialogsearchprocess.h"
#include "dialoghexsignature.h"

class XHexView : public XAbstractTableView
{
    Q_OBJECT
public:
    struct OPTIONS
    {
        qint64 nStartAddress;
        qint64 nStartSelectionOffset;
        qint64 nSizeOfSelection;
        bool bMenu_Disasm; // TODO remove TODO list of custom items
        bool bMenu_MemoryMap; // TODO remove TODO list of custom items
        QString sSignaturesPath;
        // TODO BackupFileName
        // TODO save backup
    };

    explicit XHexView(QWidget *pParent=nullptr);
    void setData(QIODevice *pDevice,OPTIONS options);
    void goToAddress(qint64 nAddress);
    void goToOffset(qint64 nOffset);
    qint64 getStartAddress();

    void setReadonly(bool bState);
    void enableReadOnly(bool bState);
    void setEdited(bool bState);

private:
    enum COLUMN
    {
        COLUMN_ADDRESS=0,
        COLUMN_HEX,
        COLUMN_SYMBOLS
    };

    QChar filterSymbol(QChar cChar);

protected:
    virtual bool isOffsetValid(qint64 nOffset);
    virtual bool isEnd(qint64 nOffset);
    virtual OS cursorPositionToOS(CURSOR_POSITION cursorPosition);
    virtual void updateData();
    virtual void paintCell(QPainter *pPainter,qint32 nRow,qint32 nColumn,qint32 nLeft,qint32 nTop,qint32 nWidth,qint32 nHeight);
    virtual void contextMenu(const QPoint &pos);
    virtual void wheelEvent(QWheelEvent *pEvent);
    virtual void keyPressEvent(QKeyEvent *pEvent);
    virtual qint64 getScrollValue();
    virtual void setScrollValue(qint64 nOffset);
    virtual void adjustColumns();
    virtual void registerShortcuts(bool bState);

private slots:
    void _goToOffsetSlot();
    void _goToAddressSlot();
    void _dumpToFileSlot();
    void _signatureSlot();
    void _findSlot();
    void _findNextSlot();
    void _selectAllSlot();
    void _copyAsHexSlot();
    void _copyCursorOffsetSlot();
    void _copyCursorAddressSlot();
    void _disasmSlot();
    void _memoryMapSlot();

signals:
    void showOffsetDisasm(qint64 nOffset);
    void showOffsetMemoryMap(qint64 nOffset);

private:
    QIODevice *g_pDevice;
    OPTIONS g_options;
    qint64 g_nDataSize;
    qint32 g_nBytesProLine;
    qint32 g_nDataBlockSize;
    qint32 g_nViewStartDelta;
    QByteArray g_baDataBuffer;
    QByteArray g_baDataHexBuffer;
    QList<QString> g_listAddresses;
    SearchProcess::SEARCHDATA g_searchData;
    QShortcut *g_scGoToOffset;
    QShortcut *g_scGoToAddress;
    QShortcut *g_scDumpToFile;
    QShortcut *g_scSelectAll;
    QShortcut *g_scCopyAsHex;
    QShortcut *g_scCopyCursorOffset;
    QShortcut *g_scCopyCursorAddress;
    QShortcut *g_scFind;
    QShortcut *g_scFindNext;
    QShortcut *g_scSignature;
    QShortcut *g_scDisasm;
    QShortcut *g_scMemoryMap;
    qint32 g_nAddressWidth;
};

#endif // XHEXVIEW_H
