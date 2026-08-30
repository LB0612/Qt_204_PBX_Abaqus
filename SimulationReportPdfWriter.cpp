#include "SimulationReportPdfWriter.h"

#include <QColor>
#include <QFont>
#include <QFontMetricsF>
#include <QImage>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QStringList>
#include <QtGlobal>

namespace {

constexpr int kPdfDpi = 144;

qreal mmToPx(qreal mm)
{
    return mm * static_cast<qreal>(kPdfDpi) / 25.4;
}

QSizeF fitImage(
    const QSize &source,
    qreal maxWidth,
    qreal maxHeight)
{
    if (!source.isValid()
        || source.width() <= 0
        || source.height() <= 0) {
        return QSizeF();
    }

    const qreal widthScale =
        maxWidth / static_cast<qreal>(source.width());
    const qreal heightScale =
        maxHeight / static_cast<qreal>(source.height());
    const qreal scale = qMin(widthScale, heightScale);

    return QSizeF(
        source.width() * scale,
        source.height() * scale
    );
}

struct PageGeom
{
    qreal pageW = 0;
    qreal pageH = 0;
    qreal left = 0;
    qreal right = 0;
    qreal top = 0;
    qreal bottom = 0;
    qreal contentW = 0;
    qreal contentH = 0;
};

PageGeom makeGeom()
{
    PageGeom g;
    g.pageW = mmToPx(210.0);
    g.pageH = mmToPx(297.0);
    g.left = mmToPx(20.0);
    g.right = mmToPx(20.0);
    g.top = mmToPx(22.0);
    g.bottom = mmToPx(20.0);
    g.contentW = g.pageW - g.left - g.right;
    g.contentH = g.pageH - g.top - g.bottom;
    return g;
}

void drawHeaderFooter(
    QPainter &painter,
    const PageGeom &geom,
    const QString &projectName,
    int pageIndex,
    int totalPages,
    bool isCover)
{
    if (isCover) {
        return;
    }

    painter.save();
    QFont headerFont(QStringLiteral("Microsoft YaHei"));
    headerFont.setPixelSize(11);
    painter.setFont(headerFont);
    painter.setPen(QColor(QStringLiteral("#555555")));

    const QString left =
        QStringLiteral("PBX浇注固化仿真分析报告");
    const QString right = projectName;
    painter.drawText(
        QRectF(geom.left, mmToPx(8.0), geom.contentW * 0.55, mmToPx(8.0)),
        Qt::AlignLeft | Qt::AlignVCenter,
        left
    );
    painter.drawText(
        QRectF(
            geom.left + geom.contentW * 0.45,
            mmToPx(8.0),
            geom.contentW * 0.55,
            mmToPx(8.0)
        ),
        Qt::AlignRight | Qt::AlignVCenter,
        right
    );

    painter.drawLine(
        QPointF(geom.left, mmToPx(16.0)),
        QPointF(geom.pageW - geom.right, mmToPx(16.0))
    );

    const QString footer =
        QStringLiteral("第 %1 页 / 共 %2 页")
            .arg(pageIndex)
            .arg(totalPages);
    painter.drawText(
        QRectF(
            geom.left,
            geom.pageH - mmToPx(14.0),
            geom.contentW,
            mmToPx(8.0)
        ),
        Qt::AlignCenter | Qt::AlignVCenter,
        footer
    );
    painter.restore();
}

void drawTitle(
    QPainter &painter,
    qreal &y,
    const PageGeom &geom,
    const QString &text,
    int pixelSize,
    bool bold = true)
{
    QFont font(QStringLiteral("Microsoft YaHei"));
    font.setPixelSize(pixelSize);
    font.setBold(bold);
    painter.setFont(font);
    painter.setPen(QColor(QStringLiteral("#222222")));

    const QFontMetricsF metrics(font);
    const qreal h = metrics.height() + mmToPx(2.0);
    painter.drawText(
        QRectF(geom.left, y, geom.contentW, h),
        Qt::AlignLeft | Qt::AlignVCenter,
        text
    );
    y += h + mmToPx(3.0);
}

void drawParagraph(
    QPainter &painter,
    qreal &y,
    const PageGeom &geom,
    const QString &text,
    int pixelSize = 12)
{
    QFont font(QStringLiteral("Microsoft YaHei"));
    font.setPixelSize(pixelSize);
    painter.setFont(font);
    painter.setPen(QColor(QStringLiteral("#333333")));

    const QFontMetricsF metrics(font);
    const QRectF bound = metrics.boundingRect(
        QRectF(0, 0, geom.contentW, geom.contentH),
        Qt::TextWordWrap,
        text
    );
    painter.drawText(
        QRectF(geom.left, y, geom.contentW, bound.height()),
        Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
        text
    );
    y += bound.height() + mmToPx(2.0);
}

void drawKeyValue(
    QPainter &painter,
    qreal &y,
    const PageGeom &geom,
    const QString &key,
    const QString &value)
{
    drawParagraph(
        painter,
        y,
        geom,
        key + QStringLiteral("：") + value,
        13
    );
}

void drawTable(
    QPainter &painter,
    qreal &y,
    const PageGeom &geom,
    const SimulationReportTable &table)
{
    drawTitle(painter, y, geom, table.title, 14, true);

    QFont font(QStringLiteral("Microsoft YaHei"));
    font.setPixelSize(11);
    painter.setFont(font);
    const QFontMetricsF metrics(font);
    const qreal rowH = metrics.height() + mmToPx(4.0);
    const qreal col1 = geom.contentW * 0.34;
    const qreal col2 = geom.contentW * 0.42;
    const qreal col3 = geom.contentW * 0.24;

    auto drawRow = [&](const QString &c1,
                       const QString &c2,
                       const QString &c3,
                       bool header) {
        if (y + rowH > geom.pageH - geom.bottom) {
            return false;
        }

        const QRectF r1(geom.left, y, col1, rowH);
        const QRectF r2(geom.left + col1, y, col2, rowH);
        const QRectF r3(geom.left + col1 + col2, y, col3, rowH);

        if (header) {
            painter.fillRect(
                QRectF(geom.left, y, geom.contentW, rowH),
                QColor(QStringLiteral("#f2f2f2"))
            );
        }

        painter.setPen(QColor(QStringLiteral("#cccccc")));
        painter.drawRect(r1);
        painter.drawRect(r2);
        painter.drawRect(r3);

        painter.setPen(QColor(QStringLiteral("#333333")));
        QFont rowFont = font;
        rowFont.setBold(header);
        painter.setFont(rowFont);
        painter.drawText(r1.adjusted(4, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft, c1);
        painter.drawText(r2.adjusted(4, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft, c2);
        painter.drawText(r3.adjusted(4, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft, c3);
        y += rowH;
        return true;
    };

    drawRow(
        QStringLiteral("参数"),
        QStringLiteral("数值"),
        QStringLiteral("单位"),
        true
    );

    for (const SimulationReportRow &row : table.rows) {
        drawRow(row.name, row.value, row.unit, false);
    }

    y += mmToPx(4.0);
}

bool beginBodyPage(
    QPainter &painter,
    QPdfWriter &writer,
    bool &firstPage,
    const PageGeom &geom,
    const QString &projectName,
    int pageIndex,
    int totalPages)
{
    if (!firstPage) {
        if (!writer.newPage()) {
            return false;
        }
    }
    firstPage = false;
    painter.fillRect(
        QRectF(0, 0, geom.pageW, geom.pageH),
        Qt::white
    );
    drawHeaderFooter(
        painter,
        geom,
        projectName,
        pageIndex,
        totalPages,
        false
    );
    return true;
}

} // namespace

bool SimulationReportPdfWriter::write(
    const SimulationReportModel &model,
    const QString &outputPath,
    QString &errorMessage)
{
    int figurePages = 0;
    for (const SimulationReportResultSection &section : model.resultSections) {
        figurePages += section.figures.size();
    }

    // cover + body pages (overview + parameters + figures + notes + trace)
    const int bodyTotalPages = 1 + 1 + figurePages + 1 + 1;
    if (bodyTotalPages < 1) {
        errorMessage = QStringLiteral("报告页数无效。");
        return false;
    }

    QPdfWriter writer(outputPath);
    writer.setResolution(kPdfDpi);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(0, 0, 0, 0), QPageLayout::Millimeter);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        errorMessage = QStringLiteral("无法创建 PDF 绘制上下文。");
        return false;
    }

    const PageGeom geom = makeGeom();
    bool firstPage = true;
    int bodyPageIndex = 1;

    // Cover
    {
        painter.fillRect(QRectF(0, 0, geom.pageW, geom.pageH), Qt::white);
        qreal y = mmToPx(70.0);
        drawTitle(painter, y, geom, model.reportTitle, 28, true);
        y += mmToPx(10.0);
        drawKeyValue(painter, y, geom, QStringLiteral("工程名称"), model.projectName);
        drawKeyValue(painter, y, geom, QStringLiteral("Job名称"), model.jobName);
        drawKeyValue(painter, y, geom, QStringLiteral("软件名称"), model.productName);
        drawKeyValue(painter, y, geom, QStringLiteral("软件版本"), model.appVersion);
        drawKeyValue(painter, y, geom, QStringLiteral("生成时间"), model.generatedAt);
        drawHeaderFooter(painter, geom, model.projectName, 0, 0, true);
        firstPage = false;
    }

    // 1 Overview
    if (!beginBodyPage(
            painter,
            writer,
            firstPage,
            geom,
            model.projectName,
            bodyPageIndex,
            bodyTotalPages)) {
        errorMessage = QStringLiteral("无法创建 PDF 新页。");
        return false;
    }
    {
        qreal y = geom.top;
        drawTitle(painter, y, geom, QStringLiteral("1 仿真概况"), 18);
        SimulationReportTable overview;
        overview.title = QStringLiteral("概况");
        overview.rows = model.overviewRows;
        drawTable(painter, y, geom, overview);
    }
    ++bodyPageIndex;

    // 2 Parameters
    if (!beginBodyPage(
            painter,
            writer,
            firstPage,
            geom,
            model.projectName,
            bodyPageIndex,
            bodyTotalPages)) {
        errorMessage = QStringLiteral("无法创建 PDF 新页。");
        return false;
    }
    {
        qreal y = geom.top;
        drawTitle(painter, y, geom, QStringLiteral("2 输入参数"), 18);
        const QStringList prefixes = {
            QStringLiteral("2.1 "),
            QStringLiteral("2.2 "),
            QStringLiteral("2.3 "),
            QStringLiteral("2.4 "),
            QStringLiteral("2.5 "),
        };
        for (int i = 0; i < model.parameterTables.size(); ++i) {
            SimulationReportTable table = model.parameterTables.at(i);
            if (i < prefixes.size()) {
                table.title = prefixes.at(i) + table.title;
            }
            drawTable(painter, y, geom, table);
        }
    }
    ++bodyPageIndex;

    // Result figures: sections 3/4/5
    int sectionNumber = 3;
    for (const SimulationReportResultSection &section : model.resultSections) {
        for (const SimulationReportFigure &figure : section.figures) {
            if (!beginBodyPage(
                    painter,
                    writer,
                    firstPage,
                    geom,
                    model.projectName,
                    bodyPageIndex,
                    bodyTotalPages)) {
                errorMessage = QStringLiteral("无法创建 PDF 新页。");
                return false;
            }

            qreal y = geom.top;
            drawTitle(
                painter,
                y,
                geom,
                QStringLiteral("%1 %2")
                    .arg(sectionNumber)
                    .arg(section.title),
                18
            );
            drawParagraph(painter, y, geom, figure.label, 13);
            drawParagraph(painter, y, geom, figure.frameText, 12);
            if (!figure.timeText.isEmpty()) {
                drawParagraph(painter, y, geom, figure.timeText, 12);
            }

            QImage image(figure.imagePath);
            if (image.isNull()) {
                errorMessage =
                    QStringLiteral("无法加载报告图片：\n%1")
                        .arg(figure.imagePath);
                return false;
            }

            const qreal maxW = geom.contentW;
            const qreal maxH =
                geom.pageH - geom.bottom - y - mmToPx(4.0);
            const QSizeF fitted =
                fitImage(figure.imagePixelSize, maxW, maxH);
            if (!fitted.isValid()) {
                errorMessage = QStringLiteral("报告图片尺寸无效。");
                return false;
            }

            const QRectF target(
                geom.left
                    + (geom.contentW - fitted.width()) / 2.0,
                y,
                fitted.width(),
                fitted.height()
            );
            painter.drawImage(target, image);
            ++bodyPageIndex;
        }
        ++sectionNumber;
    }

    // 6 Notes
    if (!beginBodyPage(
            painter,
            writer,
            firstPage,
            geom,
            model.projectName,
            bodyPageIndex,
            bodyTotalPages)) {
        errorMessage = QStringLiteral("无法创建 PDF 新页。");
        return false;
    }
    {
        qreal y = geom.top;
        drawTitle(painter, y, geom, QStringLiteral("6 结果文件说明"), 18);
        for (const QString &note : model.notes) {
            drawParagraph(
                painter,
                y,
                geom,
                QStringLiteral("• %1").arg(note),
                12
            );
        }
    }
    ++bodyPageIndex;

    // 7 Trace
    if (!beginBodyPage(
            painter,
            writer,
            firstPage,
            geom,
            model.projectName,
            bodyPageIndex,
            bodyTotalPages)) {
        errorMessage = QStringLiteral("无法创建 PDF 新页。");
        return false;
    }
    {
        qreal y = geom.top;
        drawTitle(painter, y, geom, QStringLiteral("7 报告追溯信息"), 18);
        SimulationReportTable trace;
        trace.title = QStringLiteral("追溯信息");
        trace.rows = model.traceRows;
        drawTable(painter, y, geom, trace);
    }

    painter.end();
    errorMessage.clear();
    return true;
}
