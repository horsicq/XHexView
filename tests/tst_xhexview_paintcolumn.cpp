#include <QtTest>
#include <QBuffer>
#include <QImage>
#include <QPainter>

#include "xhexview.h"

class TestableXHexView : public XHexView
{
public:
    explicit TestableXHexView(QWidget *pParent = nullptr) : XHexView(pParent)
    {
    }

    using XHexView::paintColumn;
};

static int countPixels(const QImage &image, bool bBright)
{
    int nResult = 0;

    for (int y = 0; y < image.height(); y++) {
        for (int x = 0; x < image.width(); x++) {
            const QColor color = image.pixelColor(x, y);

            if (color.alpha() == 0) {
                continue;
            }

            const int nLuma = qGray(color.rgb());

            if ((bBright && (nLuma > 160)) || ((!bBright) && (nLuma < 40))) {
                nResult++;
            }
        }
    }

    return nResult;
}

static QImage renderColumn(TestableXHexView *pView, qint32 nColumn, qint32 nWidth, qint32 nHeight, const QColor &color)
{
    QImage image(nWidth, nHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setPen(color);
    painter.setFont(pView->getTextFont());
    pView->paintColumn(&painter, nColumn, 0, 0, nWidth, nHeight);
    painter.end();

    return image;
}

class XHexViewPaintColumnTest : public QObject
{
    Q_OBJECT

private slots:
    void textUsesPainterPen();
};

void XHexViewPaintColumnTest::textUsesPainterPen()
{
    QByteArray baData(256, '\0');

    for (int i = 0; i < baData.size(); i++) {
        baData[i] = (char)i;
    }

    QBuffer buffer(&baData);
    QVERIFY(buffer.open(QIODevice::ReadOnly));

    TestableXHexView view;
    view.setActive(true);
    view.resize(640, 480);
    view.viewport()->resize(640, 480);
    view.adjust(true);

    XBinaryView::OPTIONS options = {};
    options.fileType = XBinary::FT_BINARY;
    options.nStartSelectionOffset = -1;
    view.setData(&buffer, options, true);

    QCOMPARE(view.getBinaryView()->getViewSize(), (qint64)baData.size());

    const QColor colorLight(230, 230, 230);
    const QImage imageLight = renderColumn(&view, 1, 480, 120, colorLight);
    QVERIFY(countPixels(imageLight, true) > 0);
    QCOMPARE(countPixels(imageLight, false), 0);

    const QColor colorDark(0, 0, 0);
    const QImage imageDark = renderColumn(&view, 1, 480, 120, colorDark);
    QVERIFY(countPixels(imageDark, false) > 0);
}

QTEST_MAIN(XHexViewPaintColumnTest)

#include "tst_xhexview_paintcolumn.moc"
