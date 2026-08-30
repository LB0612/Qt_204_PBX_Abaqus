#include "SimulationReportPdfWriter.h"

#include "SimulationReportStyle.h"

#include <QColor>
#include <QDateTime>
#include <QFont>
#include <QFontMetricsF>
#include <QImage>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QPen>
#include <QStringList>
#include <QtGlobal>
#include <QtMath>

namespace {

constexpr int kPdfDpi = 144;

using namespace SimulationReportStyle;

qreal mmToPx(qreal mm)
{
    return mm * static_cast<qreal>(kPdfDpi) / 25.4;
}

QFont makeFont(
    const QString &family,
    qreal pointSize,
    bool bold = false)
{
    QFont font(family);
    font.setPointSizeF(pointSize);
    font.setBold(bold);
    font.setStyleStrategy(
        static_cast<QFont::StyleStrategy>(
            QFont::PreferAntialias | QFont::PreferQuality
        )
    );
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

QString formatCoverDate(const QString &generatedAt)
{
    const QDateTime dt = QDateTime::fromString(
        generatedAt,
        QStringLiteral("yyyy-MM-dd HH:mm:ss")
    );
    if (!dt.isValid()) {
        return generatedAt;
    }

    return QStringLiteral("%1 年 %2 月 %3 日")
        .arg(dt.date().year())
        .arg(dt.date().month(), 2, 10, QLatin1Char('0'))
        .arg(dt.date().day(), 2, 10, QLatin1Char('0'));
}

QString compactFrameText(const QString &frameText)
{
    QString text = frameText;
    text.replace(QStringLiteral(" / "), QStringLiteral("/"));
    return text;
}

QString compactTimeText(const QString &timeText)
{
    if (timeText.isEmpty()) {
        return QString();
    }

    QString text = timeText;
    text.replace(QStringLiteral("仿真时间："), QStringLiteral("t="));
    text.replace(QStringLiteral("仿真时间:"), QStringLiteral("t="));
    return text;
}

QString figureCaption(
    int sectionNumber,
    int figureIndex,
    const QString &sectionTitle,
    const SimulationReportFigure &figure)
{
    QString caption =
        QStringLiteral("图 %1.%2 %3（%4，%5")
            .arg(sectionNumber)
            .arg(figureIndex)
            .arg(sectionTitle)
            .arg(figure.label)
            .arg(compactFrameText(figure.frameText));

    const QString timePart = compactTimeText(figure.timeText);
    if (!timePart.isEmpty()) {
        caption += QStringLiteral("，%1").arg(timePart);
    }
    caption += QStringLiteral("）");
    return caption;
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
    g.pageW = mmToPx(PageWidthMm);
    g.pageH = mmToPx(PageHeightMm);
    g.left = mmToPx(MarginLeftMm);
    g.right = mmToPx(MarginRightMm);
    g.top = mmToPx(MarginTopMm);
    g.bottom = mmToPx(MarginBottomMm);
    g.contentW = g.pageW - g.left - g.right;
    g.contentH = g.pageH - g.top - g.bottom;
    return g;
}

struct PdfDrawContext
{
    QPainter *painter = nullptr;
    QPdfWriter *writer = nullptr;
    PageGeom geom;
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

    QImage probe(8, 8, QImage::Format_ARGB32);
    const int dpm = qRound(kPdfDpi / 0.0254);
    probe.setDotsPerMeterX(dpm);
    probe.setDotsPerMeterY(dpm);
    QPainter probePainter(&probe);
    probePainter.setFont(font);
    return QFontMetricsF(probePainter.fontMetrics());
}

void drawHeaderFooter(PdfDrawContext &ctx, bool isCover)
{
    if (ctx.dryRun || isCover || !ctx.painter) {
        return;
    }

    QPainter &painter = *ctx.painter;
    const PageGeom &geom = ctx.geom;

    painter.save();
    painter.setFont(
        makeFont(QStringLiteral("SimHei"), HeaderPt, true)
    );
    painter.setPen(QColor(QStringLiteral("#222222")));

    const qreal headerY = mmToPx(HeaderMm) - mmToPx(8.0);
    painter.drawText(
        QRectF(geom.left, headerY, geom.contentW, mmToPx(8.0)),
        Qt::AlignCenter | Qt::AlignVCenter,
        QStringLiteral("PBX浇注固化仿真分析报告")
    );

    const qreal lineY = mmToPx(HeaderMm) - mmToPx(2.0);
    painter.setPen(QPen(QColor(QStringLiteral("#333333")), 1.0));
    painter.drawLine(
        QPointF(geom.left, lineY),
        QPointF(geom.pageW - geom.right, lineY)
    );

    painter.setFont(
        makeFont(QStringLiteral("SimSun"), FooterPt)
    );
    painter.setPen(QColor(QStringLiteral("#222222")));
    painter.drawText(
        QRectF(
            geom.left,
            geom.pageH - mmToPx(FooterMm),
            geom.contentW,
            mmToPx(8.0)
        ),
        Qt::AlignRight | Qt::AlignVCenter,
        QStringLiteral("第 %1 页 共 %2 页")
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

void drawCenteredText(
    PdfDrawContext &ctx,
    qreal &y,
    const QString &text,
    const QFont &font,
    qreal extraGapMm = 3.0)
{
    const QFontMetricsF metrics = metricsFor(ctx, font);
    const QRectF bound = metrics.boundingRect(
        QRectF(0, 0, ctx.geom.contentW, ctx.geom.contentH),
        Qt::TextWordWrap,
        text
    );
    const qreal h = bound.height();

    if (!ctx.dryRun && ctx.painter) {
        ctx.painter->setFont(font);
        ctx.painter->setPen(QColor(QStringLiteral("#222222")));
        ctx.painter->drawText(
            QRectF(ctx.geom.left, y, ctx.geom.contentW, h),
            Qt::TextWordWrap | Qt::AlignHCenter | Qt::AlignTop,
            text
        );
    }

    y += h + mmToPx(extraGapMm);
}

void drawLeftText(
    PdfDrawContext &ctx,
    qreal &y,
    const QString &text,
    const QFont &font,
    qreal lineHeightFactor = 1.0,
    qreal extraGapMm = 3.0)
{
    const QFontMetricsF metrics = metricsFor(ctx, font);
    const qreal lineH = metrics.height() * lineHeightFactor;
    const QRectF bound = metrics.boundingRect(
        QRectF(0, 0, ctx.geom.contentW, ctx.geom.contentH),
        Qt::TextWordWrap,
        text
    );
    const qreal h = qMax(bound.height(), lineH);

    if (!ctx.dryRun && ctx.painter) {
        ctx.painter->setFont(font);
        ctx.painter->setPen(QColor(QStringLiteral("#222222")));
        ctx.painter->drawText(
            QRectF(ctx.geom.left, y, ctx.geom.contentW, h),
            Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
            text
        );
    }

    y += h + mmToPx(extraGapMm);
}

void drawBodyParagraph(
    PdfDrawContext &ctx,
    qreal &y,
    const QString &text)
{
    const QFont font =
        makeFont(QStringLiteral("SimSun"), BodyPt);
    const QFontMetricsF metrics = metricsFor(ctx, font);
    const qreal firstIndent = metrics.horizontalAdvance(
        QStringLiteral("缩进")
    );
    const QRectF bound = metrics.boundingRect(
        QRectF(
            0,
            0,
            ctx.geom.contentW - firstIndent,
            ctx.geom.contentH
        ),
        Qt::TextWordWrap,
        text
    );
    const qreal lineSpacing = metrics.height() * 1.5;
    const int lineCount = qMax(
        1,
        qCeil(bound.height() / metrics.height())
    );
    const qreal h = lineCount * lineSpacing;

    if (!ctx.dryRun && ctx.painter) {
        ctx.painter->setFont(font);
        ctx.painter->setPen(QColor(QStringLiteral("#222222")));
        ctx.painter->drawText(
            QRectF(
                ctx.geom.left + firstIndent,
                y,
                ctx.geom.contentW - firstIndent,
                h
            ),
            Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
            text
        );
    }

    y += h + mmToPx(2.0);
}

bool drawTable(
    PdfDrawContext &ctx,
    qreal &y,
    const QString &caption,
    const SimulationReportTable &table)
{
    const QFont captionFont =
        makeFont(QStringLiteral("SimSun"), CaptionPt);
    const QFont tableFont =
        makeFont(QStringLiteral("SimSun"), TablePt);
    const QFontMetricsF captionMetrics =
        metricsFor(ctx, captionFont);

    const qreal captionH = captionMetrics.height() + mmToPx(2.0);
    const qreal cellPadY = mmToPx(2.0);
    const qreal tableW = ctx.geom.contentW * TableWidthPct;
    const qreal tableLeft =
        ctx.geom.left + (ctx.geom.contentW - tableW) / 2.0;
    const qreal col1 = tableW * 0.34;
    const qreal col2 = tableW * 0.42;
    const qreal col3 = tableW * 0.24;
    const qreal cellPadX = 4.0;

    auto ensureSpace = [&](qreal need) -> bool {
        if (y + need <= ctx.geom.pageH - ctx.geom.bottom) {
            return true;
        }
        if (!beginBodyPage(ctx)) {
            return false;
        }
        y = ctx.geom.top;
        return true;
    };

    auto measureRowHeight = [&](const QString &c1,
                                const QString &c2,
                                const QString &c3,
                                bool header) -> qreal {
        QFont rowFont = tableFont;
        rowFont.setBold(header);
        const QFontMetricsF metrics = metricsFor(ctx, rowFont);

        const qreal h1 = metrics.boundingRect(
            QRectF(0, 0, col1 - cellPadX * 2.0, ctx.geom.contentH),
            Qt::TextWordWrap,
            c1
        ).height();
        const qreal h2 = metrics.boundingRect(
            QRectF(0, 0, col2 - cellPadX * 2.0, ctx.geom.contentH),
            Qt::TextWordWrap,
            c2
        ).height();
        const qreal h3 = metrics.boundingRect(
            QRectF(0, 0, col3 - cellPadX * 2.0, ctx.geom.contentH),
            Qt::TextWordWrap,
            c3
        ).height();

        return qMax(h1, qMax(h2, h3)) + cellPadY * 2.0;
    };

    auto drawRow = [&](const QString &c1,
                       const QString &c2,
                       const QString &c3,
                       bool header) -> bool {
        const qreal rowH = measureRowHeight(c1, c2, c3, header);
        if (!ensureSpace(rowH)) {
            return false;
        }

        if (!ctx.dryRun && ctx.painter) {
            QPainter &painter = *ctx.painter;
            const QRectF r1(
                tableLeft + cellPadX,
                y,
                col1 - cellPadX * 2.0,
                rowH
            );
            const QRectF r2(
                tableLeft + col1 + cellPadX,
                y,
                col2 - cellPadX * 2.0,
                rowH
            );
            const QRectF r3(
                tableLeft + col1 + col2 + cellPadX,
                y,
                col3 - cellPadX * 2.0,
                rowH
            );

            QFont rowFont = tableFont;
            rowFont.setBold(header);
            painter.setFont(rowFont);
            painter.setPen(QColor(QStringLiteral("#222222")));
            const int flags =
                Qt::TextWordWrap
                | Qt::AlignHCenter
                | Qt::AlignVCenter;
            painter.drawText(r1, flags, c1);
            painter.drawText(r2, flags, c2);
            painter.drawText(r3, flags, c3);
        }

        y += rowH;
        return true;
    };

    const qreal headerRowH = measureRowHeight(
        QStringLiteral("参数"),
        QStringLiteral("数值"),
        QStringLiteral("单位"),
        true
    );

    if (!ensureSpace(captionH + headerRowH * 2.0)) {
        return false;
    }

    if (!ctx.dryRun && ctx.painter) {
        ctx.painter->setFont(captionFont);
        ctx.painter->setPen(QColor(QStringLiteral("#222222")));
        ctx.painter->drawText(
            QRectF(ctx.geom.left, y, ctx.geom.contentW, captionH),
            Qt::AlignHCenter | Qt::AlignVCenter,
            caption
        );
    }
    y += captionH + mmToPx(2.0);

    if (!drawRow(
            QStringLiteral("参数"),
            QStringLiteral("数值"),
            QStringLiteral("单位"),
            true)) {
        return false;
    }

    for (const SimulationReportRow &row : table.rows) {
        const qreal rowH = measureRowHeight(
            row.name,
            row.value,
            row.unit,
            false
        );

        if (y + rowH > ctx.geom.pageH - ctx.geom.bottom) {
            if (!beginBodyPage(ctx)) {
                return false;
            }
            y = ctx.geom.top;
            if (!ctx.dryRun && ctx.painter) {
                ctx.painter->setFont(captionFont);
                ctx.painter->setPen(QColor(QStringLiteral("#222222")));
                ctx.painter->drawText(
                    QRectF(
                        ctx.geom.left,
                        y,
                        ctx.geom.contentW,
                        captionH
                    ),
                    Qt::AlignHCenter | Qt::AlignVCenter,
                    caption + QStringLiteral("（续）")
                );
            }
            y += captionH + mmToPx(2.0);
            if (!drawRow(
                    QStringLiteral("参数"),
                    QStringLiteral("数值"),
                    QStringLiteral("单位"),
                    true)) {
                return false;
            }
        }

        if (!drawRow(row.name, row.value, row.unit, false)) {
            return false;
        }
    }

    y += mmToPx(6.0);
    return true;
}

bool layoutReport(
    PdfDrawContext &ctx,
    const SimulationReportModel &model,
    QString &errorMessage)
{
    // Cover
    {
        if (!ctx.dryRun && ctx.painter) {
            ctx.painter->fillRect(
                QRectF(0, 0, ctx.geom.pageW, ctx.geom.pageH),
                Qt::white
            );
        }

        qreal y = mmToPx(MarginTopMm + CoverTopSpacerMm);
        drawCenteredText(
            ctx,
            y,
            model.reportTitle,
            makeFont(QStringLiteral("SimHei"), CoverTitlePt, true),
            CoverAfterTitleMm
        );
        drawCenteredText(
            ctx,
            y,
            model.productName,
            makeFont(QStringLiteral("SimHei"), CoverSubtitlePt, true),
            CoverAfterSubtitleMm
        );

        y += mmToPx(CoverBeforeInfoMm);
        const QFont infoFont =
            makeFont(QStringLiteral("KaiTi"), CoverInfoPt);
        drawCenteredText(
            ctx,
            y,
            QStringLiteral("工程名称：%1").arg(model.projectName),
            infoFont,
            CoverInfoGapMm
        );
        drawCenteredText(
            ctx,
            y,
            QStringLiteral("Job名称：%1").arg(model.jobName),
            infoFont,
            CoverInfoGapMm
        );
        drawCenteredText(
            ctx,
            y,
            QStringLiteral("软件版本：%1").arg(model.appVersion),
            infoFont,
            CoverBeforeDateMm
        );

        y = ctx.geom.pageH - mmToPx(60.0);
        drawCenteredText(
            ctx,
            y,
            formatCoverDate(model.generatedAt),
            makeFont(QStringLiteral("KaiTi"), CoverDatePt),
            0.0
        );
        ctx.coverDrawn = true;
    }

    // 1 Overview
    if (!beginBodyPage(ctx)) {
        errorMessage = QStringLiteral("无法创建 PDF 新页。");
        return false;
    }
    {
        qreal y = ctx.geom.top;
        drawLeftText(
            ctx,
            y,
            QStringLiteral("1 仿真概况"),
            makeFont(QStringLiteral("SimHei"), Heading1Pt, true),
            1.0,
            6.0
        );
        SimulationReportTable overview;
        overview.rows = model.overviewRows;
        if (!drawTable(
                ctx,
                y,
                QStringLiteral("表 1.1 仿真概况"),
                overview)) {
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
        drawLeftText(
            ctx,
            y,
            QStringLiteral("2 输入参数"),
            makeFont(QStringLiteral("SimHei"), Heading1Pt, true),
            1.0,
            6.0
        );

        for (int i = 0; i < model.parameterTables.size(); ++i) {
            const SimulationReportTable &table =
                model.parameterTables.at(i);
            const QString caption =
                QStringLiteral("表 2.%1 %2")
                    .arg(i + 1)
                    .arg(table.title);
            if (!drawTable(ctx, y, caption, table)) {
                errorMessage =
                    QStringLiteral("无法绘制输入参数表格。");
                return false;
            }
        }
    }

    // 3/4/5 figures
    int sectionNumber = 3;
    for (const SimulationReportResultSection &section
         : model.resultSections) {
        int figureIndex = 1;
        for (const SimulationReportFigure &figure
             : section.figures) {
            if (!beginBodyPage(ctx)) {
                errorMessage = QStringLiteral("无法创建 PDF 新页。");
                return false;
            }

            qreal y = ctx.geom.top;
            drawLeftText(
                ctx,
                y,
                QStringLiteral("%1 %2")
                    .arg(sectionNumber)
                    .arg(section.title),
                makeFont(QStringLiteral("SimHei"), Heading1Pt, true),
                1.0,
                8.0
            );

            const qreal maxW = mmToPx(FigureMaxWidthMm);
            const qreal maxH = mmToPx(FigureMaxHeightMm);
            const QSizeF fitted =
                fitImage(figure.imagePixelSize, maxW, maxH);
            if (!fitted.isValid()) {
                errorMessage = QStringLiteral("报告图片尺寸无效。");
                return false;
            }

            const QFont captionFont =
                makeFont(QStringLiteral("SimSun"), CaptionPt);
            const QString caption = figureCaption(
                sectionNumber,
                figureIndex,
                section.title,
                figure
            );

            if (!ctx.dryRun) {
                QImage image(figure.imagePath);
                if (image.isNull()) {
                    errorMessage =
                        QStringLiteral("无法加载报告图片：\n%1")
                            .arg(figure.imagePath);
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

            y += fitted.height() + mmToPx(4.0);
            drawCenteredText(ctx, y, caption, captionFont, 0.0);
            ++figureIndex;
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
        drawLeftText(
            ctx,
            y,
            QStringLiteral("6 结果文件说明"),
            makeFont(QStringLiteral("SimHei"), Heading1Pt, true),
            1.0,
            6.0
        );
        for (const QString &note : model.notes) {
            drawBodyParagraph(ctx, y, note);
        }
    }

    // 7 Trace
    if (!beginBodyPage(ctx)) {
        errorMessage = QStringLiteral("无法创建 PDF 新页。");
        return false;
    }
    {
        qreal y = ctx.geom.top;
        drawLeftText(
            ctx,
            y,
            QStringLiteral("7 报告追溯信息"),
            makeFont(QStringLiteral("SimHei"), Heading1Pt, true),
            1.0,
            6.0
        );
        SimulationReportTable trace;
        trace.rows = model.traceRows;
        if (!drawTable(
                ctx,
                y,
                QStringLiteral("表 7.1 报告追溯信息"),
                trace)) {
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
    countCtx.dryRun = true;
    countCtx.bodyPageIndex = 1;
    countCtx.bodyTotalPages = 1;

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
    drawCtx.dryRun = false;
    drawCtx.bodyPageIndex = 1;
    drawCtx.bodyTotalPages = bodyTotalPages;

    if (!layoutReport(drawCtx, model, errorMessage)) {
        return false;
    }

    painter.end();
    errorMessage.clear();
    return true;
}
