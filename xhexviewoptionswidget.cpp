/* Copyright (c) 2020-2025 hors<horsicq@gmail.com>
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
#include "xhexviewoptionswidget.h"

#include "ui_xhexviewoptionswidget.h"

XHexViewOptionsWidget::XHexViewOptionsWidget(QWidget *pParent) : XShortcutsWidget(pParent), ui(new Ui::XHexViewOptionsWidget)
{
    ui->setupUi(this);

    g_pOptions = nullptr;

    setProperty("GROUPID", XOptions::GROUPID_HEX);
}

XHexViewOptionsWidget::~XHexViewOptionsWidget()
{
    delete ui;
}

void XHexViewOptionsWidget::adjustView()
{
    // TODO
}

void XHexViewOptionsWidget::setOptions(XOptions *pOptions)
{
    g_pOptions = pOptions;

    reload();
}

void XHexViewOptionsWidget::save()
{
    g_pOptions->getCheckBox(ui->checkBoxHexLocationColon, XOptions::ID_HEX_LOCATIONCOLON);
}

void XHexViewOptionsWidget::setDefaultValues(XOptions *pOptions)
{
    pOptions->addID(XOptions::ID_HEX_FONT, XOptions::getMonoFont().toString());
    pOptions->addID(XOptions::ID_HEX_LOCATIONCOLON, true);
}

void XHexViewOptionsWidget::reloadData(bool bSaveSelection)
{
    Q_UNUSED(bSaveSelection)
    reload();
}

void XHexViewOptionsWidget::reload()
{
    g_pOptions->setCheckBox(ui->checkBoxHexLocationColon, XOptions::ID_HEX_LOCATIONCOLON);
}

void XHexViewOptionsWidget::registerShortcuts(bool bState)
{
    Q_UNUSED(bState)
}
