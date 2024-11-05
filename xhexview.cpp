/* Copyright (c) 2020-2024 hors<horsicq@gmail.com>
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
    g_nBytesProLine = 16;  // Default
    _setMode(MODE_HEX);
    g_nDataBlockSize = 0;
    g_nViewStartDelta = 0;
    //    g_smode=SMODE_ANSI;  // TODO Set/Get
    g_nThisBase = 0;
    g_hexOptions = {};
    g_nAddressWidth = 8;         // TODO Set/Get
    g_bIsLocationColon = false;  // TODO Check
                                 //    g_nPieceSize=1; // TODO
    memset(g_shortCuts, 0, sizeof g_shortCuts);

    addColumn(tr("Address"), 0, true);
    addColumn(tr("Hex"), 0, true);
    addColumn(tr("Symbols"), 0, true);

    setTextFont(XOptions::getMonoFont());  // mb TODO move to XDeviceTableView !!!
                                           //    setBlinkingCursorEnable(true);
    // setBlinkingCursorEnable(false);
    g_sCodePage = "";
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
    g_pCodePageMenu = g_xCodePageOptions.createCodePagesMenu(this, true);
    connect(&g_xCodePageOptions, SIGNAL(setCodePage(QString)), this, SLOT(_setCodePage(QString)));
#endif
    setLocationMode(LOCMODE_OFFSET);
    setMapEnable(true);
    setMapWidth(20);

    // g_pixmapCache.setCacheLimit(1024);
    setVerticalLinesVisible(false);
}

void XHexView::adjustView()
{
    setTextFontFromOptions(XOptions::ID_HEX_FONT);

    g_bIsLocationColon = getGlobalOptions()->getValue(XOptions::ID_HEX_LOCATIONCOLON).toBool();

    viewport()->update();
}

void XHexView::_adjustView()
{
    adjustView();

    if (getDevice()) {
        reload(true);
    }
}

void XHexView::setData(QIODevice *pDevice, const OPTIONS &options, bool bReload)
{
    g_hexOptions = options;

    setDevice(pDevice);

    adjustView();
    adjustMap();

    XBinary binary(pDevice, true, options.nStartAddress);
    XBinary::_MEMORY_MAP memoryMap = binary.getMemoryMap();

    setMemoryMap(memoryMap);

    //    resetCursorData();

    setLocationMode(options.addressMode);

    adjustHeader();

    adjustColumns();
    adjustScrollCount();

    if ((options.nStartSelectionOffset > 0) && (options.nStartSelectionOffset != -1)) {
        _goToViewPos(options.nStartSelectionOffset);
    }

    _initSetSelection(options.nStartSelectionOffset, options.nSizeOfSelection);
    //    setCursorViewPos(options.nStartSelectionOffset, COLUMN_HEX);

    if (bReload) {
        reload(true);
    }
}

void XHexView::goToAddress(XADDR nAddress)
{
    qint64 nViewPos = deviceOffsetToViewPos(nAddress - g_hexOptions.nStartAddress);
    _goToViewPos(nViewPos);
    // TODO reload
}

void XHexView::goToOffset(qint64 nOffset)
{
    qint64 nViewPos = deviceOffsetToViewPos(nOffset);
    _goToViewPos(nViewPos);
}

XADDR XHexView::getStartAddress()
{
    return g_hexOptions.nStartAddress;
}

XADDR XHexView::getSelectionInitAddress()
{
    return getSelectionInitOffset() + g_hexOptions.nStartAddress;
}

XHexView::SHOWRECORD XHexView::_getShowRecordByOffset(qint64 nOffset)
{
    SHOWRECORD result = {};

    qint32 nNumberOfRecords = g_listShowRecords.count();

    for (qint32 i = 0; i < nNumberOfRecords; i++) {
        if ((g_listShowRecords.at(i).nViewPos != -1) && (g_listShowRecords.at(i).nViewPos <= nOffset) &&
            (nOffset < (g_listShowRecords.at(i).nViewPos + g_listShowRecords.at(i).nSize))) {
            result = g_listShowRecords.at(i);
            break;
        }
    }

    return result;
}

XAbstractTableView::OS XHexView::cursorPositionToOS(const XAbstractTableView::CURSOR_POSITION &cursorPosition)
{
    OS osResult = {};

    osResult.nViewPos = -1;

    if ((cursorPosition.bIsValid) && (cursorPosition.ptype == PT_CELL)) {
        qint64 nBlockOffset = getViewPosStart() + (cursorPosition.nRow * g_nBytesProLine);

        if (cursorPosition.nColumn == COLUMN_LOCATION) {
            osResult.nViewPos = nBlockOffset;
            //            osResult.nSize=g_nPieceSize;
            osResult.nSize = 1;
        } else if (cursorPosition.nColumn == COLUMN_ELEMENTS) {
            osResult.nViewPos = nBlockOffset + (cursorPosition.nAreaLeft - getSideDelta() - getCharWidth()) / (getCharWidth() * g_nSymbolsProElement + getSideDelta());
            //            osResult.nSize=g_nPieceSize;
            osResult.nSize = 1;
        } else if (cursorPosition.nColumn == COLUMN_SYMBOLS) {
            osResult.nViewPos = nBlockOffset + (cursorPosition.nAreaLeft - getSideDelta() - getCharWidth()) / getCharWidth();
            //            osResult.nSize=g_nPieceSize;
            osResult.nSize = 1;
        }

        //        osResult.nOffset=S_ALIGN_DOWN(osResult.nOffset,g_nPieceSize);

        if (isViewPosValid(osResult.nViewPos)) {
            SHOWRECORD showRecord = _getShowRecordByOffset(osResult.nViewPos);

            if (showRecord.nSize) {
                osResult.nViewPos = showRecord.nViewPos;
                osResult.nSize = showRecord.nSize;
            }

        } else {
            osResult.nViewPos = getViewSize();  // TODO Check
            osResult.nSize = 0;
        }

        //        qDebug("nBlockOffset %x",nBlockOffset);
        //        qDebug("cursorPosition.nCellLeft %x",cursorPosition.nCellLeft);
        //        qDebug("getCharWidth() %x",getCharWidth());
        //        qDebug("nOffset %x",osResult.nOffset);
    } else if ((cursorPosition.bIsValid) && (cursorPosition.ptype == PT_MAP)) {
        osResult.nViewPos = XBinary::align_down((getViewSize() * cursorPosition.nPercentage) / getMapCount(), g_nBytesProLine);
        osResult.nSize = 0;
    }
    return osResult;
}

void XHexView::updateData()
{
    QIODevice *_pDevice = getDevice();

    if (_pDevice) {
        // Update cursor position
        qint64 nDataBlockStartOffset = getViewPosStart();  // TODO Check
        quint64 nInitLocation = 0;

        XIODevice *pSubDevice = dynamic_cast<XIODevice *>(_pDevice);

        if (pSubDevice) {
            nInitLocation = pSubDevice->getInitLocation();
        }

        //        qint64 nCursorOffset = nBlockStartLine + getCursorDelta();

        //        if (nCursorOffset >= getViewSize()) {
        //            nCursorOffset = getViewSize() - 1;
        //        }

        //        setCursorViewPos(nCursorOffset);

        XBinary::MODE mode = XBinary::getWidthModeFromByteSize(g_nAddressWidth);

        g_listLocationRecords.clear();
        g_listShowRecords.clear();

        qint32 nDataBlockSize = g_nBytesProLine * getLinesProPage();

        g_listHighlightsRegion.clear();
        if (getXInfoDB()) {
            QList<XInfoDB::BOOKMARKRECORD> listBookMarks = getXInfoDB()->getBookmarkRecords(nDataBlockStartOffset + nInitLocation, XBinary::LT_OFFSET, nDataBlockSize);
            g_listHighlightsRegion.append(_convertBookmarksToHighlightRegion(&listBookMarks));
        }

        g_baDataBuffer = read_array(nDataBlockStartOffset, nDataBlockSize);
        // QList<QChar> listElements = getStringBuffer(&g_baDataBuffer);


        // qint32 nNumberOfElements = listElements.count();

        g_nDataBlockSize = g_baDataBuffer.size();

        if (g_nDataBlockSize) {
            QString sDataHexBuffer;

            if ((g_mode == MODE_HEX) || (g_mode == MODE_BYTE)) {
                sDataHexBuffer = QByteArray(g_baDataBuffer.toHex());
            }

            // Locations
            for (qint32 i = 0; i < g_nDataBlockSize; i += g_nBytesProLine) {
                XADDR nCurrentLocation = 0;

                LOCATIONRECORD record = {};
                record.nLocation = i + g_hexOptions.nStartAddress + nDataBlockStartOffset;

                if (getlocationMode() == LOCMODE_THIS) {
                    nCurrentLocation = record.nLocation;

                    qint64 nDelta = (qint64)nCurrentLocation - (qint64)g_nThisBase;

                    record.sLocation = XBinary::thisToString(nDelta);
                } else {
                    if (getlocationMode() == LOCMODE_ADDRESS) {
                        nCurrentLocation = record.nLocation;
                    } else if (getlocationMode() == LOCMODE_OFFSET) {
                        nCurrentLocation = i + nDataBlockStartOffset;
                    }

                    if (g_bIsLocationColon) {
                        record.sLocation = XBinary::valueToHexColon(mode, nCurrentLocation);
                    } else {
                        record.sLocation = XBinary::valueToHex(mode, nCurrentLocation);
                    }
                }

                g_listLocationRecords.append(record);
            }

            // Elements
            char *pData = g_baDataBuffer.data();
            qint32 nCurrentRowOffset = 0;
            qint32 nRow = 0;
            bool bFirst = true;

            for (qint32 i = 0; i < g_nDataBlockSize; ) {
                SHOWRECORD record = {};

                record.nViewPos = nDataBlockStartOffset + i;
                record.nSize = 1;
                record.nRowOffset = nCurrentRowOffset;
                record.nRow = nRow;

                if (bFirst) {
                    record.bFirstRowSymbol = true;
                    bFirst = false;
                }

                if (g_mode == MODE_HEX) {
                    if (g_sCodePage == "") {
                        QChar _char = g_baDataBuffer.at(i);

                        if (!_char.isPrint()) {
                            _char = '.';
                        }

                        record.sSymbol = _char;
                    } else {
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
                        if (g_pCodec) {
                            qint32 nJmax = qMin(g_nDataBlockSize - i, (qint32)8);

                            for (int j = 0; j < nJmax; j++) {
                                QTextCodec::ConverterState converterState = {};
                                record.nSize = j + 1;
                                record.sSymbol = g_pCodec->toUnicode(pData + i, record.nSize, &converterState);

                                if (converterState.remainingChars == 0) {
                                    break;
                                }
                            }
                        }
#endif
                    }

                    record.sElement = sDataHexBuffer.mid(i * 2, 2 * record.nSize);
                }else if (g_mode == MODE_BYTE) {
                    record.sElement = sDataHexBuffer.mid(i * 2, 2);  // g_nSymbolsProElement
                } else if (g_mode == MODE_UINT8) {
                    record.sElement = QString::number(XBinary::_read_uint8(pData + i));
                } else if (g_mode == MODE_INT8) {
                    record.sElement = QString::number(XBinary::_read_int8(pData + i));
                }

                // if (i < nNumberOfElements) {
                //     record.sSymbol = listElements.at(i);
                // }

                if (record.nSize == 1) {
                    record.bIsBold = (g_baDataBuffer.at(i) != 0);  // TODO optimize !!! TODO Different rules
                }

                QList<HIGHLIGHTREGION> listHighLightRegions = getHighlightRegion(&g_listHighlightsRegion, nDataBlockStartOffset + i + nInitLocation, XBinary::LT_OFFSET);

                if (listHighLightRegions.count()) {
                    record.bIsHighlighted = true;
                    record.colBackground = listHighLightRegions.at(0).colBackground;
                    record.colBackgroundSelected = listHighLightRegions.at(0).colBackgroundSelected;
                } else {
                    record.colBackgroundSelected = getColor(TCLOLOR_SELECTED);
                }

                //                record.bIsSelected = isViewPosSelected(nDataBlockStartOffset + i);

                i += record.nSize;
                nCurrentRowOffset += record.nSize;

                if (nCurrentRowOffset >= g_nBytesProLine) {
                    nCurrentRowOffset -= g_nBytesProLine;
                    nRow++;
                    record.bLastRowSymbol = true;
                    bFirst = true;
                } else if (nCurrentRowOffset >= getViewSize()) {
                    record.bLastRowSymbol = true;
                }

                g_listShowRecords.append(record);
            }
        } else {
            g_baDataBuffer.clear();
        }

        setCurrentBlock(nDataBlockStartOffset, g_nDataBlockSize);

        g_pixmapCache.clear();
    }
}

void XHexView::paintMap(QPainter *pPainter, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight)
{
    pPainter->save();

    QString sKey = "Map_0_0";
    sKey += QString("_%1").arg(nWidth);
    sKey += QString("_%1").arg(nHeight);

    QPixmap _pixmap(0, 0);

    if (g_pixmapCache.find(sKey, &_pixmap)) {
        //        if (false) {
        pPainter->drawPixmap(nLeft, nTop, nWidth, nHeight, _pixmap);
    } else {
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        qreal ratio = QPaintDevice::devicePixelRatioF();
#else
        qreal ratio = QPaintDevice::devicePixelRatio();
#endif
        // qint32 nPartCount = qMin(nHeight, (qint32)nHeight);
        // qint32 nPartSize = getDevice()->size() / nPartCount;

        // XBinary::_MEMORY_MAP *pMemoryMap = getMemoryMap();

        // TODO memoryMap tooltips

        QPixmap pixmap(nWidth * ratio, nHeight * ratio);
        pixmap.setDevicePixelRatio(ratio);
        pixmap.fill(Qt::transparent);

        QPainter painterPixmap(&pixmap);
        // painterPixmap.fillRect(0, 0, nWidth, nHeight, QBrush(Qt::darkYellow));

        g_pixmapCache.insert(sKey, pixmap);

        pPainter->drawPixmap(nLeft, nTop, nWidth, nHeight, pixmap);
    }

    pPainter->restore();
}

void XHexView::paintCell(QPainter *pPainter, qint32 nRow, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight)
{
    // #ifdef QT_DEBUG
    //     QElapsedTimer timer;
    //     timer.start();
    // #endif
    //     g_pPainterText->drawRect(nLeft,nTop,nWidth,nHeight);

    if (nColumn == COLUMN_LOCATION) {
        //        if (nRow < g_listLocationRecords.count()) {
        //            QRect rectSymbol;

        //            rectSymbol.setLeft(nLeft + getCharWidth());
        //            rectSymbol.setTop(nTop + getLineDelta());
        //            rectSymbol.setWidth(nWidth);
        //            rectSymbol.setHeight(nHeight - getLineDelta());

        //            //            pPainter->save();
        //            //            pPainter->setPen(viewport()->palette().color(QPalette::Dark));
        //            pPainter->drawText(rectSymbol, g_listLocationRecords.at(nRow).sLocation);  // TODO Text Optional
        //                                                                              //            pPainter->restore();
        //        }
    } else if ((nColumn == COLUMN_ELEMENTS) || (nColumn == COLUMN_SYMBOLS)) {
        qint32 nNumberOfShowRecords = g_listShowRecords.count();

        for (qint32 i = 0; i < nNumberOfShowRecords; i++) {
            if (g_listShowRecords.at(i).nRow == nRow) {
                SHOWRECORD record = g_listShowRecords.at(i);

                bool bIsSelected = isViewPosSelected(record.nViewPos);
                bool bIsSelectedPrev = false;
                bool bIsSelectedNext = false;

                if (i - 1 >= 0) {
                    bIsSelectedPrev = isViewPosSelected(g_listShowRecords.at(i - 1).nViewPos);
                }

                if (i + 1 < nNumberOfShowRecords) {
                    bIsSelectedNext = isViewPosSelected(g_listShowRecords.at(i + 1).nViewPos);
                }

                QRect rectSymbol;

                if (bIsSelected) {
                    if (nColumn == COLUMN_ELEMENTS) {
                        rectSymbol.setLeft(nLeft + getCharWidth() + (record.nRowOffset * (g_nSymbolsProElement * getCharWidth() + getSideDelta())));
                        rectSymbol.setTop(nTop + getLineDelta());
                        rectSymbol.setHeight(nHeight - getLineDelta());

                        int nWidth = record.nSize * (g_nSymbolsProElement * getCharWidth() + getSideDelta());

                        if ((record.bLastRowSymbol) || (bIsSelected && (!bIsSelectedNext))) {
                            nWidth -= getSideDelta();
                        }

                        rectSymbol.setWidth(nWidth);
                    } else if (nColumn == COLUMN_SYMBOLS) {
                        rectSymbol.setLeft(nLeft + (record.nRowOffset + 1) * getCharWidth());
                        rectSymbol.setTop(nTop + getLineDelta());
                        rectSymbol.setWidth(getCharWidth() * record.nSize);
                        rectSymbol.setHeight(nHeight - getLineDelta());
                    }

                    if (rectSymbol.left() < (nLeft + nWidth)) {  // Paint Only visible
                        if (record.bIsBold) {
                            pPainter->save();
                            QFont font = pPainter->font();
                            font.setBold(true);
                            pPainter->setFont(font);
                        }

                        QString sSymbol;

                        if (nColumn == COLUMN_ELEMENTS) {
                            sSymbol = record.sElement;
                        } else if (nColumn == COLUMN_SYMBOLS) {
                            sSymbol = record.sSymbol;
                        }

                        if (bIsSelected) {
                            pPainter->fillRect(rectSymbol, record.colBackgroundSelected);
                        }

                        if (bIsSelected) {
                            // Draw lines
                            bool bTop = false;
                            bool bLeft = false;
                            bool bBottom = false;
                            bool bRight = false;

                            if ((record.bFirstRowSymbol) || (!bIsSelectedPrev)) {
                                bLeft = true;
                            }

                            if ((record.bLastRowSymbol) || (!bIsSelectedNext)) {
                                bRight = true;
                            }

                            if (!isViewPosSelected(record.nViewPos - g_nBytesProLine)) {
                                bTop = true;
                            }

                            if (!isViewPosSelected(record.nViewPos + g_nBytesProLine)) {
                                bBottom = true;
                            }

                            if (bTop) {
                                pPainter->drawLine(rectSymbol.left(), rectSymbol.top(), rectSymbol.right(), rectSymbol.top());
                            }

                            if (bLeft) {
                                pPainter->drawLine(rectSymbol.left(), rectSymbol.top(), rectSymbol.left(), rectSymbol.bottom());
                            }

                            if (bBottom) {
                                pPainter->drawLine(rectSymbol.left(), rectSymbol.bottom(), rectSymbol.right(), rectSymbol.bottom());
                            }

                            if (bRight) {
                                pPainter->drawLine(rectSymbol.right(), rectSymbol.top(), rectSymbol.right(), rectSymbol.bottom());
                            }

                            if (record.bIsBold) {
                                pPainter->restore();
                            }
                        } else {
                            break;  // Do not paint invisible
                        }
                    }
                }
            }
        }
    }
    // #ifdef QT_DEBUG
    //     qDebug("XHexView::paintCell %lld",timer.elapsed());
    // #endif
}

void XHexView::paintColumn(QPainter *pPainter, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight)
{
#ifdef QT_DEBUG
//    qDebug("XHexView::paintColumn");
//    QElapsedTimer timer;
//    timer.start();
#endif

    QString sKey;

    if (nColumn == COLUMN_LOCATION) {
        sKey = QString("location");
    } else if (nColumn == COLUMN_ELEMENTS) {
        sKey = QString("elements");
    } else if (nColumn == COLUMN_SYMBOLS) {
        sKey = QString("symbols");
    }

    if (sKey != "") {
        sKey += QString("_%1").arg(getViewPosStart());
        sKey += QString("_%1").arg(getViewSize());
        sKey += QString("_%1").arg(nWidth);
        sKey += QString("_%1").arg(nHeight);

        QPixmap _pixmap(0, 0);

        if (g_pixmapCache.find(sKey, &_pixmap)) {
            // qDebug("g_pixmapCache");
            pPainter->drawPixmap(nLeft, nTop, nWidth, nHeight, _pixmap);
        } else {
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
            qreal ratio = QPaintDevice::devicePixelRatioF();
#else
            qreal ratio = QPaintDevice::devicePixelRatio();
#endif
            QPixmap pixmap(nWidth * ratio, nHeight * ratio);
            pixmap.setDevicePixelRatio(ratio);
            pixmap.fill(Qt::transparent);

            QPainter painterPixmap(&pixmap);
            painterPixmap.setFont(pPainter->font());
            painterPixmap.setBackgroundMode(Qt::TransparentMode);

            if (nColumn == COLUMN_LOCATION) {
                qint32 nNumberOfRows = g_listLocationRecords.count();

                for (qint32 i = 0; i < nNumberOfRows; i++) {
                    QRect rectSymbol;
                    rectSymbol.setLeft(getCharWidth());
                    rectSymbol.setTop(getLineHeight() * i + getLineDelta());
                    rectSymbol.setWidth(nWidth);
                    rectSymbol.setHeight(getLineHeight() - getLineDelta());

                    painterPixmap.drawText(rectSymbol, g_listLocationRecords.at(i).sLocation);  // TODO Text Optional pPainter->restore();
                }
            } else if ((nColumn == COLUMN_ELEMENTS) || (nColumn == COLUMN_SYMBOLS)) {
                QFont fontBold = painterPixmap.font();
                fontBold.setBold(true);

                qint32 nNumberOfShowRecords= g_listShowRecords.count();

                for (int i = 0; i < nNumberOfShowRecords; i++) {
                    SHOWRECORD record = g_listShowRecords.at(i);

                    bool bIsHighlighted = g_listShowRecords.at(i).bIsHighlighted;
                    bool bIsHighlightedNext = false;

                    if (i + 1 < nNumberOfShowRecords) {
                        bIsHighlightedNext = g_listShowRecords.at(i + 1).bIsHighlighted;
                    }

                    QRect rectSymbol;

                    if (nColumn == COLUMN_ELEMENTS) {
                        rectSymbol.setLeft(getCharWidth() + (record.nRowOffset * (g_nSymbolsProElement * getCharWidth() + getSideDelta())));
                        rectSymbol.setTop(getLineHeight() * record.nRow + getLineDelta());
                        rectSymbol.setHeight(getLineHeight() - getLineDelta());

                        int nWidth = record.nSize * (g_nSymbolsProElement * getCharWidth() + getSideDelta());

                        if ((record.bLastRowSymbol) || (bIsHighlighted && (!bIsHighlightedNext))) {
                            nWidth -= getSideDelta();
                        }

                        rectSymbol.setWidth(nWidth);
                    } else if (nColumn == COLUMN_SYMBOLS) {
                        rectSymbol.setLeft((record.nRowOffset + 1) * getCharWidth());
                        rectSymbol.setTop(getLineHeight() * record.nRow + getLineDelta());
                        rectSymbol.setWidth(getCharWidth() * record.nSize);
                        rectSymbol.setHeight(getLineHeight() - getLineDelta());
                    }

                    if (record.bIsBold) {
                        painterPixmap.save();
                        painterPixmap.setFont(fontBold);
                    }

                    QString sSymbol;

                    if (nColumn == COLUMN_ELEMENTS) {
                        sSymbol = record.sElement;
                    } else if (nColumn == COLUMN_SYMBOLS) {
                        sSymbol = record.sSymbol;
                    }

                    if (bIsHighlighted) {
                        painterPixmap.fillRect(rectSymbol, record.colBackground);
                    }

                    if (nColumn == COLUMN_ELEMENTS) {
                        painterPixmap.drawText(rectSymbol, sSymbol, Qt::AlignVCenter | Qt::AlignHCenter);
                    } else if (nColumn == COLUMN_SYMBOLS) {
                        if (sSymbol != "") {
                            painterPixmap.drawText(rectSymbol, sSymbol);
                        }
                    }

                    if (record.bIsBold) {
                        painterPixmap.restore();
                    }
                }
            }

            g_pixmapCache.insert(sKey, pixmap);

            pPainter->drawPixmap(nLeft, nTop, nWidth, nHeight, pixmap);
        }
    }

#ifdef QT_DEBUG
//    qDebug("Elapsed XHexView::paintColumn %lld", timer.elapsed());
#endif
}

void XHexView::paintTitle(QPainter *pPainter, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight, const QString &sTitle)
{
    if (nColumn == COLUMN_ELEMENTS) {
        for (qint8 i = 0; i < g_nBytesProLine; i++) {
            QString sSymbol = XBinary::valueToHex(i);

            QRect rectSymbol;

            rectSymbol.setLeft(nLeft + getCharWidth() + (i * g_nSymbolsProElement) * getCharWidth() + i * getSideDelta());
            rectSymbol.setTop(nTop);
            rectSymbol.setWidth(g_nSymbolsProElement * getCharWidth() + getSideDelta());
            rectSymbol.setHeight(nHeight);

            if ((rectSymbol.left()) < (nLeft + nWidth)) {
                pPainter->drawText(rectSymbol, Qt::AlignVCenter | Qt::AlignHCenter, sSymbol);
            }
        }
    } else {
        XAbstractTableView::paintTitle(pPainter, nColumn, nLeft, nTop, nWidth, nHeight, sTitle);
    }
}

void XHexView::contextMenu(const QPoint &pos)
{
    if (isContextMenuEnable()) {
        STATE menuState = getState();

        QMenu contextMenu(this);

        QList<XShortcuts::MENUITEM> listMenuItems;

        if (menuState.nSelectionViewSize) {
            getShortcuts()->_addMenuItem_Checked(&listMenuItems, X_ID_HEX_DATAINSPECTOR, this, SLOT(_showDataInspector()), XShortcuts::GROUPID_NONE,
                                                 getViewWidgetState(VIEWWIDGET_DATAINSPECTOR));
            getShortcuts()->_addMenuItem_Checked(&listMenuItems, X_ID_HEX_DATACONVERTOR, this, SLOT(_showDataConvertor()), XShortcuts::GROUPID_NONE,
                                                 getViewWidgetState(VIEWWIDGET_DATACONVERTOR));
            getShortcuts()->_addMenuSeparator(&listMenuItems, XShortcuts::GROUPID_NONE);
        }

        getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_GOTO_OFFSET, this, SLOT(_goToOffsetSlot()), XShortcuts::GROUPID_GOTO);
        getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_GOTO_ADDRESS, this, SLOT(_goToAddressSlot()), XShortcuts::GROUPID_GOTO);

        if (menuState.nSelectionViewSize) {
            getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_GOTO_SELECTION_START, this, SLOT(_goToSelectionStart()),
                                         (XShortcuts::GROUPID_SELECTION << 8) | XShortcuts::GROUPID_GOTO);
            getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_GOTO_SELECTION_END, this, SLOT(_goToSelectionEnd()),
                                         (XShortcuts::GROUPID_SELECTION << 8) | XShortcuts::GROUPID_GOTO);
        }

        getShortcuts()->_addMenuItem_Checked(&listMenuItems, X_ID_HEX_MULTISEARCH, this, SLOT(_showMultisearch()), XShortcuts::GROUPID_NONE,
                                             getViewWidgetState(VIEWWIDGET_MULTISEARCH));

        if (menuState.nSelectionViewSize) {
            getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_DUMPTOFILE, this, SLOT(_dumpToFileSlot()), XShortcuts::GROUPID_NONE);
            getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_SIGNATURE, this, SLOT(_hexSignatureSlot()), XShortcuts::GROUPID_NONE);
        }

        getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_FIND_STRING, this, SLOT(_findStringSlot()), XShortcuts::GROUPID_FIND);
        getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_FIND_SIGNATURE, this, SLOT(_findSignatureSlot()), XShortcuts::GROUPID_FIND);
        getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_FIND_VALUE, this, SLOT(_findValueSlot()), XShortcuts::GROUPID_FIND);
        getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_FIND_NEXT, this, SLOT(_findNextSlot()), XShortcuts::GROUPID_FIND);

        getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_SELECT_ALL, this, SLOT(_selectAllSlot()), XShortcuts::GROUPID_SELECT);

        getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_COPY_OFFSET, this, SLOT(_copyOffsetSlot()), XShortcuts::GROUPID_COPY);
        getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_COPY_ADDRESS, this, SLOT(_copyAddressSlot()), XShortcuts::GROUPID_COPY);
        getShortcuts()->_addMenuSeparator(&listMenuItems, XShortcuts::GROUPID_COPY);
        getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_COPY_DATA, this, SLOT(_copyDataSlot()), XShortcuts::GROUPID_COPY);

        getShortcuts()->_addMenuItem_Checked(&listMenuItems, X_ID_HEX_STRINGS, this, SLOT(_strings()), XShortcuts::GROUPID_NONE, getViewWidgetState(VIEWWIDGET_STRINGS));
        getShortcuts()->_addMenuItem_Checked(&listMenuItems, X_ID_HEX_VISUALIZATION, this, SLOT(_visualization()), XShortcuts::GROUPID_NONE,
                                             getViewWidgetState(VIEWWIDGET_VISUALIZATION));

#ifdef QT_SQL_LIB
        getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_BOOKMARKS_NEW, this, SLOT(_bookmarkNew()), XShortcuts::GROUPID_BOOKMARKS);
        getShortcuts()->_addMenuItem_Checked(&listMenuItems, X_ID_HEX_BOOKMARKS_LIST, this, SLOT(_bookmarkList()), XShortcuts::GROUPID_BOOKMARKS,
                                             getViewWidgetState(VIEWWIDGET_BOOKMARKS));
#endif

        if (g_hexOptions.bMenu_Disasm) {
            getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_FOLLOWIN_DISASM, this, SLOT(_disasmSlot()), XShortcuts::GROUPID_FOLLOWIN);
        }

        if (g_hexOptions.bMenu_MemoryMap) {
            getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_FOLLOWIN_MEMORYMAP, this, SLOT(_memoryMapSlot()), XShortcuts::GROUPID_FOLLOWIN);
        }

        if (g_hexOptions.bMenu_MainHex) {
            getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_FOLLOWIN_HEX, this, SLOT(_mainHexSlot()), XShortcuts::GROUPID_FOLLOWIN);
        }

        if (!isReadonly()) {
            if (menuState.nSelectionViewSize) {
                getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_EDIT_HEX, this, SLOT(_editHex()), XShortcuts::GROUPID_EDIT);
            }
            getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_EDIT_PATCH, this, SLOT(_editPatch()), XShortcuts::GROUPID_EDIT);

            if (XBinary::isResizeEnable(getDevice())) {
                getShortcuts()->_addMenuSeparator(&listMenuItems, XShortcuts::GROUPID_EDIT);
                getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_EDIT_REMOVE, this, SLOT(_editRemove()), XShortcuts::GROUPID_EDIT);
                getShortcuts()->_addMenuItem(&listMenuItems, X_ID_HEX_EDIT_RESIZE, this, SLOT(_editResize()), XShortcuts::GROUPID_EDIT);
            }
        }

        QList<QObject *> listObjects = getShortcuts()->adjustContextMenu(&contextMenu, &listMenuItems);

        contextMenu.exec(pos);

        XOptions::deleteQObjectList(&listObjects);

        return;

        // TODO
#if defined(QT_SCRIPT_LIB) || defined(QT_QML_LIB)
        QAction actionScripts(this);
        getShortcuts()->adjustAction(&contextMenu, &actionScripts, X_ID_HEX_SCRIPTS, this, SLOT(_scripts()));
        if (getViewWidgetState(VIEWWIDGET_SCRIPTS)) {
            actionScripts.setCheckable(true);
            actionScripts.setChecked(true);
        }
#endif
        // TODO reset select
        contextMenu.exec(pos);
    }
}

void XHexView::wheelEvent(QWheelEvent *pEvent)
{
    if ((g_nViewStartDelta) && (pEvent->angleDelta().y() > 0)) {
        if (getCurrentViewPosFromScroll() == g_nViewStartDelta) {
            setCurrentViewPosToScroll(0);
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
        STATE state = getState();
        qint64 nViewStart = getViewPosStart();

        state.nSelectionViewSize = 1;

        if (pEvent->matches(QKeySequence::MoveToNextChar)) {
            state.nSelectionViewPos++;
        } else if (pEvent->matches(QKeySequence::MoveToPreviousChar)) {
            state.nSelectionViewPos--;
        } else if (pEvent->matches(QKeySequence::MoveToNextLine)) {
            state.nSelectionViewPos += g_nBytesProLine;
        } else if (pEvent->matches(QKeySequence::MoveToPreviousLine)) {
            state.nSelectionViewPos -= g_nBytesProLine;
        } else if (pEvent->matches(QKeySequence::MoveToStartOfLine)) {
            // TODO
        } else if (pEvent->matches(QKeySequence::MoveToEndOfLine)) {
            // TODO
        }

        if ((state.nSelectionViewPos < 0) || (pEvent->matches(QKeySequence::MoveToStartOfDocument))) {
            state.nSelectionViewPos = 0;
            g_nViewStartDelta = 0;
        }

        if ((state.nSelectionViewPos >= getViewSize()) || (pEvent->matches(QKeySequence::MoveToEndOfDocument))) {
            state.nSelectionViewPos = getViewSize() - 1;
            g_nViewStartDelta = 0;
        }

        setState(state);

        if (pEvent->matches(QKeySequence::MoveToNextChar) || pEvent->matches(QKeySequence::MoveToPreviousChar) || pEvent->matches(QKeySequence::MoveToNextLine) ||
            pEvent->matches(QKeySequence::MoveToPreviousLine)) {
            qint64 nRelOffset = state.nSelectionViewPos - nViewStart;

            if (nRelOffset >= g_nBytesProLine * getLinesProPage()) {
                _goToViewPos(nViewStart + g_nBytesProLine, true);
            } else if (nRelOffset < 0) {
                if (!_goToViewPos(nViewStart - g_nBytesProLine, true)) {
                    _goToViewPos(0);
                }
            }
        } else if (pEvent->matches(QKeySequence::MoveToNextPage) || pEvent->matches(QKeySequence::MoveToPreviousPage)) {
            if (pEvent->matches(QKeySequence::MoveToNextPage)) {
                _goToViewPos(nViewStart + g_nBytesProLine * getLinesProPage());
            } else if (pEvent->matches(QKeySequence::MoveToPreviousPage)) {
                _goToViewPos(nViewStart - g_nBytesProLine * getLinesProPage());
            }
        } else if (pEvent->matches(QKeySequence::MoveToStartOfDocument) || pEvent->matches(QKeySequence::MoveToEndOfDocument))  // TODO
        {
            _goToViewPos(state.nSelectionViewPos);
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

qint64 XHexView::getCurrentViewPosFromScroll()
{
    qint64 nResult = 0;

    qint32 nValue = verticalScrollBar()->value();

    qint64 nMaxValue = getMaxScrollValue() * g_nBytesProLine;

    if (getViewSize() > nMaxValue) {
        if (nValue == getMaxScrollValue()) {
            nResult = getViewSize() - g_nBytesProLine;
        } else {
            nResult = ((double)nValue / (double)getMaxScrollValue()) * getViewSize() + g_nViewStartDelta;
        }
    } else {
        nResult = (qint64)nValue * g_nBytesProLine + g_nViewStartDelta;
    }

    return nResult;
}

void XHexView::setCurrentViewPosToScroll(qint64 nOffset)
{
    setViewPosStart(nOffset);
    g_nViewStartDelta = (nOffset) % g_nBytesProLine;

    qint32 nValue = 0;

    if (getViewSize() > (getMaxScrollValue() * g_nBytesProLine)) {
        if (nOffset == getViewSize() - g_nBytesProLine) {
            nValue = getMaxScrollValue();
        } else {
            nValue = ((double)(nOffset - g_nViewStartDelta) / ((double)getViewSize())) * (double)getMaxScrollValue();
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

    if (XBinary::getWidthModeFromSize(getStartAddress() + getViewSize()) == XBinary::MODE_64) {
        g_nAddressWidth = 16;
        setColumnWidth(COLUMN_LOCATION, 2 * getCharWidth() + fm.boundingRect("00000000:00000000").width());
    } else {
        g_nAddressWidth = 8;
        setColumnWidth(COLUMN_LOCATION, 2 * getCharWidth() + fm.boundingRect("0000:0000").width());
    }

    setColumnWidth(COLUMN_ELEMENTS, g_nBytesProLine * g_nSymbolsProElement * getCharWidth() + 2 * getCharWidth() + getSideDelta() * g_nBytesProLine);
    setColumnWidth(COLUMN_SYMBOLS, (g_nBytesProLine + 2) * getCharWidth());
}

void XHexView::registerShortcuts(bool bState)
{
    if (bState) {
        if (!g_shortCuts[SC_DATAINSPECTOR])
            g_shortCuts[SC_DATAINSPECTOR] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_DATAINSPECTOR), this, SLOT(_showDataInspector()));
        if (!g_shortCuts[SC_DATACONVERTOR])
            g_shortCuts[SC_DATACONVERTOR] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_DATACONVERTOR), this, SLOT(_showDataConvertor()));
        if (!g_shortCuts[SC_MULTISEARCH]) g_shortCuts[SC_MULTISEARCH] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_MULTISEARCH), this, SLOT(_showMultisearch()));
        if (!g_shortCuts[SC_GOTO_OFFSET]) g_shortCuts[SC_GOTO_OFFSET] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_GOTO_OFFSET), this, SLOT(_goToOffsetSlot()));
        if (!g_shortCuts[SC_GOTO_ADDRESS])
            g_shortCuts[SC_GOTO_ADDRESS] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_GOTO_ADDRESS), this, SLOT(_goToAddressSlot()));
        if (!g_shortCuts[SC_DUMPTOFILE]) g_shortCuts[SC_DUMPTOFILE] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_DUMPTOFILE), this, SLOT(_dumpToFileSlot()));
        if (!g_shortCuts[SC_SELECTALL]) g_shortCuts[SC_SELECTALL] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_SELECT_ALL), this, SLOT(_selectAllSlot()));
        if (!g_shortCuts[SC_COPYASDATA]) g_shortCuts[SC_COPYASDATA] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_COPY_DATA), this, SLOT(_copyDataSlot()));
        if (!g_shortCuts[SC_COPYOFFSET]) g_shortCuts[SC_COPYOFFSET] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_COPY_OFFSET), this, SLOT(_copyOffsetSlot()));
        if (!g_shortCuts[SC_COPYADDRESS]) g_shortCuts[SC_COPYADDRESS] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_COPY_ADDRESS), this, SLOT(_copyAddressSlot()));
        if (!g_shortCuts[SC_FINDSTRING]) g_shortCuts[SC_FINDSTRING] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_FIND_STRING), this, SLOT(_findStringSlot()));
        if (!g_shortCuts[SC_FINDSIGNATURE])
            g_shortCuts[SC_FINDSIGNATURE] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_FIND_SIGNATURE), this, SLOT(_findSignatureSlot()));
        if (!g_shortCuts[SC_FINDVALUE]) g_shortCuts[SC_FINDVALUE] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_FIND_VALUE), this, SLOT(_findValueSlot()));
        if (!g_shortCuts[SC_FINDNEXT]) g_shortCuts[SC_FINDNEXT] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_FIND_NEXT), this, SLOT(_findNextSlot()));
        if (!g_shortCuts[SC_SIGNATURE]) g_shortCuts[SC_SIGNATURE] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_SIGNATURE), this, SLOT(_hexSignatureSlot()));
        if (!g_shortCuts[SC_DISASM]) g_shortCuts[SC_DISASM] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_FOLLOWIN_DISASM), this, SLOT(_disasmSlot()));
        if (!g_shortCuts[SC_MEMORYMAP]) g_shortCuts[SC_MEMORYMAP] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_FOLLOWIN_MEMORYMAP), this, SLOT(_memoryMapSlot()));
        if (!g_shortCuts[SC_MAINHEX]) g_shortCuts[SC_MAINHEX] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_FOLLOWIN_HEX), this, SLOT(_mainHexSlot()));
        if (!g_shortCuts[SC_EDIT_HEX]) g_shortCuts[SC_EDIT_HEX] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_EDIT_HEX), this, SLOT(_editHex()));
        if (!g_shortCuts[SC_EDIT_REMOVE]) g_shortCuts[SC_EDIT_REMOVE] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_EDIT_REMOVE), this, SLOT(_editRemove()));
        if (!g_shortCuts[SC_EDIT_RESIZE]) g_shortCuts[SC_EDIT_RESIZE] = new QShortcut(getShortcuts()->getShortcut(X_ID_HEX_EDIT_RESIZE), this, SLOT(_editResize()));
    } else {
        for (qint32 i = 0; i < __SC_SIZE; i++) {
            if (g_shortCuts[i]) {
                delete g_shortCuts[i];
                g_shortCuts[i] = nullptr;
            }
        }
    }
}

void XHexView::adjustHeader()
{
    if (getlocationMode() == LOCMODE_ADDRESS) {
        setColumnTitle(COLUMN_LOCATION, tr("Address"));
    } else if ((getlocationMode() == LOCMODE_OFFSET) || (getlocationMode() == LOCMODE_THIS)) {
        setColumnTitle(COLUMN_LOCATION, tr("Offset"));
    }
}

void XHexView::_headerClicked(qint32 nColumn)
{
    if (nColumn == COLUMN_LOCATION) {
        // // TODO Context Menu with
        // if (getAddressMode() == LOCMODE_ADDRESS) {
        //     setColumnTitle(COLUMN_ADDRESS, tr("Offset"));
        //     setAddressMode(LOCMODE_OFFSET);
        // } else if ((getAddressMode() == LOCMODE_OFFSET) || (getAddressMode() == LOCMODE_THIS)) {
        //     setColumnTitle(COLUMN_ADDRESS, tr("Address"));
        //     setAddressMode(LOCMODE_ADDRESS);
        // }
        QMenu contextMenu(this);  // TODO

        QList<XShortcuts::MENUITEM> listMenuItems;

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = tr("Address");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeLocationView());
            menuItem.nSubgroups = XShortcuts::GROUPID_LOCATION;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = getlocationMode() == LOCMODE_ADDRESS;
            menuItem.sPropertyName = "location";
            menuItem.varProperty = LOCMODE_ADDRESS;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = tr("Offset");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeLocationView());
            menuItem.nSubgroups = XShortcuts::GROUPID_LOCATION;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = getlocationMode() == LOCMODE_OFFSET;
            menuItem.sPropertyName = "location";
            menuItem.varProperty = LOCMODE_OFFSET;

            listMenuItems.append(menuItem);
        }

        QList<QObject *> listObjects = getShortcuts()->adjustContextMenu(&contextMenu, &listMenuItems);

        contextMenu.exec(QCursor::pos());

        XOptions::deleteQObjectList(&listObjects);

        // adjust(true);
    } else if (nColumn == COLUMN_ELEMENTS) {
        QMenu contextMenu(this);  // TODO

        QMenu menuMode(tr("Mode"), this);

        QAction actionHex(QString("Hex"), this);
        actionHex.setProperty("mode", MODE_HEX);
        actionHex.setCheckable(true);
        actionHex.setChecked(g_mode == MODE_HEX);
        connect(&actionHex, SIGNAL(triggered()), this, SLOT(changeModeView()));
        menuMode.addAction(&actionHex);

        menuMode.addSeparator();

        QAction actionByte(QString("byte"), this);
        actionByte.setProperty("mode", MODE_BYTE);
        actionByte.setCheckable(true);
        actionByte.setChecked(g_mode == MODE_BYTE);
        connect(&actionByte, SIGNAL(triggered()), this, SLOT(changeModeView()));
        menuMode.addAction(&actionByte);

        menuMode.addSeparator();

        QAction actionUint8(QString("uint8"), this);
        actionUint8.setProperty("mode", MODE_UINT8);
        actionUint8.setCheckable(true);
        actionUint8.setChecked(g_mode == MODE_UINT8);
        connect(&actionUint8, SIGNAL(triggered()), this, SLOT(changeModeView()));
        menuMode.addAction(&actionUint8);

        QAction actionInt8(QString("int8"), this);
        actionInt8.setProperty("mode", MODE_INT8);
        actionInt8.setCheckable(true);
        actionInt8.setChecked(g_mode == MODE_INT8);
        connect(&actionInt8, SIGNAL(triggered()), this, SLOT(changeModeView()));
        menuMode.addAction(&actionInt8);

        QMenu menuWidth(tr("Width"), this);

        QAction action8(QString("8"), this);
        action8.setProperty("width", 8);
        action8.setCheckable(true);
        action8.setChecked(g_nBytesProLine == 8);
        connect(&action8, SIGNAL(triggered()), this, SLOT(changeWidth()));
        menuWidth.addAction(&action8);
        QAction action16(QString("16"), this);
        action16.setProperty("width", 16);
        action16.setCheckable(true);
        action16.setChecked(g_nBytesProLine == 16);
        connect(&action16, SIGNAL(triggered()), this, SLOT(changeWidth()));
        menuWidth.addAction(&action16);
        QAction action32(QString("32"), this);
        action32.setProperty("width", 32);
        action32.setCheckable(true);
        action32.setChecked(g_nBytesProLine == 32);
        connect(&action32, SIGNAL(triggered()), this, SLOT(changeWidth()));
        menuWidth.addAction(&action32);

        contextMenu.addMenu(&menuMode);
        contextMenu.addMenu(&menuWidth);

        contextMenu.exec(QCursor::pos());
    } else if (nColumn == COLUMN_SYMBOLS) {
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
        g_pCodePageMenu->exec(QCursor::pos());
#endif
    }

    XAbstractTableView::_headerClicked(nColumn);
}

void XHexView::_cellDoubleClicked(qint32 nRow, qint32 nColumn)
{
    if (nColumn == COLUMN_LOCATION) {
        setColumnTitle(COLUMN_LOCATION, "");
        setLocationMode(LOCMODE_THIS);

        if (nRow < g_listLocationRecords.count()) {
            g_nThisBase = g_listLocationRecords.at(nRow).nLocation;
        }

        adjust(true);
    }
}

void XHexView::adjustScrollCount()
{
    if (getDevice()) {
        setViewSize(getDevice()->size());
    }

    qint64 nTotalLineCount = getViewSize() / g_nBytesProLine;

    if (getViewSize() % g_nBytesProLine == 0) {
        nTotalLineCount--;
    }

    //    if((getDataSize()>0)&&(getDataSize()<g_nBytesProLine))
    //    {
    //        nTotalLineCount=1;
    //    }

    setTotalScrollCount(nTotalLineCount);
}

void XHexView::adjustMap()
{
    if (isMapEnable()) {
        if (getDevice()) {
            qint64 nNumberOfLines = getDevice()->size() / g_nBytesProLine;
            if (nNumberOfLines > 100) {
                nNumberOfLines = 100;
            } else if (nNumberOfLines == 0) {
                nNumberOfLines = 1;
            }

            setMapCount((qint32)nNumberOfLines);
        }
    }
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
    if (g_hexOptions.bMenu_Disasm) {
        emit showOffsetDisasm(getDeviceState(true).nSelectionDeviceOffset);
    }
}

void XHexView::_memoryMapSlot()
{
    if (g_hexOptions.bMenu_MemoryMap) {
        emit showOffsetMemoryMap(getDeviceState(true).nSelectionDeviceOffset);
    }
}

void XHexView::_mainHexSlot()
{
    if (g_hexOptions.bMenu_MainHex) {
        DEVICESTATE deviceState = getDeviceState(true);
        emit showOffsetMainHex(deviceState.nSelectionDeviceOffset, deviceState.nSelectionSize);
    }
}

void XHexView::_setCodePage(const QString &sCodePage)
{
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
    g_sCodePage = sCodePage;

    QString sTitle = tr("Symbols");

    if (g_sCodePage != "") {
        sTitle = g_sCodePage;
        g_pCodec = QTextCodec::codecForName(g_sCodePage.toLatin1().data());
    }

    setColumnTitle(COLUMN_SYMBOLS, sTitle);

    adjust(true);
#else
    Q_UNUSED(sCodePage)
#endif
}

void XHexView::changeWidth()
{
    QAction *pAction = qobject_cast<QAction *>(sender());

    if (pAction) {
        g_nBytesProLine = pAction->property("width").toUInt();

        adjustMap();
        adjustScrollCount();
        adjustColumns();
        adjust(true);
    }
}

void XHexView::changeModeView()
{
    QAction *pAction = qobject_cast<QAction *>(sender());

    if (pAction) {
        MODE mode = (MODE)pAction->property("mode").toUInt();

        _setMode(mode);

        adjustColumns();
        adjust(true);
    }
}

void XHexView::_setMode(MODE mode)
{
    g_mode = mode;

    if ((mode == MODE_BYTE) || (mode == MODE_HEX)) {
        g_nSymbolsProElement = 2;
    } else if (mode == MODE_UINT8) {
        g_nSymbolsProElement = 3;
    } else if (mode == MODE_INT8) {
        g_nSymbolsProElement = 4;
    }

    setColumnEnabled(COLUMN_SYMBOLS, (mode == MODE_HEX));

    // TODO make g_nSymbolsProElement make dynamic if UTF8
}
