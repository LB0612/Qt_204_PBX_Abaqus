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

constexpr qreal kPtCoverTitle = 22.0;
constexpr qreal kPtCoverInfo = 14.0;
constexpr qreal kPtHeading1 = 16.0;
constexpr qreal kPtHeading2 = 14.0;
constexpr qreal kPtFigureLabel = 13.0;
constexpr qreal kPtBody = 11.0;
constexpr qreal kPtTable = 10.0;
constexpr qreal kPtHeaderFooter = 9.0;

qreal mmToPx(qreal mm)
{
    return mm * static_cast<qreal>(kPdfDpi) / 25.4;
}

QFont reportFont(qreal pointSize, bool bold = false)
{
    QFont font(QStringLiteral("Microsoft YaHei"));
    font.setPointSizeF(pointSize);
    font.setBold(bold);
    return font;
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

struct PdfDrawContext
{
    QPainter *painter = nullptr;
    QPdfWriter *writer = nullptr;
    PageGeom geom;
    QString projectName;
    int bodyPageIndex = 1;
    int bodyTotalPages = 1;
    bool coverDrawn = false;
    bool bodyStarted = false;
    bool dryRun = false;
};

QFontMetricsF metricsFor(
    const PdfDrawContext &ctx,
    const QFont &font)
{
    if (ctx.painter && ctx.painter->isActive()) {
        ctx.painter->setFont(font);
        return QFontMetricsF(ctx.painter->fontMetrics());
    }

    // Match QPdfWriter DPI for dry-run measurement.
    QImage probe(8, 8, QImage::Format_ARGB32);
    const int dpm = qRound(kPdfDpi / 0.0254);
    probe.setDotsPerMeterX(dpm);
    probe.setDotsPerMeterY(dpm);
    QPainter probePainter(&probe);
    probePainter.setFont(font);
    return QFontMetricsF(probePainter.fontMetrics());
}

void drawHeaderFooter(
    PdfDrawContext &ctx,
    bool isCover)
{
    if (ctx.dryRun || isCover || !ctx.painter) {
        return;
    }

    QPainter &painter = *ctx.painter;
    const PageGeom &geom = ctx.geom;

    painter.save();
    painter.setFont(reportFont(kPtHeaderFooter));
    painter.setPen(QColor(QStringLiteral("#555555")));

    const qreal headerY = mmToPx(8.0);
    const qreal headerH = mmToPx(8.0);
    painter.drawText(
        QRectF(geom.left, headerY, geom.contentW * 0.55, headerH),
        Qt::AlignLeft | Qt::AlignVCenter,
        QStringLiteral("PBX浇注固化仿真分析报告")
    );
    painter.drawText(
        QRectF(
            geom.left + geom.contentW * 0.45,
            headerY,
            geom.contentW * 0.55,
            headerH
        ),
        Qt::AlignRight | Qt::AlignVCenter,
        ctx.projectName
    );

    painter.drawLine(
        QPointF(geom.left, mmToPx(16.0)),
        QPointF(geom.pageW - geom.right, mmToPx(16.0))
    );

    painter.drawText(
        QRectF(
            geom.left,
            geom.pageH - mmToPx(14.0),
            geom.contentW,
            mmToPx(8.0)
        ),
        Qt::AlignCenter | Qt::AlignVCenter,
        QStringLiteral("第 %1 页 / 共 %2 页")
            .arg(ctx.bodyPageIndex)
            .arg(ctx.bodyTotalPages)
    );
    painter.restore();
}

bool beginBodyPage(PdfDrawContext &ctx)
{
    if (ctx.bodyStarted) {
        ++ctx.bodyPageIndex;
        if (!ctx.dryRun) {
            if (!ctx.writer || !ctx.writer->newPage()) {
                return false;
            }
        }
    } else if (ctx.coverDrawn) {
        if (!ctx.dryRun) {
            if (!ctx.writer || !ctx.writer->newPage()) {
                return false;
            }
        }
    }

    ctx.bodyStarted = true;

    if (!ctx.dryRun && ctx.painter) {
        ctx.painter->fillRect(
            QRectF(0, 0, ctx.geom.pageW, ctx.geom.pageH),
            Qt::white
        );
        drawHeaderFooter(ctx, false);
    }
    return true;
}

void drawTitle(
    PdfDrawContext &ctx,
    qreal &y,
    const QString &text,
    qreal pointSize,
    bool bold = true)
{
    const QFont font = reportFont(pointSize, bold);
    const QFontMetricsF metrics = metricsFor(ctx, font);
    const qreal h = metrics.height() + mmToPx(2.0);

    if (!ctx.dryRun && ctx.painter) {
        ctx.painter->setFont(font);
        ctx.painter->setPen(QColor(QStringLiteral("#222222")));
        ctx.painter->drawText(
            QRectF(ctx.geom.left, y, ctx.geom.contentW, h),
            Qt::AlignLeft | Qt::AlignVCenter,
            text
        );
    }

    y += h + mmToPx(3.0);
}

void drawParagraph(
    PdfDrawContext &ctx,
    qreal &y,
    const QString &text,
    qreal pointSize)
{
    const QFont font = reportFont(pointSize);
    const QFontMetricsF metrics = metricsFor(ctx, font);
    const QRectF bound = metrics.boundingRect(
        QRectF(0, 0, ctx.geom.contentW, ctx.geom.contentH),
        Qt::TextWordWrap,
        text
    );

    if (!ctx.dryRun && ctx.painter) {
        ctx.painter->setFont(font);
        ctx.painter->setPen(QColor(QStringLiteral("#333333")));
        ctx.painter->drawText(
            QRectF(ctx.geom.left, y, ctx.geom.contentW, bound.height()),
            Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
            text
        );
    }

    y += bound.height() + mmToPx(2.0);
}

void drawKeyValue(
    PdfDrawContext &ctx,
    qreal &y,
    const QString &key,
    const QString &value)
{
    drawParagraph(
        ctx,
        y,
        key + QStringLiteral("：") + value,
        kPtCoverInfo
    );
}

bool drawTableRow(
    PdfDrawContext &ctx,
    qreal &y,
    qreal rowH,
    qreal col1,
    qreal col2,
    qreal col3,
    const QString &c1,
    const QString &c2,
    const QString &c3,
    bool header)
{
    if (!ctx.dryRun && ctx.painter) {
        QPainter &painter = *ctx.painter;
        const QRectF r1(ctx.geom.left, y, col1, rowH);
        const QRectF r2(ctx.geom.left + col1, y, col2, rowH);
        const QRectF r3(ctx.geom.left + col1 + col2, y, col3, rowH);

        if (header) {
            painter.fillRect(
                QRectF(ctx.geom.left, y, ctx.geom.contentW, rowH),
                QColor(QStringLiteral("#f2f2f2"))
            );
        }

        painter.setPen(QColor(QStringLiteral("#cccccc")));
        painter.drawRect(r1);
        painter.drawRect(r2);
        painter.drawRect(r3);

        QFont rowFont = reportFont(kPtTable, header);
        painter.setFont(rowFont);
        painter.setPen(QColor(QStringLiteral("#333333")));
        painter.drawText(
            r1.adjusted(4, 0, -4, 0),
            Qt::AlignVCenter | Qt::AlignLeft,
            c1
        );
        painter.drawText(
            r2.adjusted(4, 0, -4, 0),
            Qt::AlignVCenter | Qt::AlignLeft,
            c2
        );
        painter.drawText(
            r3.adjusted(4, 0, -4, 0),
            Qt::AlignVCenter | Qt::AlignLeft,
            c3
        );
    }

    y += rowH;
    return true;
}

bool drawTable(
    PdfDrawContext &ctx,
    qreal &y,
    const SimulationReportTable &table)
{
    const QFont titleFont = reportFont(kPtHeading2, true);
    const QFontMetricsF titleMetrics = metricsFor(ctx, titleFont);
    const qreal titleH = titleMetrics.height() + mmToPx(2.0);
    const QFont tableFont = reportFont(kPtTable);
    const QFontMetricsF tableMetrics = metricsFor(ctx, tableFont);
    const qreal rowH = tableMetrics.height() + mmToPx(4.0);

    // Title + header + at least one data row, otherwise start a new page.
    if (y + titleH + rowH * 2.0 > ctx.geom.pageH - ctx.geom.bottom) {
        if (!beginBodyPage(ctx)) {
            return false;
        }
        y = ctx.geom.top;
    }

    drawTitle(ctx, y, table.title, kPtHeading2, true);

    const qreal col1 = ctx.geom.contentW * 0.34;
    const qreal col2 = ctx.geom.contentW * 0.42;
    const qreal col3 = ctx.geom.contentW * 0.24;

    auto drawHeaderRow = [&]() -> bool {
        return drawTableRow(
            ctx,
            y,
            rowH,
            col1,
            col2,
            col3,
            QStringLiteral("参数"),
            QStringLiteral("数值"),
            QStringLiteral("单位"),
            true
        );
    };

    if (!drawHeaderRow()) {
        return false;
    }

    for (const SimulationReportRow &row : table.rows) {
        if (y + rowH > ctx.geom.pageH - ctx.geom.bottom) {
            if (!beginBodyPage(ctx)) {
                return false;
            }
            y = ctx.geom.top;
            if (!drawHeaderRow()) {
                return false;
            }
        }

        if (!drawTableRow(
                ctx,
                y,
                rowH,
                col1,
                col2,
                col3,
                row.name,
                row.value,
                row.unit,
                false)) {
            return false;
        }
    }

    y += mmToPx(4.0);
    return true;
}

bool layoutReport(
    PdfDrawContext &ctx,
    const SimulationReportModel &model,
    QString &errorMessage)
{
    // Cover
    {
        qreal y = mmToPx(70.0);
        if (!ctx.dryRun && ctx.painter) {
            ctx.painter->fillRect(
                QRectF(0, 0, ctx.geom.pageW, ctx.geom.pageH),
                Qt::white
            );
        }
        drawTitle(ctx, y, model.reportTitle, kPtCoverTitle, true);
        y += mmToPx(10.0);
        drawKeyValue(ctx, y, QStringLiteral("工程名称"), model.projectName);
        drawKeyValue(ctx, y, QStringLiteral("Job名称"), model.jobName);
        drawKeyValue(ctx, y, QStringLiteral("软件名称"), model.productName);
        drawKeyValue(ctx, y, QStringLiteral("软件版本"), model.appVersion);
        drawKeyValue(ctx, y, QStringLiteral("生成时间"), model.generatedAt);
        ctx.coverDrawn = true;
    }

    // 1 Overview
    if (!beginBodyPage(ctx)) {
        errorMessage = QStringLiteral("无法创建 PDF 新页。");
        return false;
    }
    {
        qreal y = ctx.geom.top;
        drawTitle(ctx, y, QStringLiteral("1 仿真概况"), kPtHeading1);
        SimulationReportTable overview;
        overview.title = QStringLiteral("概况");
        overview.rows = model.overviewRows;
        if (!drawTable(ctx, y, overview)) {
            errorMessage = QStringLiteral("无法绘制仿真概况表格。");
            return false;
        }
    }

    // 2 Parameters
    if (!beginBodyPage(ctx)) {
        errorMessage = QStringLiteral("无法创建 PDF 新页。");
        return false;
    }
    {
        qreal y = ctx.geom.top;
        drawTitle(ctx, y, QStringLiteral("2 输入参数"), kPtHeading1);
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
            if (!drawTable(ctx, y, table)) {
                errorMessage = QStringLiteral("无法绘制输入参数表格。");
                return false;
            }
        }
    }

    // 3/4/5 figures
    int sectionNumber = 3;
    for (const SimulationReportResultSection &section : model.resultSections) {
        for (const SimulationReportFigure &figure : section.figures) {
            if (!beginBodyPage(ctx)) {
                errorMessage = QStringLiteral("无法创建 PDF 新页。");
                return false;
            }

            qreal y = ctx.geom.top;
            drawTitle(
                ctx,
                y,
                QStringLiteral("%1 %2")
                    .arg(sectionNumber)
                    .arg(section.title),
                kPtHeading1
            );
            drawParagraph(ctx, y, figure.label, kPtFigureLabel);
            drawParagraph(ctx, y, figure.frameText, kPtBody);
            if (!figure.timeText.isEmpty()) {
                drawParagraph(ctx, y, figure.timeText, kPtBody);
            }

            if (!ctx.dryRun) {
                QImage image(figure.imagePath);
                if (image.isNull()) {
                    errorMessage =
                        QStringLiteral("无法加载报告图片：\n%1")
                            .arg(figure.imagePath);
                    return false;
                }

                const qreal maxW = ctx.geom.contentW;
                const qreal maxH =
                    ctx.geom.pageH - ctx.geom.bottom - y - mmToPx(4.0);
                const QSizeF fitted =
                    fitImage(figure.imagePixelSize, maxW, maxH);
                if (!fitted.isValid()) {
                    errorMessage = QStringLiteral("报告图片尺寸无效。");
                    return false;
                }

                const QRectF target(
                    ctx.geom.left
                        + (ctx.geom.contentW - fitted.width()) / 2.0,
                    y,
                    fitted.width(),
                    fitted.height()
                );
                ctx.painter->drawImage(target, image);
            }
        }
        ++sectionNumber;
    }

    // 6 Notes
    if (!beginBodyPage(ctx)) {
        errorMessage = QStringLiteral("无法创建 PDF 新页。");
        return false;
    }
    {
        qreal y = ctx.geom.top;
        drawTitle(ctx, y, QStringLiteral("6 结果文件说明"), kPtHeading1);
        for (const QString &note : model.notes) {
            drawParagraph(
                ctx,
                y,
                QStringLiteral("• %1").arg(note),
                kPtBody
            );
        }
    }

    // 7 Trace
    if (!beginBodyPage(ctx)) {
        errorMessage = QStringLiteral("无法创建 PDF 新页。");
        return false;
    }
    {
        qreal y = ctx.geom.top;
        drawTitle(ctx, y, QStringLiteral("7 报告追溯信息"), kPtHeading1);
        SimulationReportTable trace;
        trace.title = QStringLiteral("追溯信息");
        trace.rows = model.traceRows;
        if (!drawTable(ctx, y, trace)) {
            errorMessage = QStringLiteral("无法绘制追溯信息表格。");
            return false;
        }
    }

    return true;
}

} // namespace

bool SimulationReportPdfWriter::write(
    const SimulationReportModel &model,
    const QString &outputPath,
    QString &errorMessage)
{
    PdfDrawContext countCtx;
    countCtx.geom = makeGeom();
    countCtx.projectName = model.projectName;
    countCtx.dryRun = true;
    countCtx.bodyPageIndex = 1;
    countCtx.bodyTotalPages = 1;
    countCtx.coverDrawn = false;
    countCtx.bodyStarted = false;

    if (!layoutReport(countCtx, model, errorMessage)) {
        return false;
    }

    const int bodyTotalPages = countCtx.bodyPageIndex;
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

    PdfDrawContext drawCtx;
    drawCtx.painter = &painter;
    drawCtx.writer = &writer;
    drawCtx.geom = makeGeom();
    drawCtx.projectName = model.projectName;
    drawCtx.dryRun = false;
    drawCtx.bodyPageIndex = 1;
    drawCtx.bodyTotalPages = bodyTotalPages;
    drawCtx.coverDrawn = false;
    drawCtx.bodyStarted = false;

    if (!layoutReport(drawCtx, model, errorMessage)) {
        return false;
    }

    painter.end();
    errorMessage.clear();
    return true;
}
