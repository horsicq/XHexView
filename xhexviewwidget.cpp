/* Copyright (c) 2020-2023 hors<horsicq@gmail.com>
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

    connect(ui->scrollAreaHex, SIGNAL(showOffsetDisasm(qint64)), this, SIGNAL(showOffsetDisasm(qint64)));
    connect(ui->scrollAreaHex, SIGNAL(showOffsetMemoryMap(qint64)), this, SIGNAL(showOffsetMemoryMap(qint64)));
    connect(ui->scrollAreaHex, SIGNAL(errorMessage(QString)), this, SLOT(errorMessageSlot(QString)));
    connect(ui->scrollAreaHex, SIGNAL(cursorViewOffsetChanged(qint64)), this, SLOT(cursorChangedSlot(qint64)));
    connect(ui->scrollAreaHex, SIGNAL(deviceSelectionChanged(qint64, qint64)), this, SIGNAL(selectionChanged(qint64, qint64)));
    connect(ui->scrollAreaHex, SIGNAL(dataChanged(qint64, qint64)), this, SLOT(dataChangedSlot(qint64, qint64)));

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

void XHexViewWidget::setData(QIODevice *pDevice, XHexView::OPTIONS options)
{
    //    g_bIsEdited = false;

    ui->checkBoxReadonly->setEnabled(pDevice->isWritable());

    ui->scrollAreaHex->setData(pDevice, options, true);
}

void XHexViewWidget::setDevice(QIODevice *pDevice)
{
    ui->scrollAreaHex->setDevice(pDevice);
}

void XHexViewWidget::setBackupDevice(QIODevice *pDevice)
{
    ui->scrollAreaHex->setBackupDevice(pDevice);
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
    ui->scrollAreaHex->setBackupDevice(nullptr);
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

void XHexViewWidget::setEdited(qint64 nDeviceOffset, qint64 nDeviceSize)
{
    ui->scrollAreaHex->setEdited(nDeviceOffset, nDeviceSize);

    //    emit changed(); // TODO Check
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

void XHexViewWidget::cursorChangedSlot(qint64 nOffset)
{
    Q_UNUSED(nOffset)

    adjust();

    //    XDeviceTableView::DEVICESTATE deviceState = ui->scrollAreaHex->getDeviceState();

    //    emit selectionChanged(deviceState.nSelectionOffset, deviceState.nSelectionSize);
}

void XHexViewWidget::selectionChangedSlot()
{
    adjust();

    XDeviceTableView::DEVICESTATE deviceState = ui->scrollAreaHex->getDeviceState();

    emit selectionChanged(deviceState.nSelectionDeviceOffset, deviceState.nSelectionSize);
}

void XHexViewWidget::adjust()
{
    XDeviceTableView::DEVICESTATE deviceState = ui->scrollAreaHex->getDeviceState();

    //    QString sCursor = XBinary::valueToHex(state.nCursorViewOffset);
    QString sSelectionStart = XBinary::valueToHex(deviceState.nSelectionDeviceOffset);
    QString sSelectionSize = XBinary::valueToHex(deviceState.nSelectionSize);

    QString sSelection;

    sSelection = QString("%1:%2 %3:%4").arg(tr("Selection"), sSelectionStart, tr("Size"), sSelectionSize);

    ui->labelSelectionStatus->setText(sSelection);
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

void XHexViewWidget::on_pushButtonDataInspector_clicked()
{
    // TODO set Readonly
    ui->pushButtonDataInspector->setEnabled(false);  // TODO

    ui->scrollAreaHex->_showDataInspector();

    ui->pushButtonDataInspector->setEnabled(true);  // TODO
}

void XHexViewWidget::dataChangedSlot(qint64 nDeviceOffset, qint64 nDeviceSize)
{
    //    qDebug("void XHexViewWidget::dataChangedSlot(qint64 nDeviceOffset, qint64 nDeviceSize)");

    ui->scrollAreaHex->setEdited(nDeviceOffset, nDeviceSize);

    adjust();

    //    emit dataChanged(nDeviceOffset, nDeviceSize);
}
