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
#include "xhexviewwidget.h"

#include "ui_xhexviewwidget.h"

XHexViewWidget::XHexViewWidget(QWidget *pParent) : XShortcutsWidget(pParent), ui(new Ui::XHexViewWidget)
{
    ui->setupUi(this);

    XOptions::adjustToolButton(ui->toolButtonDataInspector, XOptions::ICONTYPE_DATA);
    XOptions::adjustToolButton(ui->toolButtonStrings, XOptions::ICONTYPE_STRING);

    ui->toolButtonDataInspector->setToolTip(tr("Inspector"));
    ui->toolButtonStrings->setToolTip(tr("Strings"));
    ui->checkBoxReadonly->setText(tr("Readonly"));
    ui->comboBoxType->setToolTip(tr("Type"));
    ui->comboBoxMapMode->setToolTip(tr("Mode"));
    ui->comboBoxLocationBase->setToolTip(tr("Base"));

    XFormats::setBaseComboBox(ui->comboBoxLocationBase, 10);

    g_pDevice = nullptr;
    g_options = {};

    connect(ui->scrollAreaHex, SIGNAL(followLocation(quint64, qint32, qint64, qint32)), this, SIGNAL(followLocation(quint64, qint32, qint64, qint32)));
    connect(ui->scrollAreaHex, SIGNAL(errorMessage(QString)), this, SLOT(errorMessageSlot(QString)));
    connect(ui->scrollAreaHex, SIGNAL(currentLocationChanged(quint64, qint32, qint64)), this, SIGNAL(currentLocationChanged(quint64, qint32, qint64)));
    connect(ui->scrollAreaHex, SIGNAL(selectionChanged()), this, SLOT(adjust()));
    connect(ui->scrollAreaHex, SIGNAL(dataChanged(qint64, qint64)), this, SIGNAL(dataChanged(qint64, qint64)));
    connect(ui->scrollAreaHex, SIGNAL(deviceSizeChanged(qint64, qint64)), this, SIGNAL(deviceSizeChanged(qint64, qint64)));
    connect(ui->scrollAreaHex, SIGNAL(viewWidgetsStateChanged()), this, SLOT(viewWidgetsState()));

    setReadonlyVisible(false);
    setReadonly(true);
}

XHexViewWidget::~XHexViewWidget()
{
    delete ui;
}

void XHexViewWidget::setGlobal(XShortcuts *pShortcuts, XOptions *pXOptions)
{
    ui->scrollAreaHex->setGlobal(pShortcuts, pXOptions);
    XShortcutsWidget::setGlobal(pShortcuts, pXOptions);
}

void XHexViewWidget::setData(QIODevice *pDevice, const OPTIONS &options)
{
    g_pDevice = pDevice;
    g_options = options;

    if (pDevice) {
        XFormats::setFileTypeComboBox(options.fileType, pDevice, ui->comboBoxType, XBinary::TL_OPTION_ALL);
        XFormats::getMapModesList(options.fileType, ui->comboBoxMapMode);
    } else {
        ui->scrollAreaHex->setDevice(nullptr);
    }

    // adjustVisitedState();

    reloadFileType();

    // ui->checkBoxReadonly->setEnabled(pDevice->isWritable());

    // ui->scrollAreaHex->setData(pDevice, options, true);
}

void XHexViewWidget::setDevice(QIODevice *pDevice)
{
    ui->scrollAreaHex->setDevice(pDevice);
}

void XHexViewWidget::setXInfoDB(XInfoDB *pXInfoDB)
{
    ui->scrollAreaHex->setXInfoDB(pXInfoDB);
}

void XHexViewWidget::reload()
{
    ui->scrollAreaHex->reload(true);
}

void XHexViewWidget::cleanup()
{
    ui->scrollAreaHex->setDevice(nullptr);
    ui->scrollAreaHex->setXInfoDB(nullptr);
}

void XHexViewWidget::setReadonly(bool bState)
{
    ui->scrollAreaHex->setReadonly(bState);

    ui->checkBoxReadonly->setChecked(bState);

    //    for (qint32 i = 0; i < __LIED_size; i++) {
    //        if (g_lineEdit[i]) {
    //            g_lineEdit[i]->setReadOnly(bState);
    //        }
    //    }
}

void XHexViewWidget::setReadonlyVisible(bool bState)
{
    if (bState) {
        ui->checkBoxReadonly->show();
    } else {
        ui->checkBoxReadonly->hide();
    }
}

qint64 XHexViewWidget::getStartAddress()
{
    return ui->scrollAreaHex->getStartAddress();
}

void XHexViewWidget::setSelection(qint64 nOffset, qint64 nSize)
{
    ui->scrollAreaHex->setDeviceSelection(nOffset, nSize);
    ui->scrollAreaHex->goToOffset(nOffset);
}

void XHexViewWidget::setLocation(quint64 nLocation, qint32 nLocationType, qint64 nSize)
{
    ui->scrollAreaHex->setLocation(nLocation, nLocationType, nSize);
}

void XHexViewWidget::adjustView()
{
}

void XHexViewWidget::setWidgetFocus()
{
    ui->scrollAreaHex->setFocus();
}

void XHexViewWidget::reloadData(bool bSaveSelection)
{
    ui->scrollAreaHex->reload(bSaveSelection);
}

void XHexViewWidget::reloadFileType()
{
    if (g_pDevice) {
        const bool bBlocked1 = ui->comboBoxMapMode->blockSignals(true);

        g_options.fileType = (XBinary::FT)(ui->comboBoxType->currentData().toInt());

        XHexView::OPTIONS options = {};
        options.nStartAddress = g_options.nStartAddress;
        options.bMenu_MainHex = g_options.bMenu_MainHex;
        options.bMenu_Disasm = g_options.bMenu_Disasm;
        options.bMenu_MemoryMap = g_options.bMenu_MemoryMap;
        // options.bHideReadOnly = g_options.bHideReadOnly;

        if (g_options.fileType == XBinary::FT_REGION) {
            options.memoryMapRegion = XFormats::getMemoryMap(g_options.fileType, XBinary::MAPMODE_UNKNOWN, g_pDevice, true, g_options.nStartAddress);
        } else {
            options.memoryMapRegion = XFormats::getMemoryMap(g_options.fileType, (XBinary::MAPMODE)(ui->comboBoxMapMode->currentData().toInt()), g_pDevice);
        }

        // ui->scrollAreaDisasm->setData(g_pDevice, options);

        // TODO Check
        if (ui->scrollAreaHex->getXInfoDB()) {
            ui->scrollAreaHex->getXInfoDB()->setData(g_pDevice, options.memoryMapRegion.fileType);
            //            getSymbols();
        }

        ui->scrollAreaHex->setData(g_pDevice, options, true);
        ui->scrollAreaHex->reload(true);

        ui->comboBoxMapMode->blockSignals(bBlocked1);
    }
}

// void XHexViewWidget::blockSignals(bool bState)
//{
//     _blockSignals((QObject **)g_lineEdit, __LIED_size, bState);
// }

// void XHexViewWidget::addValue(QString sTitle, DATAINS datains, LIED lied)
//{
//     QTableWidgetItem *pItemName = new QTableWidgetItem;
//     pItemName->setText(sTitle);
//     ui->tableWidgetDataInspector->setItem(datains, 0, pItemName);

//    g_lineEdit[lied] = new XLineEditHEX(this);
//    g_lineEdit[lied]->setProperty("STYPE", datains);

//    connect(g_lineEdit[lied], SIGNAL(valueChanged(quint64)), this, SLOT(valueChangedSlot(quint64)));

//    ui->tableWidgetDataInspector->setCellWidget(datains, 1, g_lineEdit[lied]);
//}

void XHexViewWidget::adjust()
{
    XDeviceTableView::DEVICESTATE deviceState = ui->scrollAreaHex->getDeviceState();

    qint32 nLocationBase = ui->scrollAreaHex->getLocationBase();

    XFormats::setComboBoxCurrent(ui->comboBoxLocationBase, nLocationBase);

    bool bIsHEX = (nLocationBase == 16);

    QString sSelectionStart;
    QString sSelectionEnd;
    QString sSelectionSize;

    if (bIsHEX) {
        sSelectionStart = "0x" + XBinary::valueToHexEx(deviceState.nSelectionDeviceOffset);
        sSelectionEnd = "0x" + XBinary::valueToHexEx(deviceState.nSelectionDeviceOffset + deviceState.nSelectionSize);
        sSelectionSize = "0x" + XBinary::valueToHexEx(deviceState.nSelectionSize);
    } else {
        sSelectionStart = QString::number(deviceState.nSelectionDeviceOffset);
        sSelectionEnd = QString::number(deviceState.nSelectionDeviceOffset + deviceState.nSelectionSize);
        sSelectionSize = QString::number(deviceState.nSelectionSize);
    }

    QString sSelection = QString("%1 - %2 : %3").arg(sSelectionStart, sSelectionEnd, sSelectionSize);

    ui->lineEditStatus->setText(sSelection);
}

void XHexViewWidget::viewWidgetsState()
{
    // ui->pushButtonBookmarks->setEnabled(!ui->scrollAreaHex->getViewWidgetState(XDeviceTableView::VIEWWIDGET_BOOKMARKS));
    // ui->pushButtonDataConverter->setEnabled(!ui->scrollAreaHex->getViewWidgetState(XDeviceTableView::VIEWWIDGET_DATACONVERTOR));
    ui->toolButtonDataInspector->setEnabled(!ui->scrollAreaHex->getViewWidgetState(XDeviceTableView::VIEWWIDGET_DATAINSPECTOR));
    // ui->pushButtonMultiSearch->setEnabled(!ui->scrollAreaHex->getViewWidgetState(XDeviceTableView::VIEWWIDGET_MULTISEARCH));
    // ui->pushButtonScripts->setEnabled(!ui->scrollAreaHex->getViewWidgetState(XDeviceTableView::VIEWWIDGET_SCRIPTS));
    ui->toolButtonStrings->setEnabled(!ui->scrollAreaHex->getViewWidgetState(XDeviceTableView::VIEWWIDGET_STRINGS));
    // ui->pushButtonVisualization->setEnabled(!ui->scrollAreaHex->getViewWidgetState(XDeviceTableView::VIEWWIDGET_VISUALIZATION));
}

void XHexViewWidget::registerShortcuts(bool bState)
{
    Q_UNUSED(bState)
}

void XHexViewWidget::on_checkBoxReadonly_toggled(bool bChecked)
{
    ui->scrollAreaHex->setReadonly(bChecked);
    setReadonly(bChecked);
}

// void XHexViewWidget::valueChangedSlot(quint64 nValue)
//{
//     XLineEditHEX *pLineEdit = qobject_cast<XLineEditHEX *>(sender());

//    DATAINS nStype = (DATAINS)(pLineEdit->property("STYPE").toInt());

//    setValue(nValue, nStype);
//}

void XHexViewWidget::on_toolButtonDataInspector_clicked()
{
    // TODO set Readonly
    ui->scrollAreaHex->_showDataInspector();
}

void XHexViewWidget::on_toolButtonStrings_clicked()
{
    ui->scrollAreaHex->_strings();
}

void XHexViewWidget::on_comboBoxType_currentIndexChanged(int nIndex)
{
    Q_UNUSED(nIndex)

    XBinary::FT fileType = (XBinary::FT)(ui->comboBoxType->currentData().toInt());
    XFormats::getMapModesList(fileType, ui->comboBoxMapMode);

    reloadFileType();
}

void XHexViewWidget::on_comboBoxLocationBase_currentIndexChanged(int nIndex)
{
    Q_UNUSED(nIndex)

    ui->scrollAreaHex->setLocationBase(ui->comboBoxLocationBase->currentData().toInt());
}
