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

XHexView::XHexView(QWidget *pParent) : XDeviceTableView(pParent)
{
    g_nBytesProLine=16;
    g_nDataBlockSize=0;
    g_nViewStartDelta=0;
    g_smode=SMODE_ANSI;

    memset(shortCuts,0,sizeof shortCuts);

    g_nThisBase=0;

    g_options={};

    g_nAddressWidth=8;

    addColumn(tr("Address"),0,true);
    addColumn(tr("Hex"));
    addColumn(tr("Symbols"),0,true);

    setTextFont(getMonoFont());
}

void XHexView::adjustView()
{
    QFont _font;
    QString sFont=getGlobalOptions()->getValue(XOptions::ID_HEX_FONT).toString();

    if((sFont!="")&&_font.fromString(sFont))
    {
        setTextFont(_font);
    }
    // mb TODO errorString signal if invalid font

    if(getDevice())
    {
        reload(true);
    }
}

void XHexView::setData(QIODevice *pDevice,XHexView::OPTIONS options)
{
    g_options=options;

    setDevice(pDevice);

    XBinary binary(pDevice,true,options.nStartAddress);
    XBinary::_MEMORY_MAP memoryMap=binary.getMemoryMap();

    setMemoryMap(memoryMap);

    resetCursorData();

    if(options.bIsOffsetTitle)
    {
        setColumnTitle(0,tr("Offset"));
    }

    adjustColumns();

    qint64 nTotalLineCount=getDataSize()/g_nBytesProLine;

    if(getDataSize()%g_nBytesProLine==0)
    {
        nTotalLineCount--;
    }

//    if((getDataSize()>0)&&(getDataSize()<g_nBytesProLine))
//    {
//        nTotalLineCount=1;
//    }

    setTotalLineCount(nTotalLineCount);

    if(options.nStartSelectionOffset)
    {
        _goToOffset(options.nStartSelectionOffset);
    }

    setSelection(options.nStartSelectionOffset,options.nSizeOfSelection);
    setCursorOffset(options.nStartSelectionOffset,COLUMN_HEX);

    reload(true);
}

void XHexView::goToAddress(qint64 nAddress)
{
    _goToOffset(nAddress-g_options.nStartAddress);
    // TODO reload
}

void XHexView::goToOffset(qint64 nOffset)
{
    _goToOffset(nOffset);
}

qint64 XHexView::getStartAddress()
{
    return g_options.nStartAddress;
}

qint64 XHexView::getSelectionInitAddress()
{
    return getSelectionInitOffset()+g_options.nStartAddress;
}

QChar XHexView::filterSymbol(QChar cChar,SMODE smode)
{
    QChar cResult=cChar;

    if(smode==SMODE_ANSI)
    {
        if((cResult<QChar(0x20))||(cResult>QChar(0x7e)))
        {
            cResult='.';
        }
    }
    else if(smode==SMODE_SYMBOLS)
    {
        if(cResult<QChar(0x20))
        {
            cResult='.';
        }
    }
    else if(smode==SMODE_UNICODE)
    {
        if(cResult<QChar(0x20))
        {
            cResult='.';
        }
    }

    return cResult;
}

XAbstractTableView::OS XHexView::cursorPositionToOS(XAbstractTableView::CURSOR_POSITION cursorPosition)
{
    OS osResult={};

    osResult.nOffset=-1;

    if((cursorPosition.bIsValid)&&(cursorPosition.ptype==PT_CELL))
    {
        qint64 nBlockOffset=getViewStart()+(cursorPosition.nRow*g_nBytesProLine);

        if(cursorPosition.nColumn==COLUMN_ADDRESS)
        {
            osResult.nOffset=nBlockOffset;
            osResult.nSize=1;
        }
        else if(cursorPosition.nColumn==COLUMN_HEX)
        {
            osResult.nOffset=nBlockOffset+(cursorPosition.nCellLeft-getLineDelta())/(getCharWidth()*2+getLineDelta());
            osResult.nSize=1;
        }
        else if(cursorPosition.nColumn==COLUMN_SYMBOLS)
        {
            osResult.nOffset=nBlockOffset+(cursorPosition.nCellLeft-getLineDelta())/getCharWidth();
            osResult.nSize=1;
        }

        if(!isOffsetValid(osResult.nOffset))
        {
            osResult.nOffset=getDataSize(); // TODO Check
            osResult.nSize=0;
        }
    }

    return osResult;
}

void XHexView::updateData()
{
    if(getDevice())
    {
        // Update cursor position
        qint64 nBlockOffset=getViewStart();
        qint64 nCursorOffset=nBlockOffset+getCursorDelta();

        if(nCursorOffset>=getDataSize())
        {
            nCursorOffset=getDataSize()-1;
        }

        setCursorOffset(nCursorOffset);

        XBinary::MODE mode=XBinary::getWidthModeFromByteSize(g_nAddressWidth);

        g_listRecords.clear();

        qint32 nDataBlockSize=g_nBytesProLine*getLinesProPage();

        g_baDataBuffer=read_array(nBlockOffset,nDataBlockSize);

        g_nDataBlockSize=g_baDataBuffer.size();

        if(g_nDataBlockSize)
        {
            g_baDataHexBuffer=QByteArray(g_baDataBuffer.toHex());

            for(qint32 i=0;i<g_nDataBlockSize;i+=g_nBytesProLine)
            {
                qint64 nCurrentAddress=0;

                RECORD record={};
                record.nAddress=i+g_options.nStartAddress+nBlockOffset;

                if(getAddressMode()==MODE_THIS)
                {
                    nCurrentAddress=record.nAddress;

                    qint64 nDelta=nCurrentAddress-g_nThisBase;

                    record.sAddress=XBinary::thisToString(nDelta);
                }
                else
                {
                    if(getAddressMode()==MODE_ADDRESS)
                    {
                        nCurrentAddress=record.nAddress;
                    }
                    else if(getAddressMode()==MODE_OFFSET)
                    {
                        nCurrentAddress=i+nBlockOffset;
                    }

                    record.sAddress=XBinary::valueToHexColon(mode,nCurrentAddress);
                }

                g_listRecords.append(record);
            }
        }
        else
        {
            g_baDataBuffer.clear();
            g_baDataHexBuffer.clear();
        }

        setCurrentBlock(nBlockOffset,g_nDataBlockSize);
    }
}

void XHexView::paintCell(QPainter *pPainter,qint32 nRow,qint32 nColumn,qint32 nLeft,qint32 nTop,qint32 nWidth,qint32 nHeight)
{
    Q_UNUSED(nWidth)
//    g_pPainterText->drawRect(nLeft,nTop,nWidth,nHeight);
    if(nColumn==COLUMN_ADDRESS)
    {
        if(nRow<g_listRecords.count())
        {
//            pPainter->save();
//            pPainter->setPen(viewport()->palette().color(QPalette::Dark));
            pPainter->drawText(nLeft+getCharWidth(),nTop+nHeight,g_listRecords.at(nRow).sAddress); // TODO Text Optional
//            pPainter->restore();
        }
    }
    else if((nColumn==COLUMN_HEX)||(nColumn==COLUMN_SYMBOLS))
    {
        STATE state=getState();

        if(nRow*g_nBytesProLine<g_nDataBlockSize)
        {
            qint64 nDataBlockStartOffset=getViewStart();
            qint64 nDataBlockSize=qMin(g_nDataBlockSize-nRow*g_nBytesProLine,g_nBytesProLine);

            for(qint32 i=0;i<nDataBlockSize;i++)
            {
                qint32 nIndex=nRow*g_nBytesProLine+i;

                QString sHex;
                QString sSymbol;
                bool bBold=false;
                QRect rectSymbol;

                sHex=g_baDataHexBuffer.mid(nIndex*2,2);
                bBold=(sHex!="00");

                bool bSelected=isOffsetSelected(nDataBlockStartOffset+nIndex);
                bool bCursor=(state.nCursorOffset==(nDataBlockStartOffset+nIndex));

                if(bBold)
                {
                    pPainter->save();
                    QFont font=pPainter->font();
                    font.setBold(true);
                    pPainter->setFont(font);
                }

                if(nColumn==COLUMN_HEX)
                {
                    rectSymbol.setRect(nLeft+getCharWidth()+(i*2)*getCharWidth()+i*getLineDelta(),nTop,2*getCharWidth()+getLineDelta(),nHeight);

                    sSymbol=sHex;
                }
                else if(nColumn==COLUMN_SYMBOLS)
                {
                    rectSymbol.setRect(nLeft+(i+1)*getCharWidth(),nTop,getCharWidth(),nHeight);

                    if((getSmode()==SMODE_ANSI)||(getSmode()==SMODE_SYMBOLS))
                    {
                        QByteArray baChar=g_baDataBuffer.mid(nIndex,1); // TODO Check

                        if(baChar.size())
                        {
                            QChar cChar;

                            cChar=g_baDataBuffer.mid(nIndex,1).at(0); // TODO Check
                            sSymbol=filterSymbol(cChar,getSmode());
                        }
                    }
                    else if(getSmode()==SMODE_UNICODE)
                    {
                        if((nDataBlockStartOffset+nIndex)%2==0)
                        {
                            QByteArray baChar=g_baDataBuffer.mid(nIndex,2); // TODO Check

                            if(baChar.size()==2)
                            {
                                quint16 nCode=XBinary::_read_uint16(baChar.data());

                                QChar cChar(nCode);

                                sSymbol=filterSymbol(cChar,getSmode());
                            }
                        }
                    }
                }

                if(bSelected||bCursor)
                {
                    QRect rectSelected;
                    rectSelected.setRect(rectSymbol.x(),rectSymbol.y()+getLineDelta(),rectSymbol.width(),rectSymbol.height());

                    if(bCursor)
                    {
                        if(nColumn==state.cursorPosition.nColumn)
                        {
                            setCursorData(rectSelected,rectSelected,sSymbol,nIndex);
                        }
                    }

                    if(bSelected)
                    {
                        pPainter->fillRect(rectSelected,viewport()->palette().color(QPalette::Highlight));
                    }
                }

                pPainter->drawText(rectSymbol.x(),rectSymbol.y()+nHeight,sSymbol);

                if(bBold)
                {
                    pPainter->restore();
                }
            }
        }
    }
}

void XHexView::contextMenu(const QPoint &pos)
{
    QAction actionGoToOffset(tr("Go to offset"),this);
    actionGoToOffset.setShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_GOTOOFFSET));
    connect(&actionGoToOffset,SIGNAL(triggered()),this,SLOT(_goToOffsetSlot()));

    QAction actionGoToAddress(tr("Go to address"),this);
    actionGoToAddress.setShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_GOTOADDRESS));
    connect(&actionGoToAddress,SIGNAL(triggered()),this,SLOT(_goToAddressSlot()));

    QAction actionDumpToFile(tr("Dump to file"),this);
    actionDumpToFile.setShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_DUMPTOFILE));
    connect(&actionDumpToFile,SIGNAL(triggered()),this,SLOT(_dumpToFileSlot()));

    QAction actionSignature(tr("Signature"),this);
    actionSignature.setShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_SIGNATURE));
    connect(&actionSignature,SIGNAL(triggered()),this,SLOT(_hexSignatureSlot()));

    QAction actionFind(tr("Find"),this);
    actionFind.setShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_FIND));
    connect(&actionFind,SIGNAL(triggered()),this,SLOT(_findSlot()));

    QAction actionFindNext(tr("Find next"),this);
    actionFindNext.setShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_FINDNEXT));
    connect(&actionFindNext,SIGNAL(triggered()),this,SLOT(_findNextSlot()));

    QAction actionSelectAll(tr("Select all"),this);
    actionSelectAll.setShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_SELECTALL));
    connect(&actionSelectAll,SIGNAL(triggered()),this,SLOT(_selectAllSlot()));

    QAction actionCopyAsHex(tr("Data as hex"),this);
    actionCopyAsHex.setShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_COPYASHEX));
    connect(&actionCopyAsHex,SIGNAL(triggered()),this,SLOT(_copyAsHexSlot()));

    QAction actionCopyCursorOffset(tr("Offset"),this);
    actionCopyCursorOffset.setShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_COPYCURSOROFFSET));
    connect(&actionCopyCursorOffset,SIGNAL(triggered()),this,SLOT(_copyCursorOffsetSlot()));

    QAction actionCopyCursorAddress(tr("Address"),this);
    actionCopyCursorAddress.setShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_COPYCURSORADDRESS));
    connect(&actionCopyCursorAddress,SIGNAL(triggered()),this,SLOT(_copyCursorAddressSlot()));

    QAction actionDisasm(tr("Disasm"),this);
    actionDisasm.setShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_DISASM));
    connect(&actionDisasm,SIGNAL(triggered()),this,SLOT(_disasmSlot()));

    QAction actionMemoryMap(tr("Memory map"),this);
    actionMemoryMap.setShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_MEMORYMAP));
    connect(&actionMemoryMap,SIGNAL(triggered()),this,SLOT(_memoryMapSlot()));

    QAction actionEditHex(tr("Hex"),this);
    actionEditHex.setShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_EDITHEX));
    connect(&actionEditHex,SIGNAL(triggered()),this,SLOT(_editHex()));

    STATE state=getState();

    QMenu contextMenu(this);
    QMenu menuGoTo(tr("Go to"),this);
    QMenu menuSelect(tr("Select"),this);
    QMenu menuCopy(tr("Copy"),this);
    QMenu menuEdit(tr("Edit"),this);

    menuGoTo.addAction(&actionGoToOffset);
    menuGoTo.addAction(&actionGoToAddress);

    contextMenu.addMenu(&menuGoTo);

    contextMenu.addAction(&actionFind);
    contextMenu.addAction(&actionFindNext);
    menuCopy.addAction(&actionCopyCursorOffset);
    menuCopy.addAction(&actionCopyCursorAddress);

    if(state.nSelectionSize)
    {
        contextMenu.addAction(&actionDumpToFile);
        contextMenu.addAction(&actionSignature);

        menuCopy.addAction(&actionCopyAsHex);
    }

    if(g_options.bMenu_Disasm)
    {
        contextMenu.addAction(&actionDisasm);
    }

    if(g_options.bMenu_MemoryMap)
    {
        contextMenu.addAction(&actionMemoryMap);
    }

    contextMenu.addMenu(&menuCopy);

    menuEdit.setEnabled(!isReadonly());

    if(state.nSelectionSize)
    {
        menuEdit.addAction(&actionEditHex);

        contextMenu.addMenu(&menuEdit);
    }

    menuSelect.addAction(&actionSelectAll);
    contextMenu.addMenu(&menuSelect);

    // TODO reset select

    contextMenu.exec(pos);
}

void XHexView::wheelEvent(QWheelEvent *pEvent)
{
    if((g_nViewStartDelta)&&(pEvent->angleDelta().y()>0))
    {
        if(getScrollValue()==g_nViewStartDelta)
        {
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
    if( pEvent->matches(QKeySequence::MoveToNextChar)||
        pEvent->matches(QKeySequence::MoveToPreviousChar)||
        pEvent->matches(QKeySequence::MoveToNextLine)||
        pEvent->matches(QKeySequence::MoveToPreviousLine)||
        pEvent->matches(QKeySequence::MoveToStartOfLine)||
        pEvent->matches(QKeySequence::MoveToEndOfLine)||
        pEvent->matches(QKeySequence::MoveToNextPage)||
        pEvent->matches(QKeySequence::MoveToPreviousPage)||
        pEvent->matches(QKeySequence::MoveToStartOfDocument)||
        pEvent->matches(QKeySequence::MoveToEndOfDocument))
    {
        qint64 nViewStart=getViewStart();

        if(pEvent->matches(QKeySequence::MoveToNextChar))
        {
            setCursorOffset(getCursorOffset()+1);
        }
        else if(pEvent->matches(QKeySequence::MoveToPreviousChar))
        {
            setCursorOffset(getCursorOffset()-1);
        }
        else if(pEvent->matches(QKeySequence::MoveToNextLine))
        {
            setCursorOffset(getCursorOffset()+g_nBytesProLine);
        }
        else if(pEvent->matches(QKeySequence::MoveToPreviousLine))
        {
            setCursorOffset(getCursorOffset()-g_nBytesProLine);
        }
        else if(pEvent->matches(QKeySequence::MoveToStartOfLine))
        {
            setCursorOffset(getCursorOffset()-(getCursorDelta()%g_nBytesProLine));
        }
        else if(pEvent->matches(QKeySequence::MoveToEndOfLine))
        {
            setCursorOffset(getCursorOffset()-(getCursorDelta()%g_nBytesProLine)+g_nBytesProLine-1);
        }

        if((getCursorOffset()<0)||(pEvent->matches(QKeySequence::MoveToStartOfDocument)))
        {
            setCursorOffset(0);
            g_nViewStartDelta=0;
        }

        if((getCursorOffset()>=getDataSize())||(pEvent->matches(QKeySequence::MoveToEndOfDocument)))
        {
            setCursorOffset(getDataSize()-1);
            g_nViewStartDelta=0;
        }

        if( pEvent->matches(QKeySequence::MoveToNextChar)||
            pEvent->matches(QKeySequence::MoveToPreviousChar)||
            pEvent->matches(QKeySequence::MoveToNextLine)||
            pEvent->matches(QKeySequence::MoveToPreviousLine))
        {
            qint64 nRelOffset=getCursorOffset()-nViewStart;

            if(nRelOffset>=g_nBytesProLine*getLinesProPage())
            {
                _goToOffset(nViewStart+g_nBytesProLine,true);
            }
            else if(nRelOffset<0)
            {
                if(!_goToOffset(nViewStart-g_nBytesProLine,true))
                {
                    _goToOffset(0);
                }
            }
        }
        else if(pEvent->matches(QKeySequence::MoveToNextPage)||
                pEvent->matches(QKeySequence::MoveToPreviousPage))
        {
            if(pEvent->matches(QKeySequence::MoveToNextPage))
            {
                _goToOffset(nViewStart+g_nBytesProLine*getLinesProPage());
            }
            else if(pEvent->matches(QKeySequence::MoveToPreviousPage))
            {
                _goToOffset(nViewStart-g_nBytesProLine*getLinesProPage());
            }
        }
        else if(pEvent->matches(QKeySequence::MoveToStartOfDocument)||
                pEvent->matches(QKeySequence::MoveToEndOfDocument)) // TODO
        {
            _goToOffset(getCursorOffset());
        }

        adjust();
        viewport()->update();
    }
    else if(pEvent->matches(QKeySequence::SelectAll))
    {
        _selectAllSlot();
    }
    else
    {
        XAbstractTableView::keyPressEvent(pEvent);
    }
}

qint64 XHexView::getScrollValue()
{
    qint64 nResult=0;

    qint32 nValue=verticalScrollBar()->value();

    qint64 nMaxValue=getMaxScrollValue()*g_nBytesProLine;

    if(getDataSize()>nMaxValue)
    {
        if(nValue==getMaxScrollValue())
        {
            nResult=getDataSize()-g_nBytesProLine;
        }
        else
        {
            nResult=((double)nValue/(double)getMaxScrollValue())*getDataSize()+g_nViewStartDelta;
        }
    }
    else
    {
        nResult=(qint64)nValue*g_nBytesProLine+g_nViewStartDelta;
    }

    return nResult;
}

void XHexView::setScrollValue(qint64 nOffset)
{
    setViewStart(nOffset);
    g_nViewStartDelta=(nOffset)%g_nBytesProLine;

    qint32 nValue=0;

    if(getDataSize()>(getMaxScrollValue()*g_nBytesProLine))
    {
        if(nOffset==getDataSize()-g_nBytesProLine)
        {
            nValue=getMaxScrollValue();
        }
        else
        {
            nValue=((double)(nOffset-g_nViewStartDelta)/((double)getDataSize()))*(double)getMaxScrollValue();
        }
    }
    else
    {
        nValue=(nOffset)/g_nBytesProLine;
    }

    verticalScrollBar()->setValue(nValue);
}

void XHexView::adjustColumns()
{
    const QFontMetricsF fm(getTextFont());

    if(XBinary::getWidthModeFromSize(getStartAddress()+getDataSize())==XBinary::MODE_64)
    {
        g_nAddressWidth=16;
        setColumnWidth(COLUMN_ADDRESS,2*getCharWidth()+fm.boundingRect("00000000:00000000").width());
    }
    else
    {
        g_nAddressWidth=8;
        setColumnWidth(COLUMN_ADDRESS,2*getCharWidth()+fm.boundingRect("0000:0000").width());
    }

    setColumnWidth(COLUMN_HEX,g_nBytesProLine*2*getCharWidth()+2*getCharWidth()+getLineDelta()*g_nBytesProLine);
    setColumnWidth(COLUMN_SYMBOLS,(g_nBytesProLine+2)*getCharWidth());
}

void XHexView::registerShortcuts(bool bState)
{
    if(bState)
    {
        if(!shortCuts[SC_GOTOOFFSET])               shortCuts[SC_GOTOOFFSET]                =new QShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_GOTOOFFSET),              this,SLOT(_goToOffsetSlot()));
        if(!shortCuts[SC_GOTOADDRESS])              shortCuts[SC_GOTOADDRESS]               =new QShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_GOTOADDRESS),             this,SLOT(_goToAddressSlot()));
        if(!shortCuts[SC_DUMPTOFILE])               shortCuts[SC_DUMPTOFILE]                =new QShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_DUMPTOFILE),              this,SLOT(_dumpToFileSlot()));
        if(!shortCuts[SC_SELECTALL])                shortCuts[SC_SELECTALL]                 =new QShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_SELECTALL),               this,SLOT(_selectAllSlot()));
        if(!shortCuts[SC_COPYASHEX])                shortCuts[SC_COPYASHEX]                 =new QShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_COPYASHEX),               this,SLOT(_copyAsHexSlot()));
        if(!shortCuts[SC_COPYCURSOROFFSET])         shortCuts[SC_COPYCURSOROFFSET]          =new QShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_COPYCURSOROFFSET),        this,SLOT(_copyCursorOffsetSlot()));
        if(!shortCuts[SC_COPYCURSORADDRESS])        shortCuts[SC_COPYCURSORADDRESS]         =new QShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_COPYCURSORADDRESS),       this,SLOT(_copyCursorAddressSlot()));
        if(!shortCuts[SC_FIND])                     shortCuts[SC_FIND]                      =new QShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_FIND),                    this,SLOT(_findSlot()));
        if(!shortCuts[SC_FINDNEXT])                 shortCuts[SC_FINDNEXT]                  =new QShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_FINDNEXT),                this,SLOT(_findNextSlot()));
        if(!shortCuts[SC_SIGNATURE])                shortCuts[SC_SIGNATURE]                 =new QShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_SIGNATURE),               this,SLOT(_hexSignatureSlot()));
        if(!shortCuts[SC_DISASM])                   shortCuts[SC_DISASM]                    =new QShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_DISASM),                  this,SLOT(_disasmSlot()));
        if(!shortCuts[SC_MEMORYMAP])                shortCuts[SC_MEMORYMAP]                 =new QShortcut(getShortcuts()->getShortcut(XShortcuts::ID_HEX_MEMORYMAP),               this,SLOT(_memoryMapSlot()));
    }
    else
    {
        for(qint32 i=0;i<__SC_SIZE;i++)
        {
            if(shortCuts[i])
            {
                delete shortCuts[i];
                shortCuts[i]=nullptr;
            }
        }
    }
}

void XHexView::_headerClicked(qint32 nColumn)
{
    if(nColumn==COLUMN_ADDRESS)
    {
        if(getAddressMode()==MODE_ADDRESS)
        {
            setColumnTitle(COLUMN_ADDRESS,tr("Offset"));
            setAddressMode(MODE_OFFSET);
        }
        else if((getAddressMode()==MODE_OFFSET)||(getAddressMode()==MODE_THIS))
        {
            setColumnTitle(COLUMN_ADDRESS,tr("Address"));
            setAddressMode(MODE_ADDRESS);
        }

        adjust(true);
    }
    else if(nColumn==COLUMN_SYMBOLS)
    {
        if(getSmode()==SMODE_SYMBOLS)
        {
            setColumnTitle(COLUMN_SYMBOLS,QString("ANSI"));
            setSmode(SMODE_ANSI);
        }
        else if(getSmode()==SMODE_ANSI)
        {
            setColumnTitle(COLUMN_SYMBOLS,QString("Unicode"));
            setSmode(SMODE_UNICODE);
        }
        else if(getSmode()==SMODE_UNICODE)
        {
            setColumnTitle(COLUMN_SYMBOLS,tr("Symbols"));
            setSmode(SMODE_SYMBOLS);
        }

        adjust(true);
    }
}

void XHexView::_cellDoubleClicked(qint32 nRow,qint32 nColumn)
{
    if(nColumn==COLUMN_ADDRESS)
    {
        setColumnTitle(COLUMN_ADDRESS,"");
        setAddressMode(MODE_THIS);

        if(nRow<g_listRecords.count())
        {
            g_nThisBase=g_listRecords.at(nRow).nAddress;
        }

        adjust(true);
    }
}

XHexView::SMODE XHexView::getSmode()
{
    return g_smode;
}

void XHexView::setSmode(SMODE smode)
{
    g_smode=smode;
}

void XHexView::_disasmSlot()
{
    if(g_options.bMenu_Disasm)
    {
        STATE state=getState();

        emit showOffsetDisasm(state.nCursorOffset);
    }
}

void XHexView::_memoryMapSlot()
{
    if(g_options.bMenu_MemoryMap)
    {
        STATE state=getState();

        emit showOffsetMemoryMap(state.nCursorOffset);
    }
}

void XHexView::_editHex()
{
    STATE state=getState();

    SubDevice sd(getDevice(),state.nSelectionOffset,state.nSelectionSize);

    if(sd.open(QIODevice::ReadWrite))
    {
        DialogHexEdit dialogHexEdit(this);

        dialogHexEdit.setGlobal(getShortcuts(),getGlobalOptions());

//        connect(&dialogHexEdit,SIGNAL(changed()),this,SLOT(_setEdited()));

        dialogHexEdit.setData(&sd,state.nSelectionOffset);
        dialogHexEdit.setBackupDevice(getBackupDevice());

        dialogHexEdit.exec();

        _setEdited();

        sd.close();
    }
}
