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
#include "dialoghexview.h"

#include "ui_dialoghexview.h"

DialogHexView::DialogHexView(QWidget *pParent) : XShortcutsDialog(pParent, true), ui(new Ui::DialogHexView)
{
    ui->setupUi(this);

    connect(ui->widgetHex, SIGNAL(dataChanged(qint64, qint64)), this, SIGNAL(dataChanged(qint64, qint64)));
    connect(ui->widgetHex, SIGNAL(deviceSizeChanged(qint64, qint64)), this, SIGNAL(deviceSizeChanged(qint64, qint64)));

    ui->widgetHex->setReadonlyVisible(true);
}

void DialogHexView::setData(QIODevice *pDevice, const XHexViewWidget::OPTIONS &options)
{
    ui->widgetHex->setData(pDevice, options);

    if (options.sTitle != "") {
        setWindowTitle(options.sTitle);
    }
}

DialogHexView::~DialogHexView()
{
    delete ui;
}

void DialogHexView::adjustView()
{
}

void DialogHexView::setGlobal(XShortcuts *pShortcuts, XOptions *pXOptions)
{
    ui->widgetHex->setGlobal(pShortcuts, pXOptions);
    XShortcutsDialog::setGlobal(pShortcuts, pXOptions);
}

void DialogHexView::setXInfoDB(XInfoDB *pXInfoDB)
{
    ui->widgetHex->setXInfoDB(pXInfoDB);
}

void DialogHexView::on_pushButtonClose_clicked()
{
    this->close();
}

void DialogHexView::registerShortcuts(bool bState)
{
    Q_UNUSED(bState)
}
