// copyright (c) 2020 hors<horsicq@gmail.com>
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
#include "xhexview.h"

XHexView::XHexView(QWidget *pParent) : XAbstractTableView(pParent)
{
    g_pDevice=nullptr;

    g_nDataSize=0;
    g_nBytesProLine=16;
    g_nDataBlockSize=0;
    g_nViewStartDelta=0;
    g_searchData={};

    g_scGoToAddress   =new QShortcut(QKeySequence(XShortcuts::GOTOADDRESS),   this,SLOT(_goToAddress()));
    g_scDumpToFile    =new QShortcut(QKeySequence(XShortcuts::DUMPTOFILE),    this,SLOT(_dumpToFile()));
    g_scSelectAll     =new QShortcut(QKeySequence(XShortcuts::SELECTALL),     this,SLOT(_selectAll()));
    g_scCopyAsHex     =new QShortcut(QKeySequence(XShortcuts::COPYASHEX),     this,SLOT(_copyAsHex()));
    g_scFind          =new QShortcut(QKeySequence(XShortcuts::FIND),          this,SLOT(_find()));
    g_scFindNext      =new QShortcut(QKeySequence(XShortcuts::FINDNEXT),      this,SLOT(_findNext()));
    g_scSignature     =new QShortcut(QKeySequence(XShortcuts::SIGNATURE),     this,SLOT(_signature()));

#ifdef Q_OS_WIN
    setTextFont(QFont("Courier",10));
#endif
#ifdef Q_OS_LINUX
    setTextFont(QFont("Monospace",10));
#endif
#ifdef Q_OS_OSX
    setTextFont(QFont("Courier",10)); // TODO Check "Menlo"
#endif

    addColumn((8+2)*getCharWidth(),tr("Address"));          // COLUMN_ADDRESS
    addColumn((g_nBytesProLine*3+1)*getCharWidth(),"HEX");  // COLUMN_HEX
    addColumn((g_nBytesProLine+2)*getCharWidth(),"ANSI");   // COLUMN_SYMBOLS
}

void XHexView::setData(QIODevice *pDevice, XHexView::OPTIONS options)
{
    g_pDevice=pDevice;
    g_options=options;

    g_nDataSize=pDevice->size();

    qint64 nTotalLineCount=g_nDataSize/g_nBytesProLine;

    if(g_nDataSize%g_nBytesProLine==0)
    {
        nTotalLineCount--;
    }

    setTotalLineCount(nTotalLineCount);

    reload(true);
}

void XHexView::goToAddress(qint64 nAddress)
{
    goToOffset(nAddress-g_options.nStartAddress);
    // TODO reload
}

bool XHexView::isOffsetValid(qint64 nOffset)
{
    bool bResult=false;

    if((nOffset>=0)&&(nOffset<g_nDataSize))
    {
        bResult=true;
    }

    return bResult;
}

bool XHexView::isEnd(qint64 nOffset)
{
    return (nOffset==g_nDataSize);
}

qint64 XHexView::cursorPositionToOffset(XAbstractTableView::CURSOR_POSITION cursorPosition)
{
    qint64 nOffset=-1;

    if((cursorPosition.bIsValid)&&(cursorPosition.ptype==PT_CELL))
    {
        qint64 nBlockOffset=getViewStart()+(cursorPosition.nRow*g_nBytesProLine);

        if(cursorPosition.nColumn==COLUMN_ADDRESS)
        {
            nOffset=nBlockOffset;
        }
        else if(cursorPosition.nColumn==COLUMN_HEX)
        {
            nOffset=nBlockOffset+cursorPosition.nCellLeft/(getCharWidth()*3);
        }
        else if(cursorPosition.nColumn==COLUMN_SYMBOLS)
        {
            nOffset=nBlockOffset+cursorPosition.nCellLeft/getCharWidth();
        }

        if(!isOffsetValid(nOffset))
        {
            nOffset=g_nDataSize; // TODO Check
        }
    }

    return nOffset;
}

void XHexView::updateData()
{
    if(g_pDevice)
    {
        // Update cursor position
        qint64 nBlockOffset=getViewStart();
        qint64 nCursorOffset=nBlockOffset+getCursorDelta();

        if(nCursorOffset>=g_nDataSize)
        {
            nCursorOffset=g_nDataSize-1;
        }

        setCursorOffset(nCursorOffset);

        g_listAddresses.clear();

        if(g_pDevice->seek(nBlockOffset))
        {
            qint32 nDataBlockSize=g_nBytesProLine*getLinesProPage();

            g_baDataBuffer.resize(nDataBlockSize);
            g_nDataBlockSize=(int)g_pDevice->read(g_baDataBuffer.data(),nDataBlockSize);
            g_baDataBuffer.resize(g_nDataBlockSize);
            g_baDataHexBuffer=QByteArray(g_baDataBuffer.toHex());

            for(int i=0;i<g_nDataBlockSize;i+=g_nBytesProLine)
            {
                QString sAddress=QString("%1").arg(i+g_options.nStartAddress+nBlockOffset,8,16,QChar('0')); // TODO address width

                g_listAddresses.append(sAddress);
            }
        }
        else
        {
            g_baDataBuffer.clear();
            g_baDataHexBuffer.clear();
        }
    }
}

void XHexView::startPainting()
{

}

void XHexView::paintColumn(qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight)
{
    Q_UNUSED(nColumn)
    Q_UNUSED(nLeft)
    Q_UNUSED(nTop)
    Q_UNUSED(nWidth)
    Q_UNUSED(nHeight)
}

void XHexView::paintCell(qint32 nRow, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight)
{
//    g_pPainterText->drawRect(nLeft,nTop,nWidth,nHeight);
    if(nColumn==COLUMN_ADDRESS)
    {
        if(nRow<g_listAddresses.count())
        {
            getPainter()->drawText(nLeft+getCharWidth(),nTop+nHeight,g_listAddresses.at(nRow)); // TODO Text Optional
        }
    }
    else if((nColumn==COLUMN_HEX)||(nColumn==COLUMN_SYMBOLS))
    {
        STATE state=getState();

        if(nRow*g_nBytesProLine<g_nDataBlockSize)
        {
            qint64 nDataBlockStartOffset=getViewStart();
            qint64 nDataBlockSize=qMin(g_nDataBlockSize-nRow*g_nBytesProLine,g_nBytesProLine);

            for(int i=0;i<nDataBlockSize;i++)
            {
                qint32 nIndex=nRow*g_nBytesProLine+i;

                QString sHex=g_baDataHexBuffer.mid(nIndex*2,2);
                QString sSymbol;

                bool bBold=(sHex!="00");
                bool bSelected=isOffsetSelected(nDataBlockStartOffset+nIndex);
                bool bCursor=(state.nCursorOffset==(nDataBlockStartOffset+nIndex));

                if(bBold)
                {
                    getPainter()->save();
                    QFont font=getPainter()->font();
                    font.setBold(true);
                    getPainter()->setFont(font);
                }

                QRect rectSymbol;

                if(nColumn==COLUMN_HEX)
                {
                    rectSymbol.setRect(nLeft+(i*3+1)*getCharWidth(),nTop,3*getCharWidth(),nHeight);
                    sSymbol=sHex;
                }
                else if(nColumn==COLUMN_SYMBOLS)
                {
                    rectSymbol.setRect(nLeft+(i+1)*getCharWidth(),nTop,getCharWidth(),nHeight);
                    sSymbol=g_baDataBuffer.mid(nIndex,1); // TODO filter \n \r
                }

                if(bSelected||bCursor)
                {
                    QRect rectSelected;
                    rectSelected.setRect(rectSymbol.x(),rectSymbol.y()+getLineDelta(),rectSymbol.width(),rectSymbol.height());

                    if(bCursor)
                    {
                        if(nColumn==state.cursorPosition.nColumn)
                        {
                            setCursor(rectSelected,sSymbol,nIndex);
                        }
                    }

                    if(bSelected)
                    {
                        getPainter()->fillRect(rectSelected,viewport()->palette().color(QPalette::Highlight));
                    }
                }

                getPainter()->drawText(rectSymbol.x(),rectSymbol.y()+nHeight,sSymbol);

                if(bBold)
                {
                    getPainter()->restore();
                }
            }
        }
    }
}

void XHexView::endPainting()
{

}

void XHexView::contextMenu(const QPoint &pos)
{
    QAction actionGoToAddress(tr("Go to address"),this);
    actionGoToAddress.setShortcut(QKeySequence(XShortcuts::GOTOADDRESS));
    connect(&actionGoToAddress,SIGNAL(triggered()),this,SLOT(_goToAddress()));

    QAction actionDumpToFile(tr("Dump to file"),this);
    actionDumpToFile.setShortcut(QKeySequence(XShortcuts::DUMPTOFILE));
    connect(&actionDumpToFile,SIGNAL(triggered()),this,SLOT(_dumpToFile()));

    QAction actionSignature(tr("Signature"),this);
    actionSignature.setShortcut(QKeySequence(XShortcuts::SIGNATURE));
    connect(&actionSignature,SIGNAL(triggered()),this,SLOT(_signature()));

    QAction actionFind(tr("Find"),this);
    actionFind.setShortcut(QKeySequence(XShortcuts::FIND));
    connect(&actionFind,SIGNAL(triggered()),this,SLOT(_find()));

    QAction actionFindNext(tr("Find next"),this);
    actionFindNext.setShortcut(QKeySequence(XShortcuts::FINDNEXT));
    connect(&actionFindNext,SIGNAL(triggered()),this,SLOT(_findNext()));

    QAction actionSelectAll(tr("Select all"),this);
    actionSelectAll.setShortcut(QKeySequence(XShortcuts::SELECTALL));
    connect(&actionSelectAll,SIGNAL(triggered()),this,SLOT(_selectAll()));

    QAction actionCopyAsHex(tr("Copy as hex"),this);
    actionCopyAsHex.setShortcut(QKeySequence(XShortcuts::COPYASHEX));
    connect(&actionCopyAsHex,SIGNAL(triggered()),this,SLOT(_copyAsHex()));

    QMenu contextMenu(this);
    QMenu menuSelect(tr("Select"),this);
    QMenu menuCopy(tr("Copy"),this);

    contextMenu.addAction(&actionGoToAddress);
    contextMenu.addAction(&actionFind);
    contextMenu.addAction(&actionFindNext);

    STATE state=getState();

    if(state.nSelectionSize)
    {
        contextMenu.addAction(&actionDumpToFile);
        contextMenu.addAction(&actionSignature);

        menuCopy.addAction(&actionCopyAsHex);
        contextMenu.addMenu(&menuCopy);
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

        if((getCursorOffset()>=g_nDataSize)||(pEvent->matches(QKeySequence::MoveToEndOfDocument)))
        {
            setCursorOffset(g_nDataSize-1);
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
                goToOffset(nViewStart+g_nBytesProLine);
            }
            else if(nRelOffset<0)
            {
                if(!goToOffset(nViewStart-g_nBytesProLine))
                {
                    goToOffset(0);
                }
            }
        }
        else if(pEvent->matches(QKeySequence::MoveToNextPage)||
                pEvent->matches(QKeySequence::MoveToPreviousPage))
        {
            if(pEvent->matches(QKeySequence::MoveToNextPage))
            {
                goToOffset(nViewStart+g_nBytesProLine*getLinesProPage());
            }
            else if(pEvent->matches(QKeySequence::MoveToPreviousPage))
            {
                goToOffset(nViewStart-g_nBytesProLine*getLinesProPage());
            }
        }
        else if(pEvent->matches(QKeySequence::MoveToStartOfDocument)||
                pEvent->matches(QKeySequence::MoveToEndOfDocument)) // TODO
        {
            goToOffset(getCursorOffset());
        }

        adjust();
        viewport()->update();
    }
    else if(pEvent->matches(QKeySequence::SelectAll))
    {
        _selectAll();
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

    if(g_nDataSize>nMaxValue)
    {
        if(nValue==getMaxScrollValue())
        {
            nResult=g_nDataSize-g_nBytesProLine;
        }
        else
        {
            nResult=((double)nValue/(double)getMaxScrollValue())*g_nDataSize+g_nViewStartDelta;
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

    if(g_nDataSize>(getMaxScrollValue()*g_nBytesProLine))
    {
        if(nOffset==g_nDataSize-g_nBytesProLine)
        {
            nValue=getMaxScrollValue();
        }
        else
        {
            nValue=((double)(nOffset-g_nViewStartDelta)/((double)g_nDataSize))*(double)getMaxScrollValue();
        }
    }
    else
    {
        nValue=(nOffset)/g_nBytesProLine;
    }

    verticalScrollBar()->setValue(nValue);

    adjust(true);
}

void XHexView::_goToAddress()
{
    DialogGoToAddress da(this,g_options.nStartAddress,g_options.nStartAddress+g_nDataSize,DialogGoToAddress::TYPE_ADDRESS);
    if(da.exec()==QDialog::Accepted)
    {
        goToAddress(da.getValue());
        setFocus();
        viewport()->update();
    }
}

void XHexView::_dumpToFile()
{
    QString sFilter;
    sFilter+=QString("%1 (*.bin)").arg(tr("Raw data"));
    QString sSaveFileName="dump.bin"; // TODO a function
    QString sFileName=QFileDialog::getSaveFileName(this,tr("Save dump"),sSaveFileName,sFilter);

    if(!sFileName.isEmpty())
    {
        STATE state=getState();

        DialogDumpProcess dd(this,g_pDevice,state.nSelectionOffset,state.nSelectionSize,sFileName,DumpProcess::DT_OFFSET);

        dd.exec();
    }
}

void XHexView::_signature()
{
    STATE state=getState();

    DialogHexSignature dsh(this,g_pDevice,state.nSelectionOffset,state.nSelectionSize);

    dsh.exec();
}

void XHexView::_find()
{
    STATE state=getState();

    g_searchData={};
    g_searchData.nResult=-1;
    g_searchData.nCurrentOffset=state.nCursorOffset;

    DialogSearch dialogSearch(this,g_pDevice,&g_searchData);

    if(dialogSearch.exec()==QDialog::Accepted)
    {
        goToOffset(g_searchData.nResult);
        setFocus();
        viewport()->update();
    }
}

void XHexView::_findNext()
{
    if(g_searchData.bInit)
    {
        g_searchData.nCurrentOffset=g_searchData.nResult+1;
        g_searchData.startFrom=SearchProcess::SF_CURRENTOFFSET;

        DialogSearchProcess dialogSearch(this,g_pDevice,&g_searchData);

        if(dialogSearch.exec()==QDialog::Accepted)
        {
            goToOffset(g_searchData.nResult);
            setFocus();
            viewport()->update();
        }
    }
}

void XHexView::_selectAll()
{
    setSelection(0,g_nDataSize);
}

void XHexView::_copyAsHex()
{
    STATE state=getState();

    qint64 nSize=qMin(state.nSelectionSize,(qint64)0x10000);

    QByteArray baData=XBinary::read_array(g_pDevice,state.nSelectionOffset,nSize);

    QApplication::clipboard()->setText(baData.toHex());
}
