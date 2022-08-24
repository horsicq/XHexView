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
#include "xhexviewwidget.h"
#include "ui_xhexviewwidget.h"

XHexViewWidget::XHexViewWidget(QWidget *pParent) :
    XShortcutsWidget(pParent),
    ui(new Ui::XHexViewWidget)
{
    ui->setupUi(this);

    memset(g_lineEdit,0,sizeof g_lineEdit);
    g_bIsEdited=false;

    connect(ui->scrollAreaHex,SIGNAL(showOffsetDisasm(qint64)),this,SIGNAL(showOffsetDisasm(qint64)));
    connect(ui->scrollAreaHex,SIGNAL(showOffsetMemoryMap(qint64)),this,SIGNAL(showOffsetMemoryMap(qint64)));
    connect(ui->scrollAreaHex,SIGNAL(errorMessage(QString)),this,SLOT(errorMessageSlot(QString)));
    connect(ui->scrollAreaHex,SIGNAL(cursorChanged(qint64)),this,SLOT(cursorChanged(qint64)));
    connect(ui->scrollAreaHex,SIGNAL(selectionChanged()),this,SLOT(selectionChanged()));
    connect(ui->scrollAreaHex,SIGNAL(dataChanged()),this,SIGNAL(dataChanged()));

    setReadonlyVisible(false);

    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setRowCount(__LIED_size);

    QStringList slHeader;
    slHeader.append(tr("Name"));
    slHeader.append(tr("Value"));

    ui->tableWidget->setHorizontalHeaderLabels(slHeader);
    ui->tableWidget->horizontalHeader()->setVisible(true);

    ui->tableWidget->setColumnWidth(0,100);

    addValue("BYTE",DATAINS_BYTE,LIED_BYTE);
    addValue("WORD",DATAINS_WORD,LIED_WORD);
    addValue("DWORD",DATAINS_DWORD,LIED_DWORD);
    addValue("QWORD",DATAINS_QWORD,LIED_QWORD);
    addValue("uint8",DATAINS_UINT8,LIED_UINT8);
    addValue("int8",DATAINS_INT8,LIED_INT8);
    addValue("uint16",DATAINS_UINT16,LIED_UINT16);
    addValue("int16",DATAINS_INT16,LIED_INT16);
    addValue("uint32",DATAINS_UINT32,LIED_UINT32);
    addValue("int32",DATAINS_INT32,LIED_INT32);
    addValue("uint64",DATAINS_UINT64,LIED_UINT64);
    addValue("int64",DATAINS_INT64,LIED_INT64);

    setReadonly(true);
}

XHexViewWidget::~XHexViewWidget()
{
    delete ui;
}

void XHexViewWidget::setGlobal(XShortcuts *pShortcuts,XOptions *pXOptions)
{
    ui->scrollAreaHex->setGlobal(pShortcuts,pXOptions);
    XShortcutsWidget::setGlobal(pShortcuts,pXOptions);
}

void XHexViewWidget::setData(QIODevice *pDevice,XHexView::OPTIONS options)
{
    g_bIsEdited=false;

    ui->checkBoxReadonly->setEnabled(pDevice->isWritable());

    ui->scrollAreaHex->setData(pDevice,options);
}

void XHexViewWidget::setDevice(QIODevice *pDevice)
{
    ui->scrollAreaHex->setDevice(pDevice);
}

void XHexViewWidget::setBackupDevice(QIODevice *pDevice)
{
    ui->scrollAreaHex->setBackupDevice(pDevice);
}

void XHexViewWidget::reload()
{
    ui->scrollAreaHex->reload(true);
}

void XHexViewWidget::setReadonly(bool bState)
{
    ui->scrollAreaHex->setReadonly(bState);

    ui->checkBoxReadonly->setChecked(bState);

    for(int i=0;i<__LIED_size;i++)
    {
        if(g_lineEdit[i])
        {
            g_lineEdit[i]->setReadOnly(bState);
        }
    }
}

void XHexViewWidget::setReadonlyVisible(bool bState)
{
    if(bState)
    {
        ui->checkBoxReadonly->show();
    }
    else
    {
        ui->checkBoxReadonly->hide();
    }
}

void XHexViewWidget::setEdited()
{
    ui->scrollAreaHex->setEdited();

//    emit changed();
}

qint64 XHexViewWidget::getStartAddress()
{
    return ui->scrollAreaHex->getStartAddress();
}

void XHexViewWidget::setSelection(qint64 nOffset,qint64 nSize)
{
    ui->scrollAreaHex->setSelection(nOffset,nSize);
    ui->scrollAreaHex->goToOffset(nOffset);
}

void XHexViewWidget::blockSignals(bool bState)
{
    _blockSignals((QObject **)g_lineEdit,__LIED_size,bState);
}

void XHexViewWidget::addValue(QString sTitle,DATAINS datains,LIED lied)
{
    QTableWidgetItem *pItemName=new QTableWidgetItem;
    pItemName->setText(sTitle);
    ui->tableWidget->setItem(datains,0,pItemName);

    g_lineEdit[lied]=new XLineEditHEX(this);
    g_lineEdit[lied]->setProperty("STYPE",datains);

    connect(g_lineEdit[lied],SIGNAL(valueChanged(quint64)),this,SLOT(valueChangedSlot(quint64)));

    ui->tableWidget->setCellWidget(datains,1,g_lineEdit[lied]);
}

void XHexViewWidget::cursorChanged(qint64 nOffset)
{
    Q_UNUSED(nOffset)

    adjust();
}

void XHexViewWidget::selectionChanged()
{
    adjust();
}

void XHexViewWidget::adjust()
{
    XAbstractTableView::STATE state=ui->scrollAreaHex->getState();

    QString sCursor=XBinary::valueToHex(state.nCursorOffset);
    QString sSelectionStart=XBinary::valueToHex(state.nSelectionOffset);
    QString sSelectionSize=XBinary::valueToHex(state.nSelectionSize);

    QString sSelection;

    sSelection=QString("%1:%2 %3:%4 %5:%6").arg(tr("Cursor"),sCursor,tr("Selection"),sSelectionStart,tr("Size"),sSelectionSize);

    ui->labelSelectionStatus->setText(sSelection);

    // TODO Data Inspector
    //ui->scrollAreaHex->getDevice();
    // TODO optimize
    blockSignals(true);

    XBinary binary(ui->scrollAreaHex->getDevice());

    g_lineEdit[LIED_BYTE]->setValue(binary.read_uint8(state.nSelectionOffset));
    g_lineEdit[LIED_WORD]->setValue(binary.read_uint16(state.nSelectionOffset));
    g_lineEdit[LIED_DWORD]->setValue(binary.read_uint32(state.nSelectionOffset));
    g_lineEdit[LIED_QWORD]->setValue(binary.read_uint64(state.nSelectionOffset));
    g_lineEdit[LIED_UINT8]->setValue(binary.read_uint8(state.nSelectionOffset),HEXValidator::MODE_DEC);
    g_lineEdit[LIED_INT8]->setValue(binary.read_int8(state.nSelectionOffset),HEXValidator::MODE_SIGN_DEC);
    g_lineEdit[LIED_UINT16]->setValue(binary.read_uint16(state.nSelectionOffset),HEXValidator::MODE_DEC);
    g_lineEdit[LIED_INT16]->setValue(binary.read_int16(state.nSelectionOffset),HEXValidator::MODE_SIGN_DEC);
    g_lineEdit[LIED_UINT32]->setValue(binary.read_uint32(state.nSelectionOffset),HEXValidator::MODE_DEC);
    g_lineEdit[LIED_INT32]->setValue(binary.read_int32(state.nSelectionOffset),HEXValidator::MODE_SIGN_DEC);
    g_lineEdit[LIED_UINT64]->setValue(binary.read_uint64(state.nSelectionOffset),HEXValidator::MODE_DEC);
    g_lineEdit[LIED_INT64]->setValue(binary.read_int64(state.nSelectionOffset),HEXValidator::MODE_SIGN_DEC);

    blockSignals(false);
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

void XHexViewWidget::valueChangedSlot(quint64 nValue)
{
    XLineEditHEX *pLineEdit=qobject_cast<XLineEditHEX *>(sender());

    DATAINS nStype=(DATAINS)(pLineEdit->property("STYPE").toInt());

    setValue(nValue,nStype);
}

void XHexViewWidget::setValue(quint64 nValue,DATAINS nType)
{
    QIODevice *pDevice=ui->scrollAreaHex->getDevice();

    bool bSuccess=true;

    if((getGlobalOptions()->isSaveBackup())&&(!g_bIsEdited))
    {
        bSuccess=XBinary::saveBackup(pDevice);
    }

    if(bSuccess)
    {
        if(pDevice->isWritable())
        {
            qint64 nOffset=ui->scrollAreaHex->getState().nSelectionOffset;

            XBinary binary(pDevice);

            if      (nType==DATAINS_BYTE)       binary.write_uint8(nOffset,(quint8)nValue);
            else if (nType==DATAINS_WORD)       binary.write_uint16(nOffset,(quint16)nValue);
            else if (nType==DATAINS_DWORD)      binary.write_uint32(nOffset,(quint32)nValue);
            else if (nType==DATAINS_QWORD)      binary.write_uint64(nOffset,(quint64)nValue);
            else if (nType==DATAINS_UINT8)      binary.write_uint8(nOffset,(quint8)nValue);
            else if (nType==DATAINS_INT8)       binary.write_int8(nOffset,(qint8)nValue);
            else if (nType==DATAINS_UINT16)     binary.write_uint16(nOffset,(quint16)nValue);
            else if (nType==DATAINS_INT16)      binary.write_int16(nOffset,(qint16)nValue);
            else if (nType==DATAINS_UINT32)     binary.write_uint32(nOffset,(quint32)nValue);
            else if (nType==DATAINS_INT32)      binary.write_int32(nOffset,(qint32)nValue);
            else if (nType==DATAINS_UINT64)     binary.write_uint64(nOffset,(quint64)nValue);
            else if (nType==DATAINS_INT64)      binary.write_int64(nOffset,(qint64)nValue);

            g_bIsEdited=true;

            ui->scrollAreaHex->reload(true);

            adjust();
        }
    }
}

