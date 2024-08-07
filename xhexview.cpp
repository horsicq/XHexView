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
    _setMode(MODE_BYTE);
    g_nDataBlockSize = 0;
    g_nViewStartDelta = 0;
    //    g_smode=SMODE_ANSI;  // TODO Set/Get
    g_nThisBase = 0;
    g_hexOptions = {};
    g_nAddressWidth = 8;        // TODO Set/Get
    g_bIsAddressColon = false;  // TODO Check
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
    setAddressMode(LOCMODE_OFFSET);

    // g_pixmapCache.setCacheLimit(1024);
    setVerticalLinesVisible(false);
}

void XHexView::adjustView()
{
    setTextFontFromOptions(XOptions::ID_HEX_FONT);

    g_bIsAddressColon = getGlobalOptions()->getValue(XOptions::ID_HEX_ADDRESSCOLON).toBool();

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

    XBinary binary(pDevice, true, options.nStartAddress);
    XBinary::_MEMORY_MAP memoryMap = binary.getMemoryMap();

    setMemoryMap(memoryMap);

    //    resetCursorData();

    setAddressMode(options.addressMode);

    adjustHeader();

    adjustColumns();
    adjustScrollCount();

    if ((options.nStartSelectionOffset > 0) && (options.nStartSelectionOffset != -1)) {
        _goToViewOffset(options.nStartSelectionOffset);
    }

    _initSetSelection(options.nStartSelectionOffset, options.nSizeOfSelection);
    //    setCursorViewOffset(options.nStartSelectionOffset, COLUMN_HEX);

    if (bReload) {
        reload(true);
    }
}

void XHexView::goToAddress(XADDR nAddress)
{
    qint64 nViewOffset = deviceOffsetToViewOffset(nAddress - g_hexOptions.nStartAddress);
    _goToViewOffset(nViewOffset);
    // TODO reload
}

void XHexView::goToOffset(qint64 nOffset)
{
    qint64 nViewOffset = deviceOffsetToViewOffset(nOffset);
    _goToViewOffset(nViewOffset);
}

XADDR XHexView::getStartAddress()
{
    return g_hexOptions.nStartAddress;
}

XADDR XHexView::getSelectionInitAddress()
{
    return getSelectionInitOffset() + g_hexOptions.nStartAddress;
}

XAbstractTableView::OS XHexView::cursorPositionToOS(XAbstractTableView::CURSOR_POSITION cursorPosition)
{
    OS osResult = {};

    osResult.nViewOffset = -1;

    if ((cursorPosition.bIsValid) && (cursorPosition.ptype == PT_CELL)) {
        qint64 nBlockOffset = getViewOffsetStart() + (cursorPosition.nRow * g_nBytesProLine);

        if (cursorPosition.nColumn == COLUMN_ADDRESS) {
            osResult.nViewOffset = nBlockOffset;
            //            osResult.nSize=g_nPieceSize;
            osResult.nSize = 1;
        } else if (cursorPosition.nColumn == COLUMN_ELEMENTS) {
            osResult.nViewOffset = nBlockOffset + (cursorPosition.nCellLeft - getSideDelta() - getCharWidth()) / (getCharWidth() * g_nSymbolsProElement + getSideDelta());
            //            osResult.nSize=g_nPieceSize;
            osResult.nSize = 1;
        } else if (cursorPosition.nColumn == COLUMN_SYMBOLS) {
            osResult.nViewOffset = nBlockOffset + (cursorPosition.nCellLeft - getSideDelta() - getCharWidth()) / getCharWidth();
            //            osResult.nSize=g_nPieceSize;
            osResult.nSize = 1;
        }

        //        osResult.nOffset=S_ALIGN_DOWN(osResult.nOffset,g_nPieceSize);

        if (!isViewOffsetValid(osResult.nViewOffset)) {
            osResult.nViewOffset = getViewSize();  // TODO Check
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
    QIODevice *_pDevice = getDevice();

    if (_pDevice) {
        // Update cursor position
        qint64 nDataBlockStartOffset = getViewOffsetStart();  // TODO Check
        quint64 nInitLocation = 0;

        XIODevice *pSubDevice = dynamic_cast<XIODevice *>(_pDevice);

        if (pSubDevice) {
            nInitLocation = pSubDevice->getInitLocation();
        }

        //        qint64 nCursorOffset = nBlockStartLine + getCursorDelta();

        //        if (nCursorOffset >= getViewSize()) {
        //            nCursorOffset = getViewSize() - 1;
        //        }

        //        setCursorViewOffset(nCursorOffset);

        XBinary::MODE mode = XBinary::getWidthModeFromByteSize(g_nAddressWidth);

        g_listLocationRecords.clear();
        g_listByteRecords.clear();

        qint32 nDataBlockSize = g_nBytesProLine * getLinesProPage();

        g_listHighlightsRegion.clear();
        if (getXInfoDB()) {
            QList<XInfoDB::BOOKMARKRECORD> listBookMarks = getXInfoDB()->getBookmarkRecords(nDataBlockStartOffset + nInitLocation, XBinary::LT_OFFSET, nDataBlockSize);
            g_listHighlightsRegion.append(_convertBookmarksToHighlightRegion(&listBookMarks));
        }

        g_baDataBuffer = read_array(nDataBlockStartOffset, nDataBlockSize);
        QList<QChar> listElements = getStringBuffer(&g_baDataBuffer);

        g_nDataBlockSize = g_baDataBuffer.size();

        if (g_nDataBlockSize) {
            QString sDataHexBuffer;

            if (g_mode == MODE_BYTE) {
                sDataHexBuffer = QByteArray(g_baDataBuffer.toHex());
            }

            for (qint32 i = 0; i < g_nDataBlockSize; i += g_nBytesProLine) {
                XADDR nCurrentAddress = 0;

                LOCATIONRECORD record = {};
                record.nLocation = i + g_hexOptions.nStartAddress + nDataBlockStartOffset;

                if (getAddressMode() == LOCMODE_THIS) {
                    nCurrentAddress = record.nLocation;

                    qint64 nDelta = (qint64)nCurrentAddress - (qint64)g_nThisBase;

                    record.sLocation = XBinary::thisToString(nDelta);
                } else {
                    if (getAddressMode() == LOCMODE_ADDRESS) {
                        nCurrentAddress = record.nLocation;
                    } else if (getAddressMode() == LOCMODE_OFFSET) {
                        nCurrentAddress = i + nDataBlockStartOffset;
                    }

                    if (g_bIsAddressColon) {
                        record.sLocation = XBinary::valueToHexColon(mode, nCurrentAddress);
                    } else {
                        record.sLocation = XBinary::valueToHex(mode, nCurrentAddress);
                    }
                }

                g_listLocationRecords.append(record);
            }

            for (qint32 i = 0; i < g_nDataBlockSize; i++) {
                BYTERECORD record = {};

                if (g_mode == MODE_BYTE) {
                    record.sElement = sDataHexBuffer.mid(i * 2, 2);  // g_nSymbolsProElement
                } else if (g_mode == MODE_UINT8) {
                    record.sElement = QString::number(XBinary::_read_uint8(g_baDataBuffer.data() + i));
                } else if (g_mode == MODE_INT8) {
                    record.sElement = QString::number(XBinary::_read_int8(g_baDataBuffer.data() + i));
                }

                record.sSymbol = listElements.at(i);
                record.bIsBold = (g_baDataBuffer.at(i) != 0);  // TODO optimize !!!

                QList<HIGHLIGHTREGION> listHighLightRegions = getHighlightRegion(&g_listHighlightsRegion, nDataBlockStartOffset + i + nInitLocation, XBinary::LT_OFFSET);

                if (listHighLightRegions.count()) {
                    record.bIsHighlighted = true;
                    record.colBackground = listHighLightRegions.at(0).colBackground;
                    record.colBackgroundSelected = listHighLightRegions.at(0).colBackgroundSelected;
                } else {
                    record.colBackgroundSelected = getColor(TCLOLOR_SELECTED);
                }

                //                record.bIsSelected = isViewOffsetSelected(nDataBlockStartOffset + i);

                g_listByteRecords.append(record);
            }
        } else {
            g_baDataBuffer.clear();
        }

        setCurrentBlock(nDataBlockStartOffset, g_nDataBlockSize);

        g_pixmapCache.clear();
    }
}

void XHexView::paintCell(QPainter *pPainter, qint32 nRow, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight)
{
    // #ifdef QT_DEBUG
    //     QElapsedTimer timer;
    //     timer.start();
    // #endif
    //     g_pPainterText->drawRect(nLeft,nTop,nWidth,nHeight);

    if (nColumn == COLUMN_ADDRESS) {
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
        //        STATE state = getState();
        if (nRow * g_nBytesProLine < g_nDataBlockSize) {
            qint64 nDataBlockStartOffset = getViewOffsetStart();
            qint64 nDataBlockSize = qMin(g_nDataBlockSize - nRow * g_nBytesProLine, g_nBytesProLine);

            for (qint32 i = 0; i < nDataBlockSize; i++) {
                qint32 nIndex = nRow * g_nBytesProLine + i;
                qint64 nCurrent = nDataBlockStartOffset + nIndex;
                //                bool bIsHighlighted = g_listByteRecords.at(nIndex).bIsHighlighted;
                //                bool bIsHighlightedNext = false;

                //                if (nIndex + 1 < g_nDataBlockSize) {
                //                    bIsHighlightedNext = g_listByteRecords.at(nIndex + 1).bIsHighlighted;
                //                }

                bool bIsSelected = isViewOffsetSelected(nCurrent);
                bool bIsSelectedNext = isViewOffsetSelected(nCurrent + 1);

                QRect rectSymbol;

                if (nColumn == COLUMN_ELEMENTS) {
                    rectSymbol.setLeft(nLeft + getCharWidth() + (i * g_nSymbolsProElement) * getCharWidth() + i * getSideDelta());
                    rectSymbol.setTop(nTop + getLineDelta());
                    rectSymbol.setHeight(nHeight - getLineDelta());

                    if (((nIndex + 1) % g_nBytesProLine) == 0) {
                        rectSymbol.setWidth(g_nSymbolsProElement * getCharWidth());
                    } else if (bIsSelected && (!bIsSelectedNext)) {
                        rectSymbol.setWidth(g_nSymbolsProElement * getCharWidth());
                        //                    } else if (bIsHighlighted && (!bIsHighlightedNext)) {
                        //                        rectSymbol.setWidth(g_nSymbolsProElement * getCharWidth());
                    } else {
                        rectSymbol.setWidth(g_nSymbolsProElement * getCharWidth() + getSideDelta());
                    }
                } else if (nColumn == COLUMN_SYMBOLS) {
                    rectSymbol.setLeft(nLeft + (i + 1) * getCharWidth());
                    rectSymbol.setTop(nTop + getLineDelta());
                    rectSymbol.setWidth(getCharWidth());
                    rectSymbol.setHeight(nHeight - getLineDelta());
                }

                if (rectSymbol.left() < (nLeft + nWidth)) {  // Paint Only visible
                    if (g_listByteRecords.at(nIndex).bIsBold) {
                        pPainter->save();
                        QFont font = pPainter->font();
                        font.setBold(true);
                        pPainter->setFont(font);
                    }

                    QString sSymbol;

                    if (nColumn == COLUMN_ELEMENTS) {
                        sSymbol = g_listByteRecords.at(nIndex).sElement;
                    } else if (nColumn == COLUMN_SYMBOLS) {
                        sSymbol = g_listByteRecords.at(nIndex).sSymbol;
                    }

                    if (bIsSelected) {
                        // pPainter->fillRect(rectSymbol, viewport()->palette().color(QPalette::Highlight));  // TODO Options
                        pPainter->fillRect(rectSymbol, g_listByteRecords.at(nIndex).colBackgroundSelected);
                        //                        pPainter->fillRect(rectSymbol, QColor(100, 0, 0, 10));
                    } /*else if (bIsHighlighted) {
                        pPainter->fillRect(rectSymbol, g_listByteRecords.at(nIndex).colBackground);
                    }*/

                    if (bIsSelected) {
                        // Draw lines
                        bool bTop = false;
                        bool bLeft = false;
                        bool bBottom = false;
                        bool bRight = false;

                        if (((nIndex % g_nBytesProLine) == 0) || (!isViewOffsetSelected(nCurrent - 1))) {
                            bLeft = true;
                        }

                        if ((((nIndex + 1) % g_nBytesProLine) == 0) || (!isViewOffsetSelected(nCurrent + 1))) {
                            bRight = true;
                        }

                        if (!isViewOffsetSelected(nCurrent - g_nBytesProLine)) {
                            bTop = true;
                        }

                        if (!isViewOffsetSelected(nCurrent + g_nBytesProLine)) {
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
                    }

                    //                    if (nColumn == COLUMN_HEX) {
                    //                        pPainter->drawText(rectSymbol, sSymbol);
                    //                    } else if (nColumn == COLUMN_SYMBOLS) {
                    //                        if (sSymbol != "") {
                    //                            pPainter->drawText(rectSymbol, sSymbol);
                    //                        }
                    //                    }

                    if (g_listByteRecords.at(nIndex).bIsBold) {
                        pPainter->restore();
                    }
                } else {
                    break;  // Do not paint invisible
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

    if (nColumn == COLUMN_ADDRESS) {
        sKey = QString("address");
    } else if (nColumn == COLUMN_ELEMENTS) {
        sKey = QString("hex");
    } else if (nColumn == COLUMN_SYMBOLS) {
        sKey = QString("symbols");
    }

    if (sKey != "") {
        sKey += QString("_%1").arg(getViewOffsetStart());
        sKey += QString("_%1").arg(getViewSize());
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
            QPixmap pixmap(nWidth * ratio, nHeight * ratio);
            pixmap.setDevicePixelRatio(ratio);
            pixmap.fill(Qt::transparent);

            QPainter painterPixmap(&pixmap);
            painterPixmap.setFont(pPainter->font());
            painterPixmap.setBackgroundMode(Qt::TransparentMode);

            qint32 nNumberOfRows = g_listLocationRecords.count();

            if (nColumn == COLUMN_ADDRESS) {
                for (qint32 i = 0; i < nNumberOfRows; i++) {
                    QRect rectSymbol;
                    rectSymbol.setLeft(getCharWidth());
                    rectSymbol.setTop(getLineHeight() * i + getLineDelta());
                    rectSymbol.setWidth(nWidth);
                    rectSymbol.setHeight(getLineHeight() - getLineDelta());

                    painterPixmap.drawText(rectSymbol, g_listLocationRecords.at(i).sLocation);  // TODO Text Optional //            pPainter->restore();
                    //                    pPainter->drawText(rectSymbol, g_listLocationRecords.at(i).sLocation);
                }
            } else if ((nColumn == COLUMN_ELEMENTS) || (nColumn == COLUMN_SYMBOLS)) {
                QFont fontBold = painterPixmap.font();
                //                QFont fontBold = pPainter->font();
                fontBold.setBold(true);

                for (qint32 nRow = 0; nRow * g_nBytesProLine < g_nDataBlockSize; nRow++) {
                    qint64 nDataBlockSize = qMin(g_nDataBlockSize - nRow * g_nBytesProLine, g_nBytesProLine);

                    for (qint32 i = 0; i < nDataBlockSize; i++) {
                        qint32 nIndex = nRow * g_nBytesProLine + i;
                        bool bIsHighlighted = g_listByteRecords.at(nIndex).bIsHighlighted;
                        bool bIsHighlightedNext = false;

                        if (nIndex + 1 < g_nDataBlockSize) {
                            bIsHighlightedNext = g_listByteRecords.at(nIndex + 1).bIsHighlighted;
                        }

                        QRect rectSymbol;

                        if (nColumn == COLUMN_ELEMENTS) {
                            rectSymbol.setLeft(getCharWidth() + (i * g_nSymbolsProElement) * getCharWidth() + i * getSideDelta());
                            rectSymbol.setTop(getLineHeight() * nRow + getLineDelta());
                            rectSymbol.setHeight(getLineHeight() - getLineDelta());

                            if (((nIndex + 1) % g_nBytesProLine) == 0) {
                                rectSymbol.setWidth(g_nSymbolsProElement * getCharWidth());
                            } else if (bIsHighlighted && (!bIsHighlightedNext)) {
                                rectSymbol.setWidth(g_nSymbolsProElement * getCharWidth());
                            } else {
                                rectSymbol.setWidth(g_nSymbolsProElement * getCharWidth() + getSideDelta());
                            }
                        } else if (nColumn == COLUMN_SYMBOLS) {
                            rectSymbol.setLeft((i + 1) * getCharWidth());
                            rectSymbol.setTop(getLineHeight() * nRow + getLineDelta());
                            rectSymbol.setWidth(getCharWidth());
                            rectSymbol.setHeight(getLineHeight() - getLineDelta());
                        }

                        if (g_listByteRecords.at(nIndex).bIsBold) {
                            painterPixmap.save();
                            painterPixmap.setFont(fontBold);
                            //                            pPainter->save();
                            //                            pPainter->setFont(fontBold);
                        }

                        QString sSymbol;

                        if (nColumn == COLUMN_ELEMENTS) {
                            sSymbol = g_listByteRecords.at(nIndex).sElement;
                        } else if (nColumn == COLUMN_SYMBOLS) {
                            sSymbol = g_listByteRecords.at(nIndex).sSymbol;
                        }

                        if (bIsHighlighted) {
                            painterPixmap.fillRect(rectSymbol, g_listByteRecords.at(nIndex).colBackground);
                            //                            pPainter->fillRect(rectSymbol, g_listByteRecords.at(nIndex).colBackground);
                        }

                        if (nColumn == COLUMN_ELEMENTS) {
                            painterPixmap.drawText(rectSymbol, sSymbol, Qt::AlignVCenter | Qt::AlignHCenter);
                            //                            pPainter->drawText(rectSymbol, sSymbol);
                        } else if (nColumn == COLUMN_SYMBOLS) {
                            if (sSymbol != "") {
                                painterPixmap.drawText(rectSymbol, sSymbol);
                                //                                pPainter->drawText(rectSymbol, sSymbol);
                            }
                        }

                        if (g_listByteRecords.at(nIndex).bIsBold) {
                            painterPixmap.restore();
                            //                            pPainter->restore();
                        }
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
    // TODO multisearch

    if (isContextMenuEnable()) {
        QAction actionDataInspector(tr("Data inspector"), this);
        actionDataInspector.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_DATAINSPECTOR));
        connect(&actionDataInspector, SIGNAL(triggered()), this, SLOT(_showDataInspector()));
        if (getViewWidgetState(VIEWWIDGET_DATAINSPECTOR)) {
            actionDataInspector.setCheckable(true);
            actionDataInspector.setChecked(true);
        }

        QAction actionDataConvertor(tr("Data convertor"), this);
        actionDataConvertor.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_DATACONVERTOR));
        connect(&actionDataConvertor, SIGNAL(triggered()), this, SLOT(_showDataConvertor()));
        if (getViewWidgetState(VIEWWIDGET_DATACONVERTOR)) {
            actionDataConvertor.setCheckable(true);
            actionDataConvertor.setChecked(true);
        }

        QAction actionMultisearch(tr("Multisearch"), this);
        actionMultisearch.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_MULTISEARCH));
        connect(&actionMultisearch, SIGNAL(triggered()), this, SLOT(_showMultisearch()));
        if (getViewWidgetState(VIEWWIDGET_MULTISEARCH)) {
            actionMultisearch.setCheckable(true);
            actionMultisearch.setChecked(true);
        }

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

        QAction actionCopyData(tr("Data"), this);
        actionCopyData.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_COPY_DATA));
        connect(&actionCopyData, SIGNAL(triggered()), this, SLOT(_copyDataSlot()));

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

        QAction actionEditPatch(tr("Patch"), this);
        actionEditPatch.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_EDIT_PATCH));
        connect(&actionEditPatch, SIGNAL(triggered()), this, SLOT(_editPatch()));

        QAction actionEditRemove(tr("Remove"), this);
        actionEditRemove.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_EDIT_REMOVE));
        connect(&actionEditRemove, SIGNAL(triggered()), this, SLOT(_editRemove()));

        QAction actionEditResize(tr("Resize"), this);
        actionEditResize.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_EDIT_RESIZE));
        connect(&actionEditResize, SIGNAL(triggered()), this, SLOT(_editResize()));
#ifdef QT_SQL_LIB
        QAction actionBookmarkNew(tr("New"), this);
        actionBookmarkNew.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_BOOKMARKS_NEW));
        connect(&actionBookmarkNew, SIGNAL(triggered()), this, SLOT(_bookmarkNew()));

        QAction actionBookmarkList(tr("List"), this);
        actionBookmarkList.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_BOOKMARKS_LIST));
        connect(&actionBookmarkList, SIGNAL(triggered()), this, SLOT(_bookmarkList()));
        if (getViewWidgetState(VIEWWIDGET_BOOKMARKS)) {
            actionBookmarkList.setCheckable(true);
            actionBookmarkList.setChecked(true);
        }
#endif
        QAction actionStrings(tr("Strings"), this);
        actionStrings.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_STRINGS));
        connect(&actionStrings, SIGNAL(triggered()), this, SLOT(_strings()));
        if (getViewWidgetState(VIEWWIDGET_STRINGS)) {
            actionStrings.setCheckable(true);
            actionStrings.setChecked(true);
        }

        QAction actionVisualization(tr("Visualization"), this);
        actionVisualization.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_STRINGS));
        connect(&actionVisualization, SIGNAL(triggered()), this, SLOT(_visualization()));
        if (getViewWidgetState(VIEWWIDGET_VISUALIZATION)) {
            actionVisualization.setCheckable(true);
            actionVisualization.setChecked(true);
        }
#if defined(QT_SCRIPT_LIB) || defined(QT_QML_LIB)
        QAction actionScripts(tr("Scripts"), this);
        actionScripts.setShortcut(getShortcuts()->getShortcut(X_ID_HEX_SCRIPTS));
        connect(&actionScripts, SIGNAL(triggered()), this, SLOT(_scripts()));
        if (getViewWidgetState(VIEWWIDGET_SCRIPTS)) {
            actionScripts.setCheckable(true);
            actionScripts.setChecked(true);
        }
#endif

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

#ifdef QT_SQL_LIB
        QMenu menuBookmarks(tr("Bookmarks"), this);
#endif
        contextMenu.addAction(&actionDataInspector);

        if (menuState.nSelectionViewSize) {
            contextMenu.addAction(&actionDataConvertor);
        }

        contextMenu.addSeparator();

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

        contextMenu.addAction(&actionMultisearch);

        menuCopy.addAction(&actionCopyCursorOffset);
        menuCopy.addAction(&actionCopyCursorAddress);

        if (menuState.nSelectionViewSize) {
            contextMenu.addAction(&actionDumpToFile);
            contextMenu.addAction(&actionSignature);

            menuCopy.addSeparator();
            menuCopy.addAction(&actionCopyData);
        }

        contextMenu.addMenu(&menuCopy);

        if (g_hexOptions.bMenu_Disasm) {
            menuFollowIn.addAction(&actionDisasm);
        }

        if (g_hexOptions.bMenu_MemoryMap) {
            menuFollowIn.addAction(&actionMemoryMap);
        }

        if (g_hexOptions.bMenu_MainHex) {
            menuFollowIn.addAction(&actionMainHex);
        }

        if ((g_hexOptions.bMenu_Disasm) || (g_hexOptions.bMenu_MemoryMap)) {
            contextMenu.addMenu(&menuFollowIn);
        }

#ifdef QT_SQL_LIB
        if (getXInfoDB()) {
            menuBookmarks.addAction(&actionBookmarkNew);
            menuBookmarks.addAction(&actionBookmarkList);
            contextMenu.addMenu(&menuBookmarks);
        }
#endif
        contextMenu.addAction(&actionStrings);
        contextMenu.addAction(&actionVisualization);
#if defined(QT_SCRIPT_LIB) || defined(QT_QML_LIB)
        contextMenu.addAction(&actionScripts);
#endif
        menuEdit.setEnabled(!isReadonly());

        if (menuState.nSelectionViewSize) {
            menuEdit.addAction(&actionEditHex);
        }

        menuEdit.addAction(&actionEditPatch);

        if (XBinary::isResizeEnable(getDevice())) {
            menuEdit.addSeparator();
            menuEdit.addAction(&actionEditRemove);
            menuEdit.addAction(&actionEditResize);
        }

        contextMenu.addMenu(&menuEdit);

        menuSelect.addAction(&actionSelectAll);
        contextMenu.addMenu(&menuSelect);

        // TODO reset select

        contextMenu.exec(pos);
    }
}

void XHexView::wheelEvent(QWheelEvent *pEvent)
{
    if ((g_nViewStartDelta) && (pEvent->angleDelta().y() > 0)) {
        if (getCurrentViewOffsetFromScroll() == g_nViewStartDelta) {
            setCurrentViewOffsetToScroll(0);
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
        qint64 nViewStart = getViewOffsetStart();

        state.nSelectionViewSize = 1;

        if (pEvent->matches(QKeySequence::MoveToNextChar)) {
            state.nSelectionViewOffset++;
        } else if (pEvent->matches(QKeySequence::MoveToPreviousChar)) {
            state.nSelectionViewOffset--;
        } else if (pEvent->matches(QKeySequence::MoveToNextLine)) {
            state.nSelectionViewOffset += g_nBytesProLine;
        } else if (pEvent->matches(QKeySequence::MoveToPreviousLine)) {
            state.nSelectionViewOffset -= g_nBytesProLine;
        } else if (pEvent->matches(QKeySequence::MoveToStartOfLine)) {
            // TODO
        } else if (pEvent->matches(QKeySequence::MoveToEndOfLine)) {
            // TODO
        }

        if ((state.nSelectionViewOffset < 0) || (pEvent->matches(QKeySequence::MoveToStartOfDocument))) {
            state.nSelectionViewOffset = 0;
            g_nViewStartDelta = 0;
        }

        if ((state.nSelectionViewOffset >= getViewSize()) || (pEvent->matches(QKeySequence::MoveToEndOfDocument))) {
            state.nSelectionViewOffset = getViewSize() - 1;
            g_nViewStartDelta = 0;
        }

        setState(state);

        if (pEvent->matches(QKeySequence::MoveToNextChar) || pEvent->matches(QKeySequence::MoveToPreviousChar) || pEvent->matches(QKeySequence::MoveToNextLine) ||
            pEvent->matches(QKeySequence::MoveToPreviousLine)) {
            qint64 nRelOffset = state.nSelectionViewOffset - nViewStart;

            if (nRelOffset >= g_nBytesProLine * getLinesProPage()) {
                _goToViewOffset(nViewStart + g_nBytesProLine, true);
            } else if (nRelOffset < 0) {
                if (!_goToViewOffset(nViewStart - g_nBytesProLine, true)) {
                    _goToViewOffset(0);
                }
            }
        } else if (pEvent->matches(QKeySequence::MoveToNextPage) || pEvent->matches(QKeySequence::MoveToPreviousPage)) {
            if (pEvent->matches(QKeySequence::MoveToNextPage)) {
                _goToViewOffset(nViewStart + g_nBytesProLine * getLinesProPage());
            } else if (pEvent->matches(QKeySequence::MoveToPreviousPage)) {
                _goToViewOffset(nViewStart - g_nBytesProLine * getLinesProPage());
            }
        } else if (pEvent->matches(QKeySequence::MoveToStartOfDocument) || pEvent->matches(QKeySequence::MoveToEndOfDocument))  // TODO
        {
            _goToViewOffset(state.nSelectionViewOffset);
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

qint64 XHexView::getCurrentViewOffsetFromScroll()
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

void XHexView::setCurrentViewOffsetToScroll(qint64 nOffset)
{
    setViewOffsetStart(nOffset);
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
        setColumnWidth(COLUMN_ADDRESS, 2 * getCharWidth() + fm.boundingRect("00000000:00000000").width());
    } else {
        g_nAddressWidth = 8;
        setColumnWidth(COLUMN_ADDRESS, 2 * getCharWidth() + fm.boundingRect("0000:0000").width());
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
    if (getAddressMode() == LOCMODE_ADDRESS) {
        setColumnTitle(COLUMN_ADDRESS, tr("Address"));
    } else if ((getAddressMode() == LOCMODE_OFFSET) || (getAddressMode() == LOCMODE_THIS)) {
        setColumnTitle(COLUMN_ADDRESS, tr("Offset"));
    }
}

void XHexView::_headerClicked(qint32 nColumn)
{
    if (nColumn == COLUMN_ADDRESS) {
        // TODO Context Menu with
        if (getAddressMode() == LOCMODE_ADDRESS) {
            setColumnTitle(COLUMN_ADDRESS, tr("Offset"));
            setAddressMode(LOCMODE_OFFSET);
        } else if ((getAddressMode() == LOCMODE_OFFSET) || (getAddressMode() == LOCMODE_THIS)) {
            setColumnTitle(COLUMN_ADDRESS, tr("Address"));
            setAddressMode(LOCMODE_ADDRESS);
        }

        adjust(true);
    } else if (nColumn == COLUMN_ELEMENTS) {
        QMenu contextMenu(this);

        QMenu menuMode(tr("Mode"), this);

        QAction actionHex(QString("byte"), this);
        actionHex.setProperty("mode", MODE_BYTE);
        actionHex.setCheckable(true);
        actionHex.setChecked(g_mode == MODE_BYTE);
        connect(&actionHex, SIGNAL(triggered()), this, SLOT(changeMode()));
        menuMode.addAction(&actionHex);

        menuMode.addSeparator();

        QAction actionUint8(QString("uint8"), this);
        actionUint8.setProperty("mode", MODE_UINT8);
        actionUint8.setCheckable(true);
        actionUint8.setChecked(g_mode == MODE_UINT8);
        connect(&actionUint8, SIGNAL(triggered()), this, SLOT(changeMode()));
        menuMode.addAction(&actionUint8);

        QAction actionInt8(QString("int8"), this);
        actionInt8.setProperty("mode", MODE_INT8);
        actionInt8.setCheckable(true);
        actionInt8.setChecked(g_mode == MODE_INT8);
        connect(&actionInt8, SIGNAL(triggered()), this, SLOT(changeMode()));
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
    if (nColumn == COLUMN_ADDRESS) {
        setColumnTitle(COLUMN_ADDRESS, "");
        setAddressMode(LOCMODE_THIS);

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

QList<QChar> XHexView::getStringBuffer(QByteArray *pbaData)
{
    QList<QChar> listResult;

    qint32 nSize = pbaData->size();

    if (g_sCodePage == "") {
        for (qint32 i = 0; i < nSize; i++) {
            QChar _char = pbaData->at(i);

            // if ((_char < QChar(0x20)) || (_char > QChar(0x7e))) {
            //     _char = '.';
            // }
            if (!_char.isPrint()) {
                _char = '.';
            }

            listResult.append(_char);
        }
    } else {
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
        //    #ifdef QT_DEBUG
        //        QElapsedTimer timer;
        //        timer.start();
        //    #endif

        if (g_pCodec) {
            QString _sResult = g_pCodec->toUnicode(*pbaData);
            QVector<uint> vecSymbols = _sResult.toUcs4();
            qint32 _nSize = vecSymbols.size();

            for (qint32 i = 0; i < nSize; i++) {
                QChar _char;
                if (i < _nSize) {
                    _char = _sResult.at(i);

                    if (!_char.isPrint()) {
                        _char = '.';
                    }
                } else {
                    _char = QChar(' ');
                }

                // if (_char < QChar(0x20)) {
                //     _char = '.';
                // }
                listResult.append(_char);
            }

            //            if (_nSize == nSize) {  // TODO Check
            //                for (qint32 i = 0; i < nSize; i++) {
            //                    QChar _char = _sResult.at(i);

            //                    // if (_char < QChar(0x20)) {
            //                    //     _char = '.';
            //                    // }
            //                    if (!_char.isPrint()) {
            //                        _char = '.';
            //                    }

            //                    listResult.append(_char);
            //                }
            //            } else {
            //                //                QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme,_sResult);

            //                //                qint32 nCurrentPosition=0;

            //                //                while(true)
            //                //                {
            //                //                    qint32 _nCurrentPosition=finder.toNextBoundary();

            //                //                    QString _sChar=_sResult.mid(nCurrentPosition,_nCurrentPosition-nCurrentPosition);
            //                //                    QByteArray _baData=pCodec->fromUnicode(_sChar);

            //                //                    if(_sChar.size()==1)
            //                //                    {
            //                //                        if(_sChar.at(0)<QChar(0x20))
            //                //                        {
            //                //                            _sChar='.';
            //                //                        }
            //                //                    }

            //                //                    sResult.append(_sChar);

            //                //                    if(_baData.size()>1)
            //                //                    {
            //                //                        qint32 nAppendSize=_baData.size()-1;

            //                //                        for(qint32 j=0;j<nAppendSize;j++)
            //                //                        {
            //                //                            sResult.append(" "); // mb TODO another symbol
            //                //                        }
            //                //                    }

            //                //                    nCurrentPosition=_nCurrentPosition;

            //                //                    if(nCurrentPosition==-1)
            //                //                    {
            //                //                        break;
            //                //                    }
            //                //                }

            //                // TODO Check Big5
            //                for (qint32 i = 0; i < _nSize; i++) {
            //                    QChar _char = QChar(_sResult.at(i));

            //                    QByteArray _baData = g_pCodec->fromUnicode(&_char, 1);

            //                    if (!_char.isPrint()) {
            //                        _char = '.';
            //                    }

            //                    // if (_sChar.at(0) < QChar(0x20)) {
            //                    //     _sChar = '.';
            //                    // }

            //                    listResult.append(_char);

            //                    if (_baData.size() > 1) {
            //                        qint32 nAppendSize = _baData.size() - 1;

            //                        for (qint32 j = 0; j < nAppendSize; j++) {
            //                            listResult.append(QChar(' '));  // mb TODO another symbol
            //                        }
            //                    }
            //                }
            //            }
        }

//    #ifdef QT_DEBUG
//        qDebug("%lld %s",timer.elapsed(),sResult.toLatin1().data());
//    #endif
#endif
    }

    return listResult;
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

        adjustColumns();
        adjust(true);
    }
}

void XHexView::changeMode()
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

    if (mode == MODE_BYTE) {
        g_nSymbolsProElement = 2;
    } else if (mode == MODE_UINT8) {
        g_nSymbolsProElement = 3;
    } else if (mode == MODE_INT8) {
        g_nSymbolsProElement = 4;
    }
}
