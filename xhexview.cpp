/* Copyright (c) 2020-2026 hors<horsicq@gmail.com>
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

#include <QtMath>
#include <QToolTip>
#include <QElapsedTimer>

// Monotonic counter used to hand every XHexView instance a unique id. It never repeats within a
// process run, so pixmap-cache keys built from it can never collide across views (even after an
// instance is destroyed and a new one is created).
static qint32 g_nXHexViewInstanceCounter = 0;

XHexView::XHexView(QWidget *pParent) : XDeviceTableEditView(pParent)
{
    m_nInstanceId = g_nXHexViewInstanceCounter++;
    m_pMapOverviewDevice = nullptr;
    m_nMapOverviewViewSize = -1;
    m_nMapScanBands = 0;
    m_mapMode = MAPMODE_ENTROPY;

    // The map overview is scanned cooperatively in small batches driven by this timer, so a large
    // file on slow media never blocks the GUI thread.
    m_timerMapScan.setInterval(0);
    connect(&m_timerMapScan, SIGNAL(timeout()), this, SLOT(_scanMapStep()));

    // Editing the data invalidates the map overview so it recomputes from the new bytes.
    connect(this, SIGNAL(dataChanged(qint64, qint64)), this, SLOT(_invalidateMapOverviewSlot()));

    addShortcut(X_ID_HEX_DATA_INSPECTOR, this, SLOT(_dataInspector()));
    addShortcut(X_ID_HEX_DATA_CONVERTOR, this, SLOT(_dataConvertor()));
    addShortcut(X_ID_HEX_MULTISEARCH, this, SLOT(_multisearch()));
    addShortcut(X_ID_HEX_GOTO_OFFSET, this, SLOT(_goToOffsetSlot()));
    addShortcut(X_ID_HEX_GOTO_ADDRESS, this, SLOT(_goToAddressSlot()));
    addShortcut(X_ID_HEX_DUMPTOFILE, this, SLOT(_dumpToFileSlot()));
    addShortcut(X_ID_HEX_SELECT_ALL, this, SLOT(_selectAllSlot()));
    addShortcut(X_ID_HEX_COPY_DATA, this, SLOT(_copyDataSlot()));
    addShortcut(X_ID_HEX_COPY_OFFSET, this, SLOT(_copyOffsetSlot()));
    addShortcut(X_ID_HEX_COPY_ADDRESS, this, SLOT(_copyAddressSlot()));
    addShortcut(X_ID_HEX_FIND_STRING, this, SLOT(_findStringSlot()));
    addShortcut(X_ID_HEX_FIND_SIGNATURE, this, SLOT(_findSignatureSlot()));
    addShortcut(X_ID_HEX_FIND_VALUE, this, SLOT(_findValueSlot()));
    addShortcut(X_ID_HEX_FIND_NEXT, this, SLOT(_findNextSlot()));
    addShortcut(X_ID_HEX_SIGNATURE, this, SLOT(_hexSignatureSlot()));
    addShortcut(X_ID_HEX_FOLLOWIN_DISASM, this, SLOT(_disasmSlot()));
    addShortcut(X_ID_HEX_FOLLOWIN_MEMORYMAP, this, SLOT(_memoryMapSlot()));
    addShortcut(X_ID_HEX_FOLLOWIN_HEX, this, SLOT(_mainHexSlot()));
    addShortcut(X_ID_HEX_EDIT_HEX, this, SLOT(_editHex()));
    addShortcut(X_ID_HEX_EDIT_REMOVE, this, SLOT(_editRemove()));
    addShortcut(X_ID_HEX_EDIT_RESIZE, this, SLOT(_editResize()));

    m_nBytesProLine = 16;  // Default
    m_nElementByteSize = 1;
    m_nSymbolByteSize = 1;
    _setMode(ELEMENT_MODE_HEX);
    m_nDataBlockSize = 0;
    m_nViewStartDelta = 0;
    m_nThisBase = 0;
    m_nAddressWidth = 8;
    m_bIsLocationColon = false;

    addColumn(tr("Address"), 0, true);
    addColumn(tr("Hex"), 0, true);
    addColumn(tr("Symbols"), 0, true);

    setTextFont(XOptions::getMonoFont());
    m_sCodePage = "";
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
    m_pCodec = nullptr;
    m_pCodePageMenu = m_xCodePageOptions.createCodePagesMenu(this, true);
    connect(&m_xCodePageOptions, SIGNAL(setCodePage(QString)), this, SLOT(_setCodePage(QString)));
#endif
    connect(this, &XDeviceTableView::locationModeChanged, this, [this](qint32) { adjustHeader(); });
    setLocationMode(XBinaryView::LOCMODE_OFFSET);
    setMapEnable(true);
    setMapWidth(20);

    setVerticalLinesVisible(false);
}

XHexView::~XHexView()
{
    m_timerMapScan.stop();
    _clearPixmapCache();  // Do not leave this instance's pixmaps behind in the global cache
}

QString XHexView::_pixmapCacheKey(const QString &sSuffix) const
{
    return QString("XHexView_%1_%2").arg(m_nInstanceId).arg(sSuffix);
}

void XHexView::_insertPixmapToCache(const QString &sKey, const QPixmap &pixmap)
{
    if (QPixmapCache::insert(sKey, pixmap)) {
        m_listPixmapCacheKeys.append(sKey);
    }
}

void XHexView::_clearPixmapCache()
{
    qint32 nNumberOfKeys = m_listPixmapCacheKeys.size();

    for (qint32 i = 0; i < nNumberOfKeys; i++) {
        QPixmapCache::remove(m_listPixmapCacheKeys.at(i));
    }

    m_listPixmapCacheKeys.clear();
}

XHexView::MAPBANDSTATS XHexView::_calcBlockStats(const QByteArray &baData)
{
    // All four overview metrics from a single 256-bin histogram pass. Entropy mirrors
    // XBinary::getBinaryStatus(BSTATUS_ENTROPY) exactly (same histogram and 1/ln2 constant);
    // gradient/zeros/text mirror BSTATUS_GRADIENT/ZEROS/TEXT so the map agrees with DIE's widgets.
    MAPBANDSTATS result = {};

    qint32 nSize = baData.size();

    if (nSize <= 0) {
        return result;
    }

    quint64 nCounts[256] = {0};
    const unsigned char *pBytes = reinterpret_cast<const unsigned char *>(baData.constData());

    for (qint32 i = 0; i < nSize; i++) {
        nCounts[pBytes[i]]++;
    }

    const double dInvLog2 = 1.4426950408889634073599246810023;  // 1/ln(2)
    const double dN = (double)nSize;

    double dEntropy = 0.0;
    quint64 nSum = 0;        // sum of byte values (gradient)
    quint64 nTextCount = 0;  // printable/text bytes

    for (qint32 j = 0; j < 256; j++) {
        quint64 nCount = nCounts[j];

        if (nCount) {
            double dP = (double)nCount / dN;
            dEntropy += -dP * (qLn(dP) * dInvLog2);
            nSum += (quint64)j * nCount;

            // Printable range [32..126] plus 8(BS),10(LF),13(CR) - same rule as BSTATUS_TEXT
            if (((j >= 32) && (j <= 126)) || (j == 8) || (j == 10) || (j == 13)) {
                nTextCount += nCount;
            }
        }
    }

    result.dEntropy = dEntropy;                       // 0..8
    result.dGradient = (double)nSum / (dN * 255.0);   // 0..1
    result.dZeros = (double)nCounts[0] / dN;          // 0..1
    result.dText = (double)nTextCount / dN;           // 0..1

    return result;
}

double XHexView::_bandValue(const MAPBANDSTATS &stats) const
{
    double dValue = 0.0;

    if (m_mapMode == MAPMODE_ENTROPY) {
        dValue = stats.dEntropy / 8.0;
    } else if (m_mapMode == MAPMODE_GRADIENT) {
        dValue = stats.dGradient;
    } else if (m_mapMode == MAPMODE_ZEROS) {
        dValue = stats.dZeros;
    } else if (m_mapMode == MAPMODE_TEXT) {
        dValue = stats.dText;
    }

    if (dValue < 0.0) {
        dValue = 0.0;
    }
    if (dValue > 1.0) {
        dValue = 1.0;
    }

    return dValue;
}

QColor XHexView::_metricColor(double dNorm) const
{
    // DIE house style (see XVisualization): a single accent color darkened by the metric value.
    // factor 100 (value 0, unchanged) .. 300 (value 1, ~3x darker). Theme-aware via the palette.
    QColor colBase = viewport()->palette().color(QPalette::Highlight);
    qint32 nFactor = 100 + (qint32)(200.0 * dNorm);

    return colBase.darker(nFactor);
}

void XHexView::_updateMapOverview()
{
    QIODevice *pDevice = getBinaryView()->getInData().pDevice;
    qint64 nViewSize = getBinaryView()->getViewSize();

    // (Re)start the scan only when the underlying content changes (new device, or size change from
    // resize/remove). Redundant calls (scroll-triggered paintMap, bytes-per-line change) are no-ops.
    if ((m_pMapOverviewDevice == pDevice) && (m_nMapOverviewViewSize == nViewSize)) {
        return;
    }

    m_pMapOverviewDevice = pDevice;
    m_nMapOverviewViewSize = nViewSize;
    m_listMapStats.clear();

    if ((!pDevice) || (nViewSize <= 0)) {
        m_nMapScanBands = 0;
        m_timerMapScan.stop();
        return;
    }

    const qint32 N_MAX_BANDS = 256;  // overview resolution (independent of map pixel height)

    m_nMapScanBands = (qint32)qMin((qint64)N_MAX_BANDS, nViewSize);

    if (m_nMapScanBands < 1) {
        m_nMapScanBands = 1;
    }

    m_listMapStats.reserve(m_nMapScanBands);

    // The scan is done cooperatively in time-bounded batches (see _scanMapStep) so a large file on
    // slow media never blocks the GUI thread; the map fills in progressively instead.
    m_timerMapScan.start(0);
}

void XHexView::_scanMapStep()
{
    if ((!m_pMapOverviewDevice) || (m_nMapScanBands <= 0) || (m_listMapStats.size() >= m_nMapScanBands)) {
        m_timerMapScan.stop();
        return;
    }

    const qint64 N_SAMPLE_CAP = 0x10000;   // <=64 KiB read per band bounds work on huge files
    const qint64 N_TIME_BUDGET_MS = 4;     // keep each tick short so the UI stays responsive

    qint64 nViewSize = m_nMapOverviewViewSize;
    qint32 nNumberOfBands = m_nMapScanBands;

    QElapsedTimer budgetTimer;
    budgetTimer.start();

    // At least one band per tick (the elapsed check runs after the first iteration), so progress is
    // guaranteed even if a single band read is slower than the whole budget.
    do {
        qint32 i = m_listMapStats.size();

        qint64 nBandStartViewPos = (nViewSize * i) / nNumberOfBands;
        qint64 nBandEndViewPos = (nViewSize * (i + 1)) / nNumberOfBands;
        qint64 nBandSize = nBandEndViewPos - nBandStartViewPos;

        if (nBandSize <= 0) {
            nBandSize = 1;
        }

        qint32 nSampleSize = (qint32)qMin(nBandSize, N_SAMPLE_CAP);

        qint64 nDeviceOffset = getBinaryView()->viewPosToDeviceOffset(nBandStartViewPos);
        QByteArray baBlock = read_array(nDeviceOffset, nSampleSize);

        m_listMapStats.append(_calcBlockStats(baBlock));
    } while ((m_listMapStats.size() < nNumberOfBands) && (budgetTimer.elapsed() < N_TIME_BUDGET_MS));

    if (m_listMapStats.size() >= nNumberOfBands) {
        m_timerMapScan.stop();
    }

    // Repaint so the newly-computed bands appear (progressive fill of the map strip).
    viewport()->update();
}

void XHexView::adjustView()
{
    setTextFontFromOptions(XOptions::ID_HEX_FONT);

    m_bIsLocationColon = getGlobalOptions()->getValue(XOptions::ID_HEX_LOCATIONCOLON).toBool();

    viewport()->update();
}

void XHexView::_adjustView()
{
    adjustView();

    if (getBinaryView()->getInData().pDevice) {
        reload(true);
    }
}

void XHexView::setData(const XBinary::INDATA &inData, const XBinaryView::OPTIONS &options, bool bReload, XInfoDB *pInfoDB)
{
    XDeviceTableView::setData(inData, options);

    // Force the map overview to recompute for this load. The (device, viewSize) guard alone is not
    // enough: a freed device pointer can be reused at the same address by a same-size file, which
    // would otherwise falsely match and show the previous file's overview.
    m_pMapOverviewDevice = nullptr;
    m_nMapOverviewViewSize = -1;

    QIODevice *pDevice = getBinaryView()->getInData().pDevice;

    bool bReadOnly = false;

    if (pDevice) {
        bReadOnly = !(pDevice->isWritable());
    }

    setXInfoDB(pInfoDB);

    setReadonly(bReadOnly);

    adjustView();
    adjustMap();

    // setMemoryMap(options.memoryMapRegion);

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

void XHexView::setData(QIODevice *pDevice, const XBinaryView::OPTIONS &options, bool bReload, XInfoDB *pInfoDB)
{
    setData(XFormats::createINDATA(options.fileType, pDevice, options.bIsImage, options.nModuleAddress), options, bReload, pInfoDB);
}

void XHexView::goToOffset(qint64 nOffset)
{
    XVPOS nViewPos = getBinaryView()->deviceOffsetToViewPos(nOffset);
    _goToViewPos(nViewPos);
}

// XADDR XHexView::getStartLocation()
// {
//     return m_hexOptions.nStartLocation;
// }

// XADDR XHexView::getSelectionInitLocation()
// {
//     return getSelectionInitOffset() + m_hexOptions.nStartLocation;
// }

void XHexView::setBytesProLine(qint32 nBytesProLine)
{
    m_nBytesProLine = nBytesProLine;
    adjustScrollCount();
    adjustView();
}

void XHexView::setElementMode(ELEMENT_MODE mode)
{
    if ((mode < ELEMENT_MODE_HEX) || (mode > ELEMENT_MODE_INT64)) {
        return;
    }

    _setMode(mode);
    adjustColumns();
    adjust(true);
    emit elementModeChanged((qint32)mode);
}

XHexView::ELEMENT_MODE XHexView::getElementMode() const
{
    return m_mode;
}

void XHexView::setCodePage(const QString &sCodePage)
{
    _setCodePage(sCodePage);
}

QString XHexView::getCodePage() const
{
    return m_sCodePage;
}

QList<XShortcuts::MENUITEM> XHexView::getMenuItems()
{
    QList<XShortcuts::MENUITEM> listResults;

    STATE menuState = getState();

    if (menuState.nSelectionViewSize) {
        getShortcuts()->_addMenuItem_Checked(&listResults, X_ID_HEX_DATA_INSPECTOR, this, SLOT(_dataInspector()), XShortcuts::GROUPID_NONE,
                                             getViewWidgetState(VIEWWIDGET_DATAINSPECTOR));
        getShortcuts()->_addMenuItem_Checked(&listResults, X_ID_HEX_DATA_CONVERTOR, this, SLOT(_dataConvertor()), XShortcuts::GROUPID_NONE,
                                             getViewWidgetState(VIEWWIDGET_DATACONVERTOR));
        getShortcuts()->_addMenuSeparator(&listResults, XShortcuts::GROUPID_NONE);
    }

    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_GOTO_OFFSET, this, SLOT(_goToOffsetSlot()), XShortcuts::GROUPID_GOTO);
    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_GOTO_ADDRESS, this, SLOT(_goToAddressSlot()), XShortcuts::GROUPID_GOTO);

    if (menuState.nSelectionViewSize) {
        getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_GOTO_SELECTION_START, this, SLOT(_goToSelectionStart()),
                                     (XShortcuts::GROUPID_SELECTION << 8) | XShortcuts::GROUPID_GOTO);
        getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_GOTO_SELECTION_END, this, SLOT(_goToSelectionEnd()),
                                     (XShortcuts::GROUPID_SELECTION << 8) | XShortcuts::GROUPID_GOTO);
    }

    getShortcuts()->_addMenuItem_Checked(&listResults, X_ID_HEX_MULTISEARCH, this, SLOT(_multisearch()), XShortcuts::GROUPID_NONE,
                                         getViewWidgetState(VIEWWIDGET_MULTISEARCH));

    if (menuState.nSelectionViewSize) {
        getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_DUMPTOFILE, this, SLOT(_dumpToFileSlot()), XShortcuts::GROUPID_NONE);
        getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_SIGNATURE, this, SLOT(_hexSignatureSlot()), XShortcuts::GROUPID_NONE);
    }

    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_FIND_STRING, this, SLOT(_findStringSlot()), XShortcuts::GROUPID_FIND);
    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_FIND_SIGNATURE, this, SLOT(_findSignatureSlot()), XShortcuts::GROUPID_FIND);
    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_FIND_VALUE, this, SLOT(_findValueSlot()), XShortcuts::GROUPID_FIND);
    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_FIND_NEXT, this, SLOT(_findNextSlot()), XShortcuts::GROUPID_FIND);

    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_SELECT_ALL, this, SLOT(_selectAllSlot()), XShortcuts::GROUPID_SELECT);

    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_COPY_OFFSET, this, SLOT(_copyOffsetSlot()), XShortcuts::GROUPID_COPY);
    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_COPY_ADDRESS, this, SLOT(_copyAddressSlot()), XShortcuts::GROUPID_COPY);
    getShortcuts()->_addMenuSeparator(&listResults, XShortcuts::GROUPID_COPY);
    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_COPY_DATA, this, SLOT(_copyDataSlot()), XShortcuts::GROUPID_COPY);

    getShortcuts()->_addMenuItem_Checked(&listResults, X_ID_HEX_STRINGS, this, SLOT(_strings()), XShortcuts::GROUPID_NONE, getViewWidgetState(VIEWWIDGET_STRINGS));
    getShortcuts()->_addMenuItem_Checked(&listResults, X_ID_HEX_VISUALIZATION, this, SLOT(_visualization()), XShortcuts::GROUPID_NONE,
                                         getViewWidgetState(VIEWWIDGET_VISUALIZATION));

    getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_BOOKMARKS_NEW, this, SLOT(_bookmarkNew()), XShortcuts::GROUPID_BOOKMARKS);
    getShortcuts()->_addMenuItem_Checked(&listResults, X_ID_HEX_BOOKMARKS_LIST, this, SLOT(_bookmarkList()), XShortcuts::GROUPID_BOOKMARKS,
                                         getViewWidgetState(VIEWWIDGET_BOOKMARKS));

    if (getBinaryView()->getOptions()->bMenu_Disasm) {
        getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_FOLLOWIN_DISASM, this, SLOT(_disasmSlot()), XShortcuts::GROUPID_FOLLOWIN);
    }

    if (getBinaryView()->getOptions()->bMenu_MemoryMap) {
        getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_FOLLOWIN_MEMORYMAP, this, SLOT(_memoryMapSlot()), XShortcuts::GROUPID_FOLLOWIN);
    }

    if (getBinaryView()->getOptions()->bMenu_MainHex) {
        getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_FOLLOWIN_HEX, this, SLOT(_mainHexSlot()), XShortcuts::GROUPID_FOLLOWIN);
    }

    if (!isReadonly()) {
        if (menuState.nSelectionViewSize) {
            getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_EDIT_HEX, this, SLOT(_editHex()), XShortcuts::GROUPID_EDIT);
        }
        getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_EDIT_PATCH, this, SLOT(_editPatch()), XShortcuts::GROUPID_EDIT);

        if (XBinary::isResizeEnable(getBinaryView()->getInData().pDevice)) {
            getShortcuts()->_addMenuSeparator(&listResults, XShortcuts::GROUPID_EDIT);
            getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_EDIT_REMOVE, this, SLOT(_editRemove()), XShortcuts::GROUPID_EDIT);
            getShortcuts()->_addMenuItem(&listResults, X_ID_HEX_EDIT_RESIZE, this, SLOT(_editResize()), XShortcuts::GROUPID_EDIT);
        }
    }

    return listResults;
}

XHexView::SHOWRECORD XHexView::_getShowRecordByViewPos(qint64 nOffset)
{
    SHOWRECORD result = {};

    qint32 nNumberOfRecords = m_listShowRecords.size();

    for (qint32 i = 0; i < nNumberOfRecords; i++) {
        if ((m_listShowRecords.at(i).nViewPos != -1) && (m_listShowRecords.at(i).nViewPos <= nOffset) &&
            (nOffset < (m_listShowRecords.at(i).nViewPos + m_listShowRecords.at(i).nSize))) {
            result = m_listShowRecords.at(i);
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
        XVPOS nBlockViewPos = getViewPosStart() + (cursorPosition.nRow * m_nBytesProLine);

        if (cursorPosition.nColumn == COLUMN_LOCATION) {
            osResult.nViewPos = nBlockViewPos;
            //            osResult.nSize=g_nPieceSize;
            osResult.nSize = 1;
        } else if (cursorPosition.nColumn == COLUMN_ELEMENTS) {
            osResult.nViewPos = nBlockViewPos + ((cursorPosition.nAreaLeft - getSideDelta() - getCharWidth()) / (getCharWidth() * m_nPrintsProElement + getSideDelta())) *
                                                    m_nElementByteSize;
            //            osResult.nSize=g_nPieceSize;
            osResult.nSize = 1;
        } else if (cursorPosition.nColumn == COLUMN_SYMBOLS) {
            osResult.nViewPos = nBlockViewPos + ((cursorPosition.nAreaLeft - getSideDelta() - getCharWidth()) / getCharWidth()) * m_nSymbolByteSize;
            //            osResult.nSize=g_nPieceSize;
            osResult.nSize = 1;
        }

        //        osResult.nOffset=S_ALIGN_DOWN(osResult.nOffset,g_nPieceSize);

        if (isViewPosValid(osResult.nViewPos)) {
            SHOWRECORD showRecord = _getShowRecordByViewPos(osResult.nViewPos);

            if (showRecord.nSize) {
                osResult.nViewPos = showRecord.nViewPos;
                osResult.nSize = showRecord.nSize;
            }
        } else {
            osResult.nViewPos = getBinaryView()->getViewSize();  // TODO Check
            osResult.nSize = 0;
        }

        //        qDebug("nBlockOffset %x",nBlockOffset);
        //        qDebug("cursorPosition.nCellLeft %x",cursorPosition.nCellLeft);
        //        qDebug("getCharWidth() %x",getCharWidth());
        //        qDebug("nOffset %x",osResult.nOffset);
    } else if ((cursorPosition.bIsValid) && (cursorPosition.ptype == PT_MAP)) {
        osResult.nViewPos = XBinary::align_down((getBinaryView()->getViewSize() * cursorPosition.nPercentage) / getMapCount(), m_nBytesProLine);
        osResult.nSize = 0;
    }
    return osResult;
}

void XHexView::updateData()
{
    QIODevice *_pDevice = getBinaryView()->getInData().pDevice;

    if (_pDevice) {
        // Update cursor position
        XVPOS nDataBlockStartViewPos = getViewPosStart();  // TODO Check

        //        qint64 nCursorOffset = nBlockStartLine + getCursorDelta();

        //        if (nCursorOffset >= getViewSize()) {
        //            nCursorOffset = getViewSize() - 1;
        //        }

        //        setCursorViewPos(nCursorOffset);

        XBinary::MODE mode = XBinary::getWidthModeFromByteSize(m_nAddressWidth);

        m_listLocationRecords.clear();
        m_listShowRecords.clear();

        qint32 nDataBlockSize = m_nBytesProLine * getLinesProPage();

        nDataBlockSize = (qint32)qMin((qint64)nDataBlockSize, (qint64)(getBinaryView()->getViewSize() - nDataBlockStartViewPos));  // qint64 to avoid overflow on large files

        m_listHighlightsRegion.clear();
        if (getXInfoDB()) {
            QVector<XInfoDB::BOOKMARKRECORD> listBookMarks = getXInfoDB()->getBookmarkRecords(nDataBlockStartViewPos, XBinary::LT_OFFSET, nDataBlockSize);
            m_listHighlightsRegion.append(_convertBookmarksToHighlightRegion(&listBookMarks));
        }

        qint64 nDeviceOffset = getBinaryView()->viewPosToDeviceOffset(nDataBlockStartViewPos);

        m_baDataBuffer = read_array(nDeviceOffset, nDataBlockSize);
        // QList<QChar> listElements = getStringBuffer(&m_baDataBuffer);

        // qint32 nNumberOfElements = listElements.count();

        m_nDataBlockSize = m_baDataBuffer.size();

        if (m_nDataBlockSize) {
            QString sDataHexBuffer;
            QString sANSI;

            if ((m_mode == ELEMENT_MODE_HEX) || (m_mode == ELEMENT_MODE_BYTE)) {
                sDataHexBuffer = QByteArray(m_baDataBuffer.toHex());
            }

            if (m_sCodePage.isEmpty()) {
                sANSI = XBinary::dataToString(m_baDataBuffer, XBinary::DSMODE_NOPRINT_TO_DOT);
            }

            // Locations
            for (qint32 i = 0; i < m_nDataBlockSize; i += m_nBytesProLine) {
                XADDR nCurrentLocation = 0;

                LOCATIONRECORD record = {};
                record.nViewPos = i + nDataBlockStartViewPos;

                if (getlocationMode() == XBinaryView::LOCMODE_THIS) {
                    nCurrentLocation = record.nViewPos;

                    qint64 nDelta = (qint64)nCurrentLocation - (qint64)m_nThisBase;

                    record.sLocation = XBinary::thisToString(nDelta, getLocationBase());
                } else {
                    if (getlocationMode() == XBinaryView::LOCMODE_ADDRESS) {
                        nCurrentLocation = getBinaryView()->viewPosToAddress(record.nViewPos);
                    } else if (getlocationMode() == XBinaryView::LOCMODE_OFFSET) {
                        nCurrentLocation = getBinaryView()->viewPosToDeviceOffset(record.nViewPos);
                    }

                    if (getLocationBase() == 16) {
                        if (m_bIsLocationColon) {
                            record.sLocation = XBinary::valueToHexColon(mode, nCurrentLocation);
                        } else {
                            record.sLocation = XBinary::valueToHex(mode, nCurrentLocation);
                        }
                    } else {
                        record.sLocation = QString("%1").arg(nCurrentLocation, m_nAddressWidth, getLocationBase(), QChar('0'));
                    }
                }

                m_listLocationRecords.append(record);
            }

            // Elements
            qint32 nMaxBytes = 1;

            if (m_mode == ELEMENT_MODE_HEX) {
                nMaxBytes = 8;  // TODO compare with XBinary::getHexSize
            } else {
                nMaxBytes = m_nElementByteSize;
            }

            char *pData = m_baDataBuffer.data();
            qint32 nCurrentRowViewPos = 0;
            qint32 nRow = 0;
            bool bFirst = true;

            for (qint32 i = 0; i < m_nDataBlockSize;) {
                SHOWRECORD record = {};

                record.nSize = qMin(m_nElementByteSize, m_nDataBlockSize - i);  // The last element can be cut off at the end of the data
                record.nViewPos = nDataBlockStartViewPos + i;
                record.nRowViewPos = nCurrentRowViewPos;
                record.nRow = nRow;

                if (bFirst) {
                    record.bFirstRowSymbol = true;
                    bFirst = false;
                }

                if (m_sCodePage.isEmpty()) {
                    record.sSymbol = sANSI.mid(i, m_nElementByteSize);
                } else {
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
                    if (m_pCodec) {
                        qint32 nJmax = qMin(m_nDataBlockSize - i, nMaxBytes);

                        for (int j = record.nSize; j <= nJmax; j++) {
                            QTextCodec::ConverterState converterState = {};
                            record.nSize = j;
                            record.sSymbol = m_pCodec->toUnicode(pData + i, record.nSize, &converterState);
                            record.bIsSymbolError = (converterState.remainingChars > 0);

                            if (converterState.remainingChars == 0) {
                                break;
                            }
                        }
                    }
#endif
                }

                record.sElement = _formatElement(pData, i, record.nSize, sDataHexBuffer);

                // if (i < nNumberOfElements) {
                //     record.sSymbol = listElements.at(i);
                // }

                if (record.nSize == 1) {
                    record.bIsBold = (m_baDataBuffer.at(i) != 0);  // TODO optimize !!! TODO Different rules
                }

                QList<HIGHLIGHTREGION> listHighLightRegions = getHighlightRegion(&m_listHighlightsRegion, nDataBlockStartViewPos + i, XBinary::LT_OFFSET);

                if (listHighLightRegions.size()) {
                    record.bIsHighlighted = true;
                    record.colBackground = listHighLightRegions.at(0).colBackground;
                    record.colBackgroundSelected = listHighLightRegions.at(0).colBackgroundSelected;
                } else {
                    record.colBackgroundSelected = getColor(TCLOLOR_SELECTED);
                }

                //                record.bIsSelected = isViewPosSelected(nDataBlockStartOffset + i);

                i += record.nSize;
                nCurrentRowViewPos += record.nSize;

                if (nCurrentRowViewPos >= m_nBytesProLine) {
                    nCurrentRowViewPos -= m_nBytesProLine;
                    nRow++;
                    record.bLastRowSymbol = true;
                    bFirst = true;
                } else if (i >= m_nDataBlockSize) {  // Last element of the data block
                    record.bLastRowSymbol = true;
                }

                m_listShowRecords.append(record);
            }
        } else {
            m_baDataBuffer.clear();
        }

        // Build nRow -> first-record-index map so paintCell() can jump straight to a row's records
        // instead of rescanning the whole list for every painted row (was O(rows*records) per frame).
        m_listRowStartIndex.clear();
        qint32 nNumberOfShowRecords = m_listShowRecords.size();

        for (qint32 i = 0; i < nNumberOfShowRecords; i++) {
            qint32 nRecordRow = m_listShowRecords.at(i).nRow;

            while (m_listRowStartIndex.size() <= nRecordRow) {
                m_listRowStartIndex.append(i);
            }
        }

        setCurrentBlock(nDataBlockStartViewPos, m_nDataBlockSize);

        _clearPixmapCache();
    }
}

void XHexView::paintMap(QPainter *pPainter, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight)
{
    pPainter->save();

    // Ensure the overview scan is scheduled/running for the current content (non-blocking; the actual
    // work happens in _scanMapStep across event-loop ticks). Bars are cheap to draw, so - unlike the
    // data columns - the map is rendered directly each paint rather than cached; this also lets the
    // progressive scan and metric switches show immediately.
    _updateMapOverview();

    // Overview: one horizontal bar per band. Bar length and shade both encode the selected metric
    // (entropy / byte density / zeros / text) of that slice of the file. m_nMapScanBands is the fixed
    // total, so already-computed bands render at their final positions while a scan is still filling in.
    qint32 nNumberOfBands = m_nMapScanBands;
    qint32 nComputedBands = m_listMapStats.size();

    if (nNumberOfBands > 0) {
        for (qint32 i = 0; i < nComputedBands; i++) {
            qint32 nBandTop = nTop + (nHeight * i) / nNumberOfBands;
            qint32 nBandBottom = nTop + (nHeight * (i + 1)) / nNumberOfBands;
            qint32 nBandHeight = nBandBottom - nBandTop;

            if (nBandHeight < 1) {
                nBandHeight = 1;
            }

            double dNorm = _bandValue(m_listMapStats.at(i));

            qint32 nBarWidth = (qint32)(dNorm * nWidth);

            if ((nBarWidth < 1) && (dNorm > 0.0)) {
                nBarWidth = 1;  // Keep non-empty low-value regions visible
            }

            if (nBarWidth > 0) {
                pPainter->fillRect(nLeft, nBandTop, nBarWidth, nBandHeight, _metricColor(dNorm));
            }
        }
    }

    // Viewport indicator: show where the currently visible page sits inside the whole view.
    // Drawn live (not cached) because it moves on every scroll, whereas the cached base above does not.
    qint64 nViewSize = getBinaryView()->getViewSize();

    if (nViewSize > 0) {
        qint64 nViewStart = getViewPosStart();
        qint64 nPageSize = (qint64)m_nBytesProLine * getLinesProPage();

        if (nViewStart < 0) {
            nViewStart = 0;
        }
        if (nViewStart > nViewSize) {
            nViewStart = nViewSize;
        }

        qint32 nIndicatorTop = nTop + (qint32)((nViewStart * nHeight) / nViewSize);
        qint32 nIndicatorHeight = (qint32)((nPageSize * nHeight) / nViewSize);

        if (nIndicatorHeight < 2) {
            nIndicatorHeight = 2;  // Keep the indicator visible even for very large views
        }

        if ((nIndicatorTop + nIndicatorHeight) > (nTop + nHeight)) {
            nIndicatorTop = (nTop + nHeight) - nIndicatorHeight;
        }

        pPainter->fillRect(nLeft, nIndicatorTop, nWidth, nIndicatorHeight, getColor(TCLOLOR_SELECTED));
    }

    // Bookmark markers on top, so the map doubles as a navigation minimap.
    _paintMapBookmarks(pPainter, nLeft, nTop, nWidth, nHeight);

    pPainter->restore();
}

void XHexView::_paintMapBookmarks(QPainter *pPainter, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight)
{
    XInfoDB *pXInfoDB = getXInfoDB();

    if (!pXInfoDB) {
        return;
    }

    QVector<XInfoDB::BOOKMARKRECORD> *pListBookmarks = pXInfoDB->getBookmarkRecords();

    if ((!pListBookmarks) || (pListBookmarks->isEmpty())) {
        return;
    }

    qint64 nViewSize = getBinaryView()->getViewSize();

    if (nViewSize <= 0) {
        return;
    }

    qint32 nNumberOfBookmarks = pListBookmarks->size();

    for (qint32 i = 0; i < nNumberOfBookmarks; i++) {
        const XInfoDB::BOOKMARKRECORD &record = pListBookmarks->at(i);

        XVPOS nViewPos = getBinaryView()->locationToViewPos(record.nLocation, record.locationType);

        if (!getBinaryView()->isViewPosValid(nViewPos)) {
            continue;  // bookmark not mappable into this view
        }

        qint32 nMarkerTop = nTop + (qint32)((nViewPos * nHeight) / nViewSize);
        qint32 nMarkerHeight = (qint32)((record.nSize * nHeight) / nViewSize);

        if (nMarkerHeight < 2) {
            nMarkerHeight = 2;  // Keep single-byte bookmarks visible
        }

        if ((nMarkerTop + nMarkerHeight) > (nTop + nHeight)) {
            nMarkerTop = (nTop + nHeight) - nMarkerHeight;
        }

        QColor colMarker = XOptions::stringToColor(record.sColorBackground);

        if (!colMarker.isValid()) {
            colMarker = getColor(TCLOLOR_SELECTED);
        }

        colMarker.setAlpha(255);  // A minimap marker should read crisply over the heatmap regardless of the bookmark's highlight alpha

        pPainter->fillRect(nLeft, nMarkerTop, nWidth, nMarkerHeight, colMarker);
    }
}

void XHexView::paintCell(QPainter *pPainter, qint32 nRow, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight)
{
    // #ifdef QT_DEBUG
    //     QElapsedTimer timer;
    //     timer.start();
    // #endif
    //     g_pPainterText->drawRect(nLeft,nTop,nWidth,nHeight);

    if (nColumn == COLUMN_LOCATION) {
        //        if (nRow < m_listLocationRecords.size()) {
        //            QRect rectSymbol;

        //            rectSymbol.setLeft(nLeft + getCharWidth());
        //            rectSymbol.setTop(nTop + getLineDelta());
        //            rectSymbol.setWidth(nWidth);
        //            rectSymbol.setHeight(nHeight - getLineDelta());

        //            //            pPainter->save();
        //            //            pPainter->setPen(viewport()->palette().color(QPalette::Dark));
        //            pPainter->drawText(rectSymbol, m_listLocationRecords.at(nRow).sLocation);  // TODO Text Optional
        //                                                                              //            pPainter->restore();
        //        }
    } else if ((nColumn == COLUMN_ELEMENTS) || (nColumn == COLUMN_SYMBOLS)) {
        qint32 nNumberOfShowRecords = m_listShowRecords.size();

        QFont fontBold = pPainter->font();
        fontBold.setBold(true);

        // Jump directly to the first record of this row (see m_listRowStartIndex in updateData()).
        qint32 nStartRecord = (nRow < m_listRowStartIndex.size()) ? m_listRowStartIndex.at(nRow) : nNumberOfShowRecords;

        for (qint32 i = nStartRecord; i < nNumberOfShowRecords; i++) {
            if (m_listShowRecords.at(i).nRow > nRow) {
                break;  // Past our row, no more matches
            }

            SHOWRECORD record = m_listShowRecords.at(i);

            bool bIsSelected = isViewPosSelected(record.nViewPos);

            if (!bIsSelected) {
                continue;
            }

            bool bIsSelectedPrev = false;
            bool bIsSelectedNext = false;

            if (i - 1 >= 0) {
                bIsSelectedPrev = isViewPosSelected(m_listShowRecords.at(i - 1).nViewPos);
            }

            if (i + 1 < nNumberOfShowRecords) {
                bIsSelectedNext = isViewPosSelected(m_listShowRecords.at(i + 1).nViewPos);
            }

            QRectF rectSymbol;

            if (nColumn == COLUMN_ELEMENTS) {
                qint32 _nRowOffset = record.nRowViewPos / m_nElementByteSize;

                rectSymbol.setLeft(nLeft + getCharWidth() + (_nRowOffset * (m_nPrintsProElement * getCharWidth() + getSideDelta())));
                rectSymbol.setTop(nTop + getLineDelta());
                rectSymbol.setHeight(nHeight - getLineDelta());

                qint32 nSelWidth = ((record.nSize + m_nElementByteSize - 1) / m_nElementByteSize) * (m_nPrintsProElement * getCharWidth() + getSideDelta());

                if ((record.bLastRowSymbol) || (!bIsSelectedNext)) {
                    nSelWidth -= getSideDelta();
                }

                rectSymbol.setWidth(nSelWidth);
            } else if (nColumn == COLUMN_SYMBOLS) {
                rectSymbol.setLeft(nLeft + (record.nRowViewPos + 1) * getCharWidth());
                rectSymbol.setTop(nTop + getLineDelta());
                rectSymbol.setWidth(getCharWidth() * (record.nSize / m_nSymbolByteSize));
                rectSymbol.setHeight(nHeight - getLineDelta());
            }

            if (rectSymbol.left() >= (nLeft + nWidth)) {
                break;  // Off-screen to the right, skip remaining
            }

            if (record.bIsBold) {
                pPainter->save();
                pPainter->setFont(fontBold);
            }

            if (record.bIsSymbolError) {
                pPainter->fillRect(rectSymbol, QBrush(Qt::red));
            } else {
                pPainter->fillRect(rectSymbol, record.colBackgroundSelected);
            }

            // Draw selection border lines
            bool bTop = !isViewPosSelected(record.nViewPos - m_nBytesProLine);
            bool bLeft = (record.bFirstRowSymbol) || (!bIsSelectedPrev);
            bool bBottom = !isViewPosSelected(record.nViewPos + m_nBytesProLine);
            bool bRight = (record.bLastRowSymbol) || (!bIsSelectedNext);

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
        }
    }
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

    if (!sKey.isEmpty()) {
        sKey += QString("_%1").arg(getViewPosStart());
        sKey += QString("_%1").arg(getBinaryView()->getViewSize());
        sKey += QString("_%1").arg(nWidth);
        sKey += QString("_%1").arg(nHeight);
        sKey = _pixmapCacheKey(sKey);

        QPixmap _pixmap(0, 0);

        if (QPixmapCache::find(sKey, &_pixmap)) {
            // qDebug("m_pixmapCache");
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
                qint32 nNumberOfRows = m_listLocationRecords.size();

                for (qint32 i = 0; i < nNumberOfRows; i++) {
                    QRectF rectSymbol;
                    rectSymbol.setLeft(getCharWidth());
                    rectSymbol.setTop(getLineHeight() * i + getLineDelta());
                    rectSymbol.setWidth(nWidth);
                    rectSymbol.setHeight(getLineHeight() - getLineDelta());

                    painterPixmap.drawText(rectSymbol, m_listLocationRecords.at(i).sLocation);  // TODO Text Optional pPainter->restore();
                }
            } else if ((nColumn == COLUMN_ELEMENTS) || (nColumn == COLUMN_SYMBOLS)) {
                QFont fontBold = painterPixmap.font();
                fontBold.setBold(true);

                qint32 nNumberOfShowRecords = m_listShowRecords.size();

                for (qint32 i = 0; i < nNumberOfShowRecords; i++) {
                    SHOWRECORD record = m_listShowRecords.at(i);

                    bool bIsHighlighted = m_listShowRecords.at(i).bIsHighlighted;
                    bool bIsHighlightedNext = false;

                    if (i + 1 < nNumberOfShowRecords) {
                        bIsHighlightedNext = m_listShowRecords.at(i + 1).bIsHighlighted;
                    }

                    QRectF rectSymbol;

                    if (nColumn == COLUMN_ELEMENTS) {
                        qint32 _nRowOffset = record.nRowViewPos / m_nElementByteSize;

                        rectSymbol.setLeft(getCharWidth() + (_nRowOffset * (m_nPrintsProElement * getCharWidth() + getSideDelta())));
                        rectSymbol.setTop(getLineHeight() * record.nRow + getLineDelta());
                        rectSymbol.setHeight(getLineHeight() - getLineDelta());

                        qint32 nElementWidth = ((record.nSize + m_nElementByteSize - 1) / m_nElementByteSize) * (m_nPrintsProElement * getCharWidth() + getSideDelta());

                        if ((record.bLastRowSymbol) || (bIsHighlighted && (!bIsHighlightedNext))) {
                            nElementWidth -= getSideDelta();
                        }

                        rectSymbol.setWidth(nElementWidth);
                    } else if (nColumn == COLUMN_SYMBOLS) {
                        rectSymbol.setLeft((record.nRowViewPos + 1) * getCharWidth());
                        rectSymbol.setTop(getLineHeight() * record.nRow + getLineDelta());
                        rectSymbol.setWidth(getCharWidth() * (record.nSize / m_nSymbolByteSize));
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

                    if (record.bIsSymbolError) {
                        painterPixmap.fillRect(rectSymbol, QBrush(Qt::red));  // TODO
                    } else if (bIsHighlighted) {
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

            _insertPixmapToCache(sKey, pixmap);

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
        for (qint32 i = 0; i < m_nBytesProLine / m_nElementByteSize; i++) {
            QString sSymbol = QString("%1").arg(i * m_nElementByteSize, 2, getLocationBase(), QChar('0'));

            QRectF rectSymbol;

            rectSymbol.setLeft(nLeft + getCharWidth() + (i * m_nPrintsProElement) * getCharWidth() + i * getSideDelta());
            rectSymbol.setTop(nTop);
            rectSymbol.setWidth(m_nPrintsProElement * getCharWidth() + getSideDelta());
            rectSymbol.setHeight(nHeight);

            if ((rectSymbol.left()) < (nLeft + nWidth)) {
                pPainter->drawText(rectSymbol, Qt::AlignVCenter | Qt::AlignHCenter, sSymbol);
            }
        }
    } else {
        XAbstractTableView::paintTitle(pPainter, nColumn, nLeft, nTop, nWidth, nHeight, sTitle);
    }
}

void XHexView::wheelEvent(QWheelEvent *pEvent)
{
    if ((m_nViewStartDelta) && (pEvent->angleDelta().y() > 0)) {
        if (getCurrentViewPosFromScroll() == m_nViewStartDelta) {
            setCurrentViewPosToScroll(0);
            adjust(true);
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
        XVPOS nViewStart = getViewPosStart();

        state.nSelectionViewSize = 1;

        if (pEvent->matches(QKeySequence::MoveToNextChar)) {
            state.nSelectionViewPos += m_nElementByteSize;  // TODO fix UTF8
        } else if (pEvent->matches(QKeySequence::MoveToPreviousChar)) {
            state.nSelectionViewPos -= m_nElementByteSize;
        } else if (pEvent->matches(QKeySequence::MoveToNextLine)) {
            state.nSelectionViewPos += m_nBytesProLine;
        } else if (pEvent->matches(QKeySequence::MoveToPreviousLine)) {
            state.nSelectionViewPos -= m_nBytesProLine;
        } else if (pEvent->matches(QKeySequence::MoveToStartOfLine)) {
            state.nSelectionViewPos = XBinary::align_down(state.nSelectionViewPos, m_nBytesProLine);
        } else if (pEvent->matches(QKeySequence::MoveToEndOfLine)) {
            state.nSelectionViewPos = XBinary::align_down(state.nSelectionViewPos, m_nBytesProLine) + m_nBytesProLine - m_nElementByteSize;
        }

        if ((state.nSelectionViewPos < 0) || (pEvent->matches(QKeySequence::MoveToStartOfDocument))) {
            state.nSelectionViewPos = 0;
            m_nViewStartDelta = 0;
        }

        if ((state.nSelectionViewPos >= getBinaryView()->getViewSize()) || (pEvent->matches(QKeySequence::MoveToEndOfDocument))) {
            state.nSelectionViewPos = getBinaryView()->getViewSize() - 1;
            m_nViewStartDelta = 0;
        }

        if (isViewPosValid(state.nSelectionViewPos)) {
            SHOWRECORD showRecord = _getShowRecordByViewPos(state.nSelectionViewPos);

            if (showRecord.nSize) {
                state.nSelectionViewPos = showRecord.nViewPos;
                state.nSelectionViewSize = showRecord.nSize;
            }
        }

        setState(state);

        if (pEvent->matches(QKeySequence::MoveToNextChar) || pEvent->matches(QKeySequence::MoveToPreviousChar) || pEvent->matches(QKeySequence::MoveToNextLine) ||
            pEvent->matches(QKeySequence::MoveToPreviousLine)) {
            qint64 nRelOffset = state.nSelectionViewPos - nViewStart;

            if (nRelOffset >= m_nBytesProLine * getLinesProPage()) {
                _goToViewPos(nViewStart + m_nBytesProLine, true);
            } else if (nRelOffset < 0) {
                if (!_goToViewPos(nViewStart - m_nBytesProLine, true)) {
                    _goToViewPos(0);
                }
            }
        } else if (pEvent->matches(QKeySequence::MoveToNextPage) || pEvent->matches(QKeySequence::MoveToPreviousPage)) {
            if (pEvent->matches(QKeySequence::MoveToNextPage)) {
                _goToViewPos(nViewStart + m_nBytesProLine * getLinesProPage());
            } else if (pEvent->matches(QKeySequence::MoveToPreviousPage)) {
                _goToViewPos(nViewStart - m_nBytesProLine * getLinesProPage());
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

XVPOS XHexView::getCurrentViewPosFromScroll()
{
    XVPOS nResult = 0;

    qint32 nValue = verticalScrollBar()->value();

    qint64 nMaxValue = getMaxScrollValue() * m_nBytesProLine;

    if (getBinaryView()->getViewSize() > nMaxValue) {
        if (nValue == getMaxScrollValue()) {
            nResult = getBinaryView()->getViewSize() - m_nBytesProLine;
        } else {
            nResult = ((double)nValue / (double)getMaxScrollValue()) * getBinaryView()->getViewSize() + m_nViewStartDelta;
        }
    } else {
        nResult = (XVPOS)nValue * m_nBytesProLine + m_nViewStartDelta;
    }

    return nResult;
}

void XHexView::setCurrentViewPosToScroll(XVPOS nOffset)
{
    setViewPosStart(nOffset);
    m_nViewStartDelta = (nOffset) % m_nBytesProLine;

    qint32 nValue = 0;

    if (getBinaryView()->getViewSize() > (getMaxScrollValue() * m_nBytesProLine)) {
        if (nOffset == getBinaryView()->getViewSize() - m_nBytesProLine) {
            nValue = getMaxScrollValue();
        } else {
            nValue = ((double)(nOffset - m_nViewStartDelta) / ((double)getBinaryView()->getViewSize())) * (double)getMaxScrollValue();
        }
    } else {
        nValue = (nOffset) / m_nBytesProLine;
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

    // if (XBinary::getWidthModeFromSize(getStartLocation() + getViewSize()) == XBinary::MODE_64) {
    if (XBinary::getWidthModeFromSize(getBinaryView()->getViewSize()) == XBinary::MODE_64) {
        m_nAddressWidth = 16;
        setColumnWidth(COLUMN_LOCATION, 2 * getCharWidth() + fm.boundingRect("00000000:00000000").width());
    } else {
        m_nAddressWidth = 8;
        setColumnWidth(COLUMN_LOCATION, 2 * getCharWidth() + fm.boundingRect("0000:0000").width());
    }

    qint32 nNumberOfElements = m_nBytesProLine / m_nElementByteSize;
    qint32 nNumberOfSymbols = m_nBytesProLine / m_nSymbolByteSize;

    setColumnWidth(COLUMN_ELEMENTS, nNumberOfElements * m_nPrintsProElement * getCharWidth() + 2 * getCharWidth() + getSideDelta() * nNumberOfElements);
    setColumnWidth(COLUMN_SYMBOLS, (nNumberOfSymbols + 2) * getCharWidth());
}

void XHexView::adjustHeader()
{
    if (getlocationMode() == XBinaryView::LOCMODE_ADDRESS) {
        setColumnTitle(COLUMN_LOCATION, tr("Address"));
    } else if (getlocationMode() == XBinaryView::LOCMODE_OFFSET) {
        setColumnTitle(COLUMN_LOCATION, tr("Offset"));
    } else if (getlocationMode() == XBinaryView::LOCMODE_THIS) {
        setColumnTitle(COLUMN_LOCATION, QString());
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
        QMenu contextMenu(this);

        QList<XShortcuts::MENUITEM> listMenuItems;

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("10");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeLocationBase());
            menuItem.nSubgroups = XShortcuts::GROUPID_BASE;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (getLocationBase() == 10);
            menuItem.sPropertyName = "base";
            menuItem.varProperty = 10;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = QString("16");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeLocationBase());
            menuItem.nSubgroups = XShortcuts::GROUPID_BASE;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (getLocationBase() == 16);
            menuItem.sPropertyName = "base";
            menuItem.varProperty = 16;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = tr("Address");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeLocationMode());
            menuItem.nSubgroups = XShortcuts::GROUPID_LOCATION;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (getlocationMode() == XBinaryView::LOCMODE_ADDRESS);
            menuItem.sPropertyName = "mode";
            menuItem.varProperty = XBinaryView::LOCMODE_ADDRESS;

            listMenuItems.append(menuItem);
        }

        {
            XShortcuts::MENUITEM menuItem = {};
            menuItem.sText = tr("Offset");
            menuItem.pRecv = this;
            menuItem.pMethod = SLOT(changeLocationMode());
            menuItem.nSubgroups = XShortcuts::GROUPID_LOCATION;
            menuItem.bIsCheckable = true;
            menuItem.bIsChecked = (getlocationMode() == XBinaryView::LOCMODE_OFFSET);
            menuItem.sPropertyName = "mode";
            menuItem.varProperty = XBinaryView::LOCMODE_OFFSET;

            listMenuItems.append(menuItem);
        }

        getShortcuts()->adjustContextMenu(&contextMenu, &listMenuItems);

        contextMenu.exec(QCursor::pos());

        // adjust(true);
    } else if (nColumn == COLUMN_ELEMENTS) {
        QMenu contextMenu(this);

        QList<XShortcuts::MENUITEM> listMenuItems;

        _addElementModeMenuItem(&listMenuItems, tr("Hex"), ELEMENT_MODE_HEX);
        getShortcuts()->_addMenuSeparator(&listMenuItems, XShortcuts::GROUPID_MODE);
        _addElementModeMenuItem(&listMenuItems, QString("byte"), ELEMENT_MODE_BYTE);
        _addElementModeMenuItem(&listMenuItems, QString("word"), ELEMENT_MODE_WORD);
        _addElementModeMenuItem(&listMenuItems, QString("dword"), ELEMENT_MODE_DWORD);
        _addElementModeMenuItem(&listMenuItems, QString("qword"), ELEMENT_MODE_QWORD);
        getShortcuts()->_addMenuSeparator(&listMenuItems, XShortcuts::GROUPID_MODE);
        _addElementModeMenuItem(&listMenuItems, QString("uint8"), ELEMENT_MODE_UINT8);
        _addElementModeMenuItem(&listMenuItems, QString("int8"), ELEMENT_MODE_INT8);
        getShortcuts()->_addMenuSeparator(&listMenuItems, XShortcuts::GROUPID_MODE);
        _addElementModeMenuItem(&listMenuItems, QString("uint16"), ELEMENT_MODE_UINT16);
        _addElementModeMenuItem(&listMenuItems, QString("int16"), ELEMENT_MODE_INT16);
        getShortcuts()->_addMenuSeparator(&listMenuItems, XShortcuts::GROUPID_MODE);
        _addElementModeMenuItem(&listMenuItems, QString("uint32"), ELEMENT_MODE_UINT32);
        _addElementModeMenuItem(&listMenuItems, QString("int32"), ELEMENT_MODE_INT32);
        getShortcuts()->_addMenuSeparator(&listMenuItems, XShortcuts::GROUPID_MODE);
        _addElementModeMenuItem(&listMenuItems, QString("uint64"), ELEMENT_MODE_UINT64);
        _addElementModeMenuItem(&listMenuItems, QString("int64"), ELEMENT_MODE_INT64);

        _addElementWidthMenuItem(&listMenuItems, 8);
        _addElementWidthMenuItem(&listMenuItems, 16);
        _addElementWidthMenuItem(&listMenuItems, 24);
        _addElementWidthMenuItem(&listMenuItems, 32);
        _addElementWidthMenuItem(&listMenuItems, 48);
        _addElementWidthMenuItem(&listMenuItems, 64);

        getShortcuts()->adjustContextMenu(&contextMenu, &listMenuItems);

        contextMenu.exec(QCursor::pos());

    } else if (nColumn == COLUMN_SYMBOLS) {
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
        m_pCodePageMenu->exec(QCursor::pos());
#endif
    }

    XAbstractTableView::_headerClicked(nColumn);
}

void XHexView::_cellDoubleClicked(qint32 nRow, qint32 nColumn)
{
    if (nColumn == COLUMN_LOCATION) {
        setColumnTitle(COLUMN_LOCATION, "");
        setLocationMode(XBinaryView::LOCMODE_THIS);

        if (nRow < m_listLocationRecords.size()) {
            m_nThisBase = m_listLocationRecords.at(nRow).nViewPos;
        }

        adjust(true);
    }
}

void XHexView::adjustScrollCount()
{
    qint64 nViewSize = getBinaryView()->getViewSize();
    qint64 nTotalLineCount = 0;

    if (nViewSize > 0) {
        nTotalLineCount = nViewSize / m_nBytesProLine;

        if (nViewSize % m_nBytesProLine == 0) {
            nTotalLineCount--;  // A view that fills whole lines exactly needs one fewer scroll step
        }
    }

    //    if((getDataSize()>0)&&(getDataSize()<m_nBytesProLine))
    //    {
    //        nTotalLineCount=1;
    //    }

    setTotalScrollCount(nTotalLineCount);
}

void XHexView::adjustMap()
{
    if (isMapEnable()) {
        if (getBinaryView()->getInData().pDevice) {
            qint64 nNumberOfLines = getBinaryView()->getInData().pDevice->size() / m_nBytesProLine;
            if (nNumberOfLines > 100) {
                nNumberOfLines = 100;
            } else if (nNumberOfLines == 0) {
                nNumberOfLines = 1;
            }

            setMapCount((qint32)nNumberOfLines);
        }

        // The overview scan is kicked off lazily by paintMap() (which only runs when the view is
        // actually visible), so a hidden tab does no background I/O. The scan is incremental/
        // non-blocking, so there is no first-paint hitch to pre-empt here.
    }
}

void XHexView::_disasmSlot()
{
    if (getBinaryView()->getOptions()->bMenu_Disasm) {
        emit followLocation(getDeviceState().nSelectionDeviceOffset, XBinary::LT_OFFSET, 0, XOptions::WIDGETTYPE_DISASM);
    }
}

void XHexView::_memoryMapSlot()
{
    if (getBinaryView()->getOptions()->bMenu_MemoryMap) {
        emit followLocation(getDeviceState().nSelectionDeviceOffset, XBinary::LT_OFFSET, 0, XOptions::WIDGETTYPE_MEMORYMAP);
    }
}

void XHexView::_mainHexSlot()
{
    if (getBinaryView()->getOptions()->bMenu_MainHex) {
        DEVICESTATE deviceState = getDeviceState();
        emit followLocation(getDeviceState().nSelectionDeviceOffset, XBinary::LT_OFFSET, deviceState.nSelectionSize, XOptions::WIDGETTYPE_HEX);
    }
}

void XHexView::_setCodePage(const QString &sCodePage)
{
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
    m_sCodePage = sCodePage;

    QString sTitle = tr("Symbols");

    if (!m_sCodePage.isEmpty()) {
        sTitle = m_sCodePage;
        m_pCodec = QTextCodec::codecForName(m_sCodePage.toLatin1().data());
    }

    setColumnTitle(COLUMN_SYMBOLS, sTitle);

    adjust(true);
    emit codePageChanged(m_sCodePage);
#else
    Q_UNUSED(sCodePage)
#endif
}

void XHexView::changeElementWidth()
{
    QAction *pAction = qobject_cast<QAction *>(sender());

    if (pAction) {
        m_nBytesProLine = pAction->property("width").toUInt();

        adjustMap();
        adjustScrollCount();
        adjustColumns();
        adjust(true);
    }
}

void XHexView::changeElementMode()
{
    QAction *pAction = qobject_cast<QAction *>(sender());

    if (pAction) {
        ELEMENT_MODE mode = (ELEMENT_MODE)pAction->property("mode").toUInt();

        setElementMode(mode);
    }
}

void XHexView::_setMode(ELEMENT_MODE mode)
{
    m_mode = mode;

    if (mode == ELEMENT_MODE_HEX) {
        m_nPrintsProElement = 2;
        m_nElementByteSize = 1;
    } else if (mode == ELEMENT_MODE_BYTE) {
        m_nPrintsProElement = 2;
        m_nElementByteSize = 1;
    } else if (mode == ELEMENT_MODE_UINT8) {
        m_nPrintsProElement = 3;
        m_nElementByteSize = 1;
    } else if (mode == ELEMENT_MODE_INT8) {
        m_nPrintsProElement = 4;
        m_nElementByteSize = 1;
    } else if (mode == ELEMENT_MODE_WORD) {
        m_nPrintsProElement = 4;
        m_nElementByteSize = 2;
    } else if (mode == ELEMENT_MODE_UINT16) {
        m_nPrintsProElement = 5;
        m_nElementByteSize = 2;
    } else if (mode == ELEMENT_MODE_INT16) {
        m_nPrintsProElement = 6;
        m_nElementByteSize = 2;
    } else if (mode == ELEMENT_MODE_DWORD) {
        m_nPrintsProElement = 8;
        m_nElementByteSize = 4;
    } else if (mode == ELEMENT_MODE_UINT32) {
        m_nPrintsProElement = 10;
        m_nElementByteSize = 4;
    } else if (mode == ELEMENT_MODE_INT32) {
        m_nPrintsProElement = 11;
        m_nElementByteSize = 4;
    } else if (mode == ELEMENT_MODE_QWORD) {
        m_nPrintsProElement = 16;
        m_nElementByteSize = 8;
    } else if (mode == ELEMENT_MODE_UINT64) {
        m_nPrintsProElement = 18;
        m_nElementByteSize = 8;
    } else if (mode == ELEMENT_MODE_INT64) {
        m_nPrintsProElement = 19;
        m_nElementByteSize = 8;
    }
    // TODO make g_nSymbolsProElement make dynamic if UTF8
}

void XHexView::_addElementModeMenuItem(QList<XShortcuts::MENUITEM> *pListMenuItems, const QString &sText, ELEMENT_MODE mode)
{
    XShortcuts::MENUITEM menuItem = {};
    menuItem.sText = sText;
    menuItem.pRecv = this;
    menuItem.pMethod = SLOT(changeElementMode());
    menuItem.nSubgroups = XShortcuts::GROUPID_MODE;
    menuItem.bIsCheckable = true;
    menuItem.bIsChecked = (m_mode == mode);
    menuItem.sPropertyName = "mode";
    menuItem.varProperty = mode;
    pListMenuItems->append(menuItem);
}

void XHexView::_addElementWidthMenuItem(QList<XShortcuts::MENUITEM> *pListMenuItems, qint32 nWidth)
{
    XShortcuts::MENUITEM menuItem = {};
    menuItem.sText = QString::number(nWidth);
    menuItem.pRecv = this;
    menuItem.pMethod = SLOT(changeElementWidth());
    menuItem.nSubgroups = XShortcuts::GROUPID_WIDTH;
    menuItem.bIsCheckable = true;
    menuItem.bIsChecked = (m_nBytesProLine == nWidth);
    menuItem.sPropertyName = "width";
    menuItem.varProperty = nWidth;
    pListMenuItems->append(menuItem);
}

void XHexView::mousePressEvent(QMouseEvent *pEvent)
{
    // A click on the map's header button opens the overview-metric chooser (mirrors how a click on a
    // column header opens that column's configuration menu). The base does nothing for PT_MAPHEADER.
    // Gated on isActive() to match the base, which ignores all mouse input while inactive.
    if (isActive() && (pEvent->button() == Qt::LeftButton)) {
        CURSOR_POSITION cursorPosition = getCursorPosition(pEvent->pos());

        if (cursorPosition.ptype == PT_MAPHEADER) {
            _mapHeaderClicked();
            return;
        }
    }

    XAbstractTableView::mousePressEvent(pEvent);
}

void XHexView::mouseMoveEvent(QMouseEvent *pEvent)
{
    // Live readout while hovering the map: the map body shows offset + metrics under the cursor;
    // the map header advertises the (clickable) metric chooser.
    if (isActive()) {
        CURSOR_POSITION cursorPosition = getCursorPosition(pEvent->pos());

        if (cursorPosition.ptype == PT_MAP) {
            QString sText = _mapTooltipText(cursorPosition);

            if (!sText.isEmpty()) {
                QToolTip::showText(QCursor::pos(), sText, this);
            }
        } else if (cursorPosition.ptype == PT_MAPHEADER) {
            QToolTip::showText(QCursor::pos(), tr("Overview") + ": " + _mapModeTitle() + " (" + tr("click to change") + ")", this);
        } else {
            QToolTip::hideText();
        }
    }

    XAbstractTableView::mouseMoveEvent(pEvent);
}

QString XHexView::_mapModeTitle() const
{
    if (m_mapMode == MAPMODE_ENTROPY) {
        return tr("Entropy");
    } else if (m_mapMode == MAPMODE_GRADIENT) {
        return tr("Byte density");
    } else if (m_mapMode == MAPMODE_ZEROS) {
        return tr("Zeros");
    } else if (m_mapMode == MAPMODE_TEXT) {
        return tr("Text");
    }

    return QString();
}

QString XHexView::_mapTooltipText(const CURSOR_POSITION &cursorPosition)
{
    QString sResult;

    qint64 nViewSize = getBinaryView()->getViewSize();

    if (nViewSize <= 0) {
        return sResult;
    }

    OS os = cursorPositionToOS(cursorPosition);  // PT_MAP -> the view position a click would navigate to

    if (os.nViewPos == -1) {
        return sResult;
    }

    qint64 nDeviceOffset = getBinaryView()->viewPosToDeviceOffset(os.nViewPos);

    // Fall back to the view position if this slice has no backing device offset (virtual/image regions).
    quint64 nShownOffset = (nDeviceOffset >= 0) ? (quint64)nDeviceOffset : (quint64)os.nViewPos;

    sResult = tr("Offset") + QString(": 0x%1").arg(XBinary::valueToHexEx(nShownOffset));

    qint32 nNumberOfBands = m_listMapStats.size();

    if (nNumberOfBands > 0) {
        qint64 nBand = (os.nViewPos * nNumberOfBands) / nViewSize;

        if (nBand < 0) {
            nBand = 0;
        }
        if (nBand >= nNumberOfBands) {
            nBand = nNumberOfBands - 1;
        }

        MAPBANDSTATS stats = m_listMapStats.at((qint32)nBand);

        sResult += "\n" + tr("Entropy") + QString(": %1").arg(stats.dEntropy, 0, 'f', 2);
        sResult += "\n" + tr("Byte density") + QString(": %1").arg(stats.dGradient, 0, 'f', 2);
        sResult += "\n" + tr("Zeros") + QString(": %1").arg(stats.dZeros, 0, 'f', 2);
        sResult += "\n" + tr("Text") + QString(": %1").arg(stats.dText, 0, 'f', 2);
    }

    // If a bookmark covers the hovered slice, surface its comment.
    XInfoDB *pXInfoDB = getXInfoDB();

    if (pXInfoDB) {
        QVector<XInfoDB::BOOKMARKRECORD> *pListBookmarks = pXInfoDB->getBookmarkRecords();

        if (pListBookmarks) {
            qint32 nNumberOfBookmarks = pListBookmarks->size();

            for (qint32 i = 0; i < nNumberOfBookmarks; i++) {
                const XInfoDB::BOOKMARKRECORD &record = pListBookmarks->at(i);

                XVPOS nBookmarkViewPos = getBinaryView()->locationToViewPos(record.nLocation, record.locationType);

                if (!getBinaryView()->isViewPosValid(nBookmarkViewPos)) {
                    continue;
                }

                qint64 nBookmarkSize = (record.nSize > 0) ? record.nSize : 1;

                if ((os.nViewPos >= nBookmarkViewPos) && (os.nViewPos < (nBookmarkViewPos + nBookmarkSize))) {
                    sResult += "\n" + tr("Bookmark");

                    if (!record.sComment.isEmpty()) {
                        sResult += ": " + record.sComment;
                    }

                    break;  // one bookmark line is enough
                }
            }
        }
    }

    return sResult;
}

void XHexView::_mapHeaderClicked()
{
    QMenu contextMenu(this);

    QList<XShortcuts::MENUITEM> listMenuItems;

    _addMapModeMenuItem(&listMenuItems, tr("Entropy"), MAPMODE_ENTROPY);
    _addMapModeMenuItem(&listMenuItems, tr("Byte density"), MAPMODE_GRADIENT);
    _addMapModeMenuItem(&listMenuItems, tr("Zeros"), MAPMODE_ZEROS);
    _addMapModeMenuItem(&listMenuItems, tr("Text"), MAPMODE_TEXT);

    getShortcuts()->adjustContextMenu(&contextMenu, &listMenuItems);

    contextMenu.exec(QCursor::pos());
}

void XHexView::_addMapModeMenuItem(QList<XShortcuts::MENUITEM> *pListMenuItems, const QString &sText, MAPMODE mapMode)
{
    XShortcuts::MENUITEM menuItem = {};
    menuItem.sText = sText;
    menuItem.pRecv = this;
    menuItem.pMethod = SLOT(changeMapMode());
    menuItem.nSubgroups = XShortcuts::GROUPID_NONE;
    menuItem.bIsCheckable = true;
    menuItem.bIsChecked = (m_mapMode == mapMode);
    menuItem.sPropertyName = "mapmode";
    menuItem.varProperty = mapMode;
    pListMenuItems->append(menuItem);
}

void XHexView::changeMapMode()
{
    QAction *pAction = qobject_cast<QAction *>(sender());

    if (pAction) {
        m_mapMode = (MAPMODE)pAction->property("mapmode").toInt();

        // The band stats already hold all metrics and the map is drawn directly (not cached), so a
        // repaint immediately reflects the new metric - no rescan or cache invalidation needed.
        viewport()->update();
    }
}

void XHexView::_invalidateMapOverviewSlot()
{
    // Data changed: force _updateMapOverview() to restart the scan on the next paint, and drop the
    // cached data-column pixmaps (their bytes changed too). The map itself is drawn directly, uncached.
    m_pMapOverviewDevice = nullptr;
    m_nMapOverviewViewSize = -1;
    _clearPixmapCache();
    viewport()->update();
}

QString XHexView::_formatElement(char *pData, qint32 nOffset, qint32 nSize, const QString &sDataHexBuffer)
{
    QString sResult;

    char baElement[8] = {};  // Zero-padded buffer: the last element can be cut off at the end of the data
    memcpy(baElement, pData + nOffset, qMin(nSize, (qint32)sizeof(baElement)));

    if (m_mode == ELEMENT_MODE_HEX) {
        sResult = sDataHexBuffer.mid(nOffset * 2, 2 * nSize);
    } else if (m_mode == ELEMENT_MODE_BYTE) {
        sResult = sDataHexBuffer.mid(nOffset * 2, 2);
    } else if (m_mode == ELEMENT_MODE_UINT8) {
        sResult = QString::number(XBinary::_read_uint8(baElement));
    } else if (m_mode == ELEMENT_MODE_INT8) {
        sResult = QString::number(XBinary::_read_int8(baElement));
    } else if (m_mode == ELEMENT_MODE_WORD) {
        sResult = XBinary::valueToHex(XBinary::_read_uint16(baElement));
    } else if (m_mode == ELEMENT_MODE_UINT16) {
        sResult = QString::number(XBinary::_read_uint16(baElement));
    } else if (m_mode == ELEMENT_MODE_INT16) {
        sResult = QString::number(XBinary::_read_int16(baElement));
    } else if (m_mode == ELEMENT_MODE_DWORD) {
        sResult = XBinary::valueToHex(XBinary::_read_uint32(baElement));
    } else if (m_mode == ELEMENT_MODE_UINT32) {
        sResult = QString::number(XBinary::_read_uint32(baElement));
    } else if (m_mode == ELEMENT_MODE_INT32) {
        sResult = QString::number(XBinary::_read_int32(baElement));
    } else if (m_mode == ELEMENT_MODE_QWORD) {
        sResult = XBinary::valueToHex(XBinary::_read_uint64(baElement));
    } else if (m_mode == ELEMENT_MODE_UINT64) {
        sResult = QString::number(XBinary::_read_uint64(baElement));
    } else if (m_mode == ELEMENT_MODE_INT64) {
        sResult = QString::number(XBinary::_read_int64(baElement));
    }

    return sResult;
}
