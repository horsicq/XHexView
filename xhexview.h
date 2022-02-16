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

#include "xdevicetableview.h"

class XHexView : public XDeviceTableView
{
    Q_OBJECT

public:
    // TODO edit function
    // TODO setOptions
    // TODO follow functions
    struct OPTIONS
    {
        qint64 nStartAddress;
        qint64 nStartSelectionOffset;
        qint64 nSizeOfSelection;
        bool bMenu_Disasm;
        bool bMenu_MemoryMap;
        QString sTitle; // For dialogs
        bool bIsOffsetTitle;
    };

    explicit XHexView(QWidget *pParent=nullptr);

    void adjustView();

    void setData(QIODevice *pDevice,OPTIONS options);
    void goToAddress(qint64 nAddress);
    void goToOffset(qint64 nOffset);
    qint64 getStartAddress();
    void setReadonly(bool bState);
    void enableReadOnly(bool bState);
    void setEdited(bool bState);
    qint64 getSelectionInitAddress();

private:
    enum COLUMN
    {
        COLUMN_ADDRESS=0,
        COLUMN_HEX,
        COLUMN_SYMBOLS
    };

    struct RECORD
    {
        QString sAddress;
        qint64 nAddress;
    };

    QChar filterSymbol(QChar cChar);

protected:
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
    virtual void _headerClicked(qint32 nColumn);
    virtual void _cellDoubleClicked(qint32 nRow,qint32 nColumn);

private slots:
    void _disasmSlot();
    void _memoryMapSlot();

signals:
    void showOffsetDisasm(qint64 nOffset);
    void showOffsetMemoryMap(qint64 nOffset);

private:
    OPTIONS g_options;
    qint32 g_nBytesProLine;
    qint32 g_nDataBlockSize;
    qint32 g_nViewStartDelta;
    QByteArray g_baDataBuffer;
    QByteArray g_baDataHexBuffer;
    QList<RECORD> g_listRecords;
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
    qint64 g_nThisBase;
};

#endif // XHEXVIEW_H
