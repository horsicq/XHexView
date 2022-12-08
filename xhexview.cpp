/* Copyright (c) 2020-2022 hors<horsicq@gmail.com>
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
#include "xhexview.h"

XHexView::XHexView(QWidget *pParent) : XDeviceTableEditView(pParent)
{
    g_nBytesProLine = 16;  // TODO Set/Get
    g_nDataBlockSize = 0;
    g_nViewStartDelta = 0;
    //    g_smode=SMODE_ANSI;  // TODO Set/Get
    g_nThisBase = 0;
    g_options = {};
    g_nAddressWidth = 8;        // TODO Set/Get
    g_bIsAddressColon = false;  // TODO Check
                                //    g_nPieceSize=1;

    memset(shortCuts, 0, sizeof shortCuts);

    addColumn(tr("Address"), 0, true);
    addColumn(tr("Hex"));
    addColumn(tr("Symbols"), 0, true);

    setTextFont(getMonoFont());  // mb TODO move to XDeviceTableView !!!
    setBlinkingCursorEnable(true);
    //setBlinkingCursorEnable(false);

    g_sCodePage = "";

    g_pCodePageMenu = g_xOptions.createCodePagesMenu(this, true);

    connect(&g_xOptions, SIGNAL(setCodePage(QString)), this, SLOT(_setCodePage(QString)));
}

void XHexView::_adjustView()
{
    setTextFontFromOptions(XOptions::ID_HEX_FONT);

    g_bIsAddressColon = getGlobalOptions()->getValue(XOptions::ID_HEX_ADDRESSCOLON).toBool();

    setBlinkingCursorEnable(getGlobalOptions()->getValue(XOptions::ID_HEX_BLINKINGCURSOR).toBool());
}

void XHexView::adjustView()
{
    _adjustView();

    if (getDevice()) {
        reload(true);
    }
}

void XHexView::setData(QIODevice *pDevice, XHexView::OPTIONS options, bool bReload)
{
    g_options = options;

    setDevice(pDevice);

    XBinary binary(pDevice, true, options.nStartAddress);
    XBinary::_MEMORY_MAP memoryMap = binary.getMemoryMap();

    setMemoryMap(memoryMap);

    resetCursorData();

    if (options.bIsOffsetTitle) {
        setColumnTitle(0, tr("Offset"));
    }

    adjustColumns();

    qint64 nTotalLineCount = getDataSize() / g_nBytesProLine;

    if (getDataSize() % g_nBytesProLine == 0) {
        nTotalLineCount--;
    }

    //    if((getDataSize()>0)&&(getDataSize()<g_nBytesProLine))
    //    {
    //        nTotalLineCount=1;
    //    }

    setTotalLineCount(nTotalLineCount);

    if (options.nStartSelectionOffset) {
        _goToOffset(options.nStartSelectionOffset);
    }

    setSelection(options.nStartSelectionOffset, options.nSizeOfSelection);
    setCursorOffset(options.nStartSelectionOffset, COLUMN_HEX);

    _adjustView();

    if (bReload) {
        reload(true);
    }
}

void XHexView::goToAddress(XADDR nAddress)
{
    _goToOffset(nAddress - g_options.nStartAddress);
    // TODO reload
}

void XHexView::goToOffset(qint64 nOffset)
{
    _goToOffset(nOffset);
}

XADDR XHexView::getStartAddress()
{
    return g_options.nStartAddress;
}

XADDR XHexView::getSelectionInitAddress()
{
    return getSelectionInitOffset() + g_options.nStartAddress;
}

// QChar XHexView::filterSymbol(QChar cChar,SMODE smode)
//{
//     QChar cResult=cChar;

//    if(smode==SMODE_ANSI)
//    {
//        if((cResult<QChar(0x20))||(cResult>QChar(0x7e)))
//        {
//            cResult='.';
//        }
//    }
////    else if(smode==SMODE_SYMBOLS)
////    {
////        if(cResult<QChar(0x20))
////        {
////            cResult='.';
////        }
////    }
////    else if(smode==SMODE_UNICODE)
////    {
////        if(cResult<QChar(0x20))
////        {
////            cResult='.';
////        }
////    }

//    return cResult;
//}

XAbstractTableView::OS XHexView::cursorPositionToOS(XAbstractTableView::CURSOR_POSITION cursorPosition)
{
    OS osResult = {};

    osResult.nOffset = -1;

    if ((cursorPosition.bIsValid) && (cursorPosition.ptype == PT_CELL)) {
        qint64 nBlockOffset = getViewStart() + (cursorPosition.nRow * g_nBytesProLine);

        if (cursorPosition.nColumn == COLUMN_ADDRESS) {
            osResult.nOffset = nBlockOffset;
            //            osResult.nSize=g_nPieceSize;
            osResult.nSize = 1;
        } else if (cursorPosition.nColumn == COLUMN_HEX) {
            osResult.nOffset = nBlockOffset + (cursorPosition.nCellLeft - getSideDelta() - getCharWidth()) / (getCharWidth() * 2 + getSideDelta());
            //            osResult.nSize=g_nPieceSize;
            osResult.nSize = 1;
        } else if (cursorPosition.nColumn == COLUMN_SYMBOLS) {
            osResult.nOffset = nBlockOffset + (cursorPosition.nCellLeft - getSideDelta() - getCharWidth()) / getCharWidth();
            //            osResult.nSize=g_nPieceSize;
            osResult.nSize = 1;
        }

        //        osResult.nOffset=S_ALIGN_DOWN(osResult.nOffset,g_nPieceSize);

        if (!isOffsetValid(osResult.nOffset)) {
            osResult.nOffset = getDataSize();  // TODO Check
            osResult.nSize = 0;
        }

        //        qDebug("nBlockOffset %x",nBlockOffset);
        //        qDebug("cursorPosition.nCellLeft %x",cursorPosition.nCellLeft);
        //        qDebug("getCharWidth() %x",getCharWidth());
        //        qDebug("nOffset %x",osResult.nOffset);
    }

    return osResult;
}

void XHexView::updateData()
{
    if (getDevice()) {
        if (getXInfoDB()) {
            QList<XBinary::MEMORY_REPLACE> listMR = getXInfoDB()->getMemoryReplaces(getMemoryMap()->nModuleAddress, getMemoryMap()->nImageSize);

            setMemoryReplaces(listMR);
        }
        // Update cursor position
        qint64 nBlockOffset = getViewStart();
        qint64 nCursorOffset = nBlockOffset + getCursorDelta();

        if (nCursorOffset >= getDataSize()) {
            nCursorOffset = getDataSize() - 1;
        }

        setCursorOffset(nCursorOffset);

        XBinary::MODE mode = XBinary::getWidthModeFromByteSize(g_nAddressWidth);

        g_listRecords.clear();

        qint32 nDataBlockSize = g_nBytesProLine * getLinesProPage();

        g_baDataBuffer = read_array(nBlockOffset, nDataBlockSize);
        g_sStringBuffer = getStringBuffer(&g_baDataBuffer);

        g_nDataBlockSize = g_baDataBuffer.size();

        if (g_nDataBlockSize) {
            g_baDataHexBuffer = QByteArray(g_baDataBuffer.toHex());

            for (qint32 i = 0; i < g_nDataBlockSize; i += g_nBytesProLine) {
                XADDR nCurrentAddress = 0;

                RECORD record = {};
                record.nAddress = i + g_options.nStartAddress + nBlockOffset;

                if (getAddressMode() == MODE_THIS) {
                    nCurrentAddress = record.nAddress;

                    qint64 nDelta = (qint64)nCurrentAddress - (qint64)g_nThisBase;

                    record.sAddress = XBinary::thisToString(nDelta);
                } else {
                    if (getAddressMode() == MODE_ADDRESS) {
                        nCurrentAddress = record.nAddress;
                    } else if (getAddressMode() == MODE_OFFSET) {
                        nCurrentAddress = i + nBlockOffset;
                    }

                    if (g_bIsAddressColon) {
                        record.sAddress = XBinary::valueToHexColon(mode, nCurrentAddress);
                    } else {
                        record.sAddress = XBinary::valueToHex(mode, nCurrentAddress);
                    }
                }

                g_listRecords.append(record);
            }
        } else {
            g_baDataBuffer.clear();
            g_baDataHexBuffer.clear();
        }

        setCurrentBlock(nBlockOffset, g_nDataBlockSize);
    }
}

void XHexView::paintCell(QPainter *pPainter, qint32 nRow, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight)
{
    Q_UNUSED(nWidth)
    //    g_pPainterText->drawRect(nLeft,nTop,nWidth,nHeight);
    if (nColumn == COLUMN_ADDRESS) {
        if (nRow < g_listRecords.count()) {
            QRect rectSymbol;

            rectSymbol.setLeft(nLeft + getCharWidth());
            rectSymbol.setTop(nTop + getLineDelta());
            rectSymbol.setWidth(nWidth);
            rectSymbol.setHeight(nHeight - getLineDelta());

            //            pPainter->save();
            //            pPainter->setPen(viewport()->palette().color(QPalette::Dark));
            pPainter->drawText(rectSymbol, g_listRecords.at(nRow).sAddress);  // TODO Text Optional
                                                                              //            pPainter->restore();
        }
    } else if ((nColumn == COLUMN_HEX) || (nColumn == COLUMN_SYMBOLS)) {
        STATE state = getState();

        if (nRow * g_nBytesProLine < g_nDataBlockSize) {
            qint64 nDataBlockStartOffset = getViewStart();
            qint64 nDataBlockSize = qMin(g_nDataBlockSize - nRow * g_nBytesProLine, g_nBytesProLine);

            for (qint32 i = 0; i < nDataBlockSize; i++) {
                qint32 nIndex = nRow * g_nBytesProLine + i;

                QString sHex;
                QString sSymbol;
                bool bBold = false;
                QRect rectSymbol;

                sHex = g_baDataHexBuffer.mid(nIndex * 2, 2);
                bBold = (sHex != "00");

                bool bSelected = isOffsetSelected(nDataBlockStartOffset + nIndex);
                bool bCursor = (state.nCursorOffset == (nDataBlockStartOffset + nIndex));

                if (bBold) {
                    pPainter->save();
                    QFont font = pPainter->font();
                    font.setBold(true);
                    pPainter->setFont(font);
                }

                if (nColumn == COLUMN_HEX) {
                    //                    rectSymbol.setRect(nLeft+getCharWidth()+(i*2)*getCharWidth()+i*getSideDelta(),nTop,2*getCharWidth()+getSideDelta(),nHeight);
                    rectSymbol.setLeft(nLeft + getCharWidth() + (i * 2) * getCharWidth() + i * getSideDelta());
                    rectSymbol.setTop(nTop + getLineDelta());
                    rectSymbol.setWidth(2 * getCharWidth() + getSideDelta());
                    rectSymbol.setHeight(nHeight - getLineDelta());

                    sSymbol = sHex;
                } else if (nColumn == COLUMN_SYMBOLS) {
                    //                    rectSymbol.setRect(nLeft+(i+1)*getCharWidth(),nTop,getCharWidth(),nHeight);
                    rectSymbol.setLeft(nLeft + (i + 1) * getCharWidth());
                    rectSymbol.setTop(nTop + getLineDelta());
                    rectSymbol.setWidth(getCharWidth());
                    rectSymbol.setHeight(nHeight - getLineDelta());

                    sSymbol = g_sStringBuffer.mid(nIndex, 1);

                    // TODO

                    //                    if((getSmode()==SMODE_ANSI)||(getSmode()==SMODE_SYMBOLS))
                    //                    {
                    //                        QByteArray baChar=g_baDataBuffer.mid(nIndex,1); // TODO Check

                    //                        if(baChar.size())
                    //                        {
                    //                            QChar cChar;

                    //                            cChar=g_baDataBuffer.mid(nIndex,1).at(0); // TODO Check
                    //                            sSymbol=filterSymbol(cChar,getSmode());
                    //                        }
                    //                    }
                    //                    else if(getSmode()==SMODE_UNICODE)
                    //                    {
                    //                        if((nDataBlockStartOffset+nIndex)%2==0)
                    //                        {
                    //                            QByteArray baChar=g_baDataBuffer.mid(nIndex,2); // TODO Check

                    //                            if(baChar.size()==2)
                    //                            {
                    //                                quint16 nCode=XBinary::_read_uint16(baChar.data());

                    //                                QChar cChar(nCode);

                    //                                sSymbol=filterSymbol(cChar,getSmode());
                    //                            }
                    //                        }
                    //                    }
                }

                if (bSelected || bCursor) {
                    //                    QRect rectSelected;
                    //// rectSelected.setRect(rectSymbol.x(),rectSymbol.y()+getLineDelta(),rectSymbol.width()*g_nPieceSize,rectSymbol.height());
                    //                    rectSelected.setRect(rectSymbol.x(),rectSymbol.y()+getLineDelta(),rectSymbol.width(),rectSymbol.height());

                    if (bCursor) {
                        if (nColumn == state.cursorPosition.nColumn) {
                            //                            setCursorData(rectSelected,rectSelected,sSymbol,nIndex);
                            setCursorData(rectSymbol, rectSymbol, sSymbol, nIndex);
                        }
                    }

                    if (bSelected) {
                        //                        pPainter->fillRect(rectSelected,viewport()->palette().color(QPalette::Highlight));
                        pPainter->fillRect(rectSymbol, viewport()->palette().color(QPalette::Highlight));
                    }
                }

                pPainter->drawText(rectSymbol, sSymbol);

                if (bBold) {
                    pPainter->restore();
                }
            }
        }
    }
}

void XHexView::paintTitle(QPainter *pPainter, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight, QString sTitle)
{
    if (nColumn == COLUMN_HEX) {
        for (qint8 i = 0; i < g_nBytesProLine; i++) {

            QString sSymbol = XBinary::valueToHex(i);

            QRect rectSymbol;

            rectSymbol.setLeft(nLeft + getCharWidth() + (i * 2) * getCharWidth() + i * getSideDelta());
            rectSymbol.setTop(nTop);
            rectSymbol.setWidth(2 * getCharWidth() + getSideDelta());
            rectSymbol.setHeight(nHeight);

            pPainter->drawText(rectSymbol, Qt::AlignVCenter | Qt::AlignLeft, sSymbol);
        }
    } else {
        XAbstractTableView::paintTitle(pPainter, nColumn, nLeft, nTop, nWidth, nHeight, sTitle);
    }
}

void XHexView::contextMenu(const QPoint &pos)
{
    if (isContextMenuEnable()) {
        QAction actionGoToOffset(tr("Offset"), this);
        actionGoToOffset.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_GOTO_OFFSET));
        connect(&actionGoToOffset, SIGNAL(triggered()), this, SLOT(_goToOffsetSlot()));

        QAction actionGoToAddress(tr("Address"), this);
        actionGoToAddress.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_GOTO_ADDRESS));
        connect(&actionGoToAddress, SIGNAL(triggered()), this, SLOT(_goToAddressSlot()));

        QAction actionGoToSelectionStart(tr("Start"), this);
        actionGoToSelectionStart.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_GOTO_SELECTION_START));
        connect(&actionGoToSelectionStart, SIGNAL(triggered()), this, SLOT(_goToSelectionStart()));

        QAction actionGoToSelectionEnd(tr("End"), this);
        actionGoToSelectionEnd.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_GOTO_SELECTION_END));
        connect(&actionGoToSelectionEnd, SIGNAL(triggered()), this, SLOT(_goToSelectionEnd()));

        QAction actionDumpToFile(tr("Dump to file"), this);
        actionDumpToFile.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_DUMPTOFILE));
        connect(&actionDumpToFile, SIGNAL(triggered()), this, SLOT(_dumpToFileSlot()));

        QAction actionSignature(tr("Signature"), this);
        actionSignature.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_SIGNATURE));
        connect(&actionSignature, SIGNAL(triggered()), this, SLOT(_hexSignatureSlot()));

        QAction actionFindString(tr("String"), this);
        actionFindString.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_FIND_STRING));
        connect(&actionFindString, SIGNAL(triggered()), this, SLOT(_findStringSlot()));

        QAction actionFindSignature(tr("Signature"), this);
        actionFindSignature.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_FIND_SIGNATURE));
        connect(&actionFindSignature, SIGNAL(triggered()), this, SLOT(_findSignatureSlot()));

        QAction actionFindValue(tr("Value"), this);
        actionFindValue.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_FIND_VALUE));
        connect(&actionFindValue, SIGNAL(triggered()), this, SLOT(_findValueSlot()));

        QAction actionFindNext(tr("Find next"), this);
        actionFindNext.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_FIND_NEXT));
        connect(&actionFindNext, SIGNAL(triggered()), this, SLOT(_findNextSlot()));

        QAction actionSelectAll(tr("Select all"), this);
        actionSelectAll.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_SELECT_ALL));
        connect(&actionSelectAll, SIGNAL(triggered()), this, SLOT(_selectAllSlot()));

        QAction actionCopyHex(tr("Hex"), this);
        actionCopyHex.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_COPY_HEX));
        connect(&actionCopyHex, SIGNAL(triggered()), this, SLOT(_copyHexSlot()));

        QAction actionCopyCursorOffset(tr("Offset"), this);
        actionCopyCursorOffset.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_COPY_OFFSET));
        connect(&actionCopyCursorOffset, SIGNAL(triggered()), this, SLOT(_copyOffsetSlot()));

        QAction actionCopyCursorAddress(tr("Address"), this);
        actionCopyCursorAddress.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_COPY_ADDRESS));
        connect(&actionCopyCursorAddress, SIGNAL(triggered()), this, SLOT(_copyAddressSlot()));

        QAction actionDisasm(tr("Disasm"), this);
        actionDisasm.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_FOLLOWIN_DISASM));
        connect(&actionDisasm, SIGNAL(triggered()), this, SLOT(_disasmSlot()));

        QAction actionMemoryMap(tr("Memory map"), this);
        actionMemoryMap.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_FOLLOWIN_MEMORYMAP));
        connect(&actionMemoryMap, SIGNAL(triggered()), this, SLOT(_memoryMapSlot()));

        QAction actionMainHex(tr("Hex"), this);
        actionMainHex.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_FOLLOWIN_HEX));
        connect(&actionMainHex, SIGNAL(triggered()), this, SLOT(_mainHexSlot()));

        QAction actionEditHex(tr("Hex"), this);
        actionEditHex.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_EDIT_HEX));
        connect(&actionEditHex, SIGNAL(triggered()), this, SLOT(_editHex()));

        STATE menuState = getState();

        // TODO string from XShortcuts
        QMenu contextMenu(this);
        QMenu menuGoTo(tr("Go to"), this);
        QMenu menuGoToSelection(tr("Selection"), this);
        QMenu menuFind(tr("Find"), this);
        QMenu menuSelect(tr("Select"), this);
        QMenu menuCopy(tr("Copy"), this);
        QMenu menuFollowIn(tr("Follow in"), this);
        QMenu menuEdit(tr("Edit"), this);

        menuGoTo.addAction(&actionGoToOffset);
        menuGoTo.addAction(&actionGoToAddress);
        menuGoTo.addMenu(&menuGoToSelection);
        menuGoToSelection.addAction(&actionGoToSelectionStart);
        menuGoToSelection.addAction(&actionGoToSelectionEnd);

        contextMenu.addMenu(&menuGoTo);

        menuFind.addAction(&actionFindString);
        menuFind.addAction(&actionFindSignature);
        menuFind.addAction(&actionFindValue);
        menuFind.addAction(&actionFindNext);

        contextMenu.addMenu(&menuFind);

        menuCopy.addAction(&actionCopyCursorOffset);
        menuCopy.addAction(&actionCopyCursorAddress);

        if (menuState.nSelectionSize) {
            contextMenu.addAction(&actionDumpToFile);
            contextMenu.addAction(&actionSignature);

            menuCopy.addAction(&actionCopyHex);
        }

        contextMenu.addMenu(&menuCopy);

        if (g_options.bMenu_Disasm) {
            menuFollowIn.addAction(&actionDisasm);
        }

        if (g_options.bMenu_MemoryMap) {
            menuFollowIn.addAction(&actionMemoryMap);
        }

        if (g_options.bMenu_MainHex) {
            menuFollowIn.addAction(&actionMainHex);
        }

        if ((g_options.bMenu_Disasm) || (g_options.bMenu_MemoryMap)) {
            contextMenu.addMenu(&menuFollowIn);
        }

        menuEdit.setEnabled(!isReadonly());

        if (menuState.nSelectionSize) {
            menuEdit.addAction(&actionEditHex);

            contextMenu.addMenu(&menuEdit);
        }

        menuSelect.addAction(&actionSelectAll);
        contextMenu.addMenu(&menuSelect);

        // TODO reset select

        contextMenu.exec(pos);
    }
}

void XHexView::wheelEvent(QWheelEvent *pEvent)
{
    if ((g_nViewStartDelta) && (pEvent->angleDelta().y() > 0)) {
        if (getScrollValue() == g_nViewStartDelta) {
            setScrollValue(0);
            adjust(true);
            viewport()->update();
        }
    }

    XAbstractTableView::wheelEvent(pEvent);
}

void XHexView::keyPressEvent(QKeyEvent *pEvent)
{
    // Move commands
    if (pEvent->matches(QKeySequence::MoveToNextChar) || pEvent->matches(QKeySequence::MoveToPreviousChar) || pEvent->matches(QKeySequence::MoveToNextLine) ||
        pEvent->matches(QKeySequence::MoveToPreviousLine) || pEvent->matches(QKeySequence::MoveToStartOfLine) || pEvent->matches(QKeySequence::MoveToEndOfLine) ||
        pEvent->matches(QKeySequence::MoveToNextPage) || pEvent->matches(QKeySequence::MoveToPreviousPage) || pEvent->matches(QKeySequence::MoveToStartOfDocument) ||
        pEvent->matches(QKeySequence::MoveToEndOfDocument)) {
        qint64 nViewStart = getViewStart();

        if (pEvent->matches(QKeySequence::MoveToNextChar)) {
            setCursorOffset(getCursorOffset() + 1);
        } else if (pEvent->matches(QKeySequence::MoveToPreviousChar)) {
            setCursorOffset(getCursorOffset() - 1);
        } else if (pEvent->matches(QKeySequence::MoveToNextLine)) {
            setCursorOffset(getCursorOffset() + g_nBytesProLine);
        } else if (pEvent->matches(QKeySequence::MoveToPreviousLine)) {
            setCursorOffset(getCursorOffset() - g_nBytesProLine);
        } else if (pEvent->matches(QKeySequence::MoveToStartOfLine)) {
            setCursorOffset(getCursorOffset() - (getCursorDelta() % g_nBytesProLine));
        } else if (pEvent->matches(QKeySequence::MoveToEndOfLine)) {
            setCursorOffset(getCursorOffset() - (getCursorDelta() % g_nBytesProLine) + g_nBytesProLine - 1);
        }

        if ((getCursorOffset() < 0) || (pEvent->matches(QKeySequence::MoveToStartOfDocument))) {
            setCursorOffset(0);
            g_nViewStartDelta = 0;
        }

        if ((getCursorOffset() >= getDataSize()) || (pEvent->matches(QKeySequence::MoveToEndOfDocument))) {
            setCursorOffset(getDataSize() - 1);
            g_nViewStartDelta = 0;
        }

        if (pEvent->matches(QKeySequence::MoveToNextChar) || pEvent->matches(QKeySequence::MoveToPreviousChar) || pEvent->matches(QKeySequence::MoveToNextLine) ||
            pEvent->matches(QKeySequence::MoveToPreviousLine)) {
            qint64 nRelOffset = getCursorOffset() - nViewStart;

            if (nRelOffset >= g_nBytesProLine * getLinesProPage()) {
                _goToOffset(nViewStart + g_nBytesProLine, true);
            } else if (nRelOffset < 0) {
                if (!_goToOffset(nViewStart - g_nBytesProLine, true)) {
                    _goToOffset(0);
                }
            }
        } else if (pEvent->matches(QKeySequence::MoveToNextPage) || pEvent->matches(QKeySequence::MoveToPreviousPage)) {
            if (pEvent->matches(QKeySequence::MoveToNextPage)) {
                _goToOffset(nViewStart + g_nBytesProLine * getLinesProPage());
            } else if (pEvent->matches(QKeySequence::MoveToPreviousPage)) {
                _goToOffset(nViewStart - g_nBytesProLine * getLinesProPage());
            }
        } else if (pEvent->matches(QKeySequence::MoveToStartOfDocument) || pEvent->matches(QKeySequence::MoveToEndOfDocument))  // TODO
        {
            _goToOffset(getCursorOffset());
        }

        adjust();
        viewport()->update();
    }
    //    else if(pEvent->matches(QKeySequence::SelectAll))
    //    {
    //        _selectAllSlot();
    //    }
    else {
        XAbstractTableView::keyPressEvent(pEvent);
    }
}

qint64 XHexView::getScrollValue()
{
    qint64 nResult = 0;

    qint32 nValue = verticalScrollBar()->value();

    qint64 nMaxValue = getMaxScrollValue() * g_nBytesProLine;

    if (getDataSize() > nMaxValue) {
        if (nValue == getMaxScrollValue()) {
            nResult = getDataSize() - g_nBytesProLine;
        } else {
            nResult = ((double)nValue / (double)getMaxScrollValue()) * getDataSize() + g_nViewStartDelta;
        }
    } else {
        nResult = (qint64)nValue * g_nBytesProLine + g_nViewStartDelta;
    }

    return nResult;
}

void XHexView::setScrollValue(qint64 nOffset)
{
    setViewStart(nOffset);
    g_nViewStartDelta = (nOffset) % g_nBytesProLine;

    qint32 nValue = 0;

    if (getDataSize() > (getMaxScrollValue() * g_nBytesProLine)) {
        if (nOffset == getDataSize() - g_nBytesProLine) {
            nValue = getMaxScrollValue();
        } else {
            nValue = ((double)(nOffset - g_nViewStartDelta) / ((double)getDataSize())) * (double)getMaxScrollValue();
        }
    } else {
        nValue = (nOffset) / g_nBytesProLine;
    }

    {
        const bool bBlocked1 = verticalScrollBar()->blockSignals(true);

        verticalScrollBar()->setValue(nValue);
        _verticalScroll();

        verticalScrollBar()->blockSignals(bBlocked1);
    }
}

void XHexView::adjustColumns()
{
    const QFontMetricsF fm(getTextFont());

    if (XBinary::getWidthModeFromSize(getStartAddress() + getDataSize()) == XBinary::MODE_64) {
        g_nAddressWidth = 16;
        setColumnWidth(COLUMN_ADDRESS, 2 * getCharWidth() + fm.boundingRect("00000000:00000000").width());
    } else {
        g_nAddressWidth = 8;
        setColumnWidth(COLUMN_ADDRESS, 2 * getCharWidth() + fm.boundingRect("0000:0000").width());
    }

    setColumnWidth(COLUMN_HEX, g_nBytesProLine * 2 * getCharWidth() + 2 * getCharWidth() + getSideDelta() * g_nBytesProLine);
    setColumnWidth(COLUMN_SYMBOLS, (g_nBytesProLine + 2) * getCharWidth());
}

void XHexView::registerShortcuts(bool bState)
{
    if (bState) {
        if (!shortCuts[SC_GOTO_OFFSET]) shortCuts[SC_GOTO_OFFSET] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_GOTO_OFFSET), this, SLOT(_goToOffsetSlot()));
        if (!shortCuts[SC_GOTO_ADDRESS]) shortCuts[SC_GOTO_ADDRESS] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_GOTO_ADDRESS), this, SLOT(_goToAddressSlot()));
        if (!shortCuts[SC_DUMPTOFILE]) shortCuts[SC_DUMPTOFILE] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_DUMPTOFILE), this, SLOT(_dumpToFileSlot()));
        if (!shortCuts[SC_SELECTALL]) shortCuts[SC_SELECTALL] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_SELECT_ALL), this, SLOT(_selectAllSlot()));
        if (!shortCuts[SC_COPYHEX]) shortCuts[SC_COPYHEX] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_COPY_HEX), this, SLOT(_copyHexSlot()));
        if (!shortCuts[SC_COPYOFFSET]) shortCuts[SC_COPYOFFSET] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_COPY_OFFSET), this, SLOT(_copyOffsetSlot()));
        if (!shortCuts[SC_COPYADDRESS]) shortCuts[SC_COPYADDRESS] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_COPY_ADDRESS), this, SLOT(_copyAddressSlot()));
        if (!shortCuts[SC_FINDSTRING]) shortCuts[SC_FINDSTRING] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_FIND_STRING), this, SLOT(_findStringSlot()));
        if (!shortCuts[SC_FINDSIGNATURE])
            shortCuts[SC_FINDSIGNATURE] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_FIND_SIGNATURE), this, SLOT(_findSignatureSlot()));
        if (!shortCuts[SC_FINDVALUE]) shortCuts[SC_FINDVALUE] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_FIND_VALUE), this, SLOT(_findValueSlot()));
        if (!shortCuts[SC_FINDNEXT]) shortCuts[SC_FINDNEXT] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_FIND_NEXT), this, SLOT(_findNextSlot()));
        if (!shortCuts[SC_SIGNATURE]) shortCuts[SC_SIGNATURE] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_SIGNATURE), this, SLOT(_hexSignatureSlot()));
        if (!shortCuts[SC_DISASM]) shortCuts[SC_DISASM] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_FOLLOWIN_DISASM), this, SLOT(_disasmSlot()));
        if (!shortCuts[SC_MEMORYMAP]) shortCuts[SC_MEMORYMAP] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_FOLLOWIN_MEMORYMAP), this, SLOT(_memoryMapSlot()));
        if (!shortCuts[SC_MAINHEX]) shortCuts[SC_MAINHEX] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_FOLLOWIN_HEX), this, SLOT(_mainHexSlot()));
        if (!shortCuts[SC_EDITHEX]) shortCuts[SC_EDITHEX] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_EDIT_HEX), this, SLOT(_editHex()));
    } else {
        for (qint32 i = 0; i < __SC_SIZE; i++) {
            if (shortCuts[i]) {
                delete shortCuts[i];
                shortCuts[i] = nullptr;
            }
        }
    }
}

void XHexView::_headerClicked(qint32 nColumn)
{
    if (nColumn == COLUMN_ADDRESS) {
        if (getAddressMode() == MODE_ADDRESS) {
            setColumnTitle(COLUMN_ADDRESS, tr("Offset"));
            setAddressMode(MODE_OFFSET);
        } else if ((getAddressMode() == MODE_OFFSET) || (getAddressMode() == MODE_THIS)) {
            setColumnTitle(COLUMN_ADDRESS, tr("Address"));
            setAddressMode(MODE_ADDRESS);
        }

        adjust(true);
    } else if (nColumn == COLUMN_SYMBOLS) {
        g_pCodePageMenu->exec(QCursor::pos());

        //        delete pMenu; // TODO !!!
        // TODO
        //        if(getSmode()==SMODE_SYMBOLS)
        //        {
        //            setColumnTitle(COLUMN_SYMBOLS,QString("ANSI"));
        //            setSmode(SMODE_ANSI);
        //        }
        //        else if(getSmode()==SMODE_ANSI)
        //        {
        //            setColumnTitle(COLUMN_SYMBOLS,QString("Unicode"));
        //            setSmode(SMODE_UNICODE);
        //        }
        //        else if(getSmode()==SMODE_UNICODE)
        //        {
        //            setColumnTitle(COLUMN_SYMBOLS,tr("Symbols"));
        //            setSmode(SMODE_SYMBOLS);
        //        }

        //        adjust(true);
    }
}

void XHexView::_cellDoubleClicked(qint32 nRow, qint32 nColumn)
{
    if (nColumn == COLUMN_ADDRESS) {
        setColumnTitle(COLUMN_ADDRESS, "");
        setAddressMode(MODE_THIS);

        if (nRow < g_listRecords.count()) {
            g_nThisBase = g_listRecords.at(nRow).nAddress;
        }

        adjust(true);
    }
}

QString XHexView::getStringBuffer(QByteArray *pbaData)
{
    QString sResult;

    qint32 nSize = pbaData->size();

    if (g_sCodePage == "") {
        for (qint32 i = 0; i < nSize; i++) {
            QChar _char = pbaData->at(i);

            if ((_char < QChar(0x20)) || (_char > QChar(0x7e))) {
                _char = '.';
            }

            sResult.append(_char);
        }
    } else {
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
        QTextCodec *pCodec = QTextCodec::codecForName(g_sCodePage.toLatin1().data());

        if (pCodec) {
            QString _sResult = pCodec->toUnicode(*pbaData);
            QVector<uint> vecSymbols = _sResult.toUcs4();
            qint32 _nSize = vecSymbols.size();

            if (_nSize == nSize) {
                for (qint32 i = 0; i < nSize; i++) {
                    QChar _char = _sResult.at(i);

                    if (_char < QChar(0x20)) {
                        _char = '.';
                    }

                    sResult.append(_char);
                }
            } else {
                //                QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme,_sResult);

                //                qint32 nCurrentPosition=0;

                //                while(true)
                //                {
                //                    qint32 _nCurrentPosition=finder.toNextBoundary();

                //                    QString _sChar=_sResult.mid(nCurrentPosition,_nCurrentPosition-nCurrentPosition);
                //                    QByteArray _baData=pCodec->fromUnicode(_sChar);

                //                    if(_sChar.size()==1)
                //                    {
                //                        if(_sChar.at(0)<QChar(0x20))
                //                        {
                //                            _sChar='.';
                //                        }
                //                    }

                //                    sResult.append(_sChar);

                //                    if(_baData.size()>1)
                //                    {
                //                        qint32 nAppendSize=_baData.size()-1;

                //                        for(qint32 j=0;j<nAppendSize;j++)
                //                        {
                //                            sResult.append(" "); // mb TODO another symbol
                //                        }
                //                    }

                //                    nCurrentPosition=_nCurrentPosition;

                //                    if(nCurrentPosition==-1)
                //                    {
                //                        break;
                //                    }
                //                }

                for (qint32 i = 0; i < _nSize; i++) {
                    QString _sChar = _sResult.mid(i, 1);

                    QByteArray _baData = pCodec->fromUnicode(_sChar);

                    if (_sChar.at(0) < QChar(0x20)) {
                        _sChar = '.';
                    }

                    sResult.append(_sChar);

                    if (_baData.size() > 1) {
                        qint32 nAppendSize = _baData.size() - 1;

                        for (qint32 j = 0; j < nAppendSize; j++) {
                            sResult.append(" ");  // mb TODO another symbol
                        }
                    }
                }
            }
        }
#endif
    }

    return sResult;
}

// XHexView::SMODE XHexView::getSmode()
//{
//     return g_smode;
// }

// void XHexView::setSmode(SMODE smode)
//{
//     g_smode=smode;

////    if(smode==SMODE_UNICODE)
////    {
////        g_nPieceSize=2;
////    }
////    else
////    {
////        g_nPieceSize=1;
////    }
//}

void XHexView::_disasmSlot()
{
    if (g_options.bMenu_Disasm) {
        emit showOffsetDisasm(getStateOffset());
    }
}

void XHexView::_memoryMapSlot()
{
    if (g_options.bMenu_MemoryMap) {
        emit showOffsetMemoryMap(getStateOffset());
    }
}

void XHexView::_mainHexSlot()
{
    if (g_options.bMenu_MainHex) {
        emit showOffsetMainHex(getStateOffset(), getState().nSelectionSize);
    }
}

void XHexView::_setCodePage(QString sCodePage)
{
    g_sCodePage = sCodePage;

    QString sTitle = tr("Symbols");

    if (sCodePage != "") {
        sTitle = sCodePage;
    }

    setColumnTitle(COLUMN_SYMBOLS, sTitle);

    adjust(true);
}
