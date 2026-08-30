#include "SimulationReportDocxWriter.h"

#include "SimulationReportStyle.h"

#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QVector>
#include <QtEndian>
#include <QtGlobal>

namespace {

using namespace SimulationReportStyle;

quint32 crc32Of(const QByteArray &data)
{
    static quint32 table[256];
    static bool ready = false;
    if (!ready) {
        for (quint32 i = 0; i < 256; ++i) {
            quint32 c = i;
            for (int j = 0; j < 8; ++j) {
                c = (c & 1U)
                    ? (0xEDB88320U ^ (c >> 1))
                    : (c >> 1);
            }
            table[i] = c;
        }
        ready = true;
    }

    quint32 crc = 0xFFFFFFFFU;
    for (unsigned char byte : data) {
        crc = table[(crc ^ byte) & 0xFFU] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFU;
}

class DocxPackageWriter
{
public:
    explicit DocxPackageWriter(const QString &filePath)
        : m_filePath(filePath)
    {
    }

    bool addFile(const QString &name, const QByteArray &data)
    {
        Entry entry;
        entry.name = name.toUtf8();
        entry.crc = crc32Of(data);
        entry.size = static_cast<quint32>(data.size());
        m_entries.append(entry);
        m_data.append(data);
        return true;
    }

    bool close(QString &errorMessage)
    {
        QFile file(m_filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            errorMessage = QStringLiteral("无法创建 Word 报告文件。");
            return false;
        }

        QByteArray localParts;
        for (int i = 0; i < m_entries.size(); ++i) {
            Entry &entry = m_entries[i];
            entry.offset = static_cast<quint32>(localParts.size());

            QByteArray header;
            appendU32(header, 0x04034b50U);
            appendU16(header, 20);
            appendU16(header, 0);
            appendU16(header, 0);
            appendU16(header, 0);
            appendU16(header, 0);
            appendU32(header, entry.crc);
            appendU32(header, entry.size);
            appendU32(header, entry.size);
            appendU16(header, static_cast<quint16>(entry.name.size()));
            appendU16(header, 0);
            header.append(entry.name);

            localParts.append(header);
            localParts.append(m_data.at(i));
        }

        QByteArray central;
        for (const Entry &entry : m_entries) {
            appendU32(central, 0x02014b50U);
            appendU16(central, 20);
            appendU16(central, 20);
            appendU16(central, 0);
            appendU16(central, 0);
            appendU16(central, 0);
            appendU16(central, 0);
            appendU32(central, entry.crc);
            appendU32(central, entry.size);
            appendU32(central, entry.size);
            appendU16(central, static_cast<quint16>(entry.name.size()));
            appendU16(central, 0);
            appendU16(central, 0);
            appendU16(central, 0);
            appendU16(central, 0);
            appendU32(central, 0);
            appendU32(central, entry.offset);
            central.append(entry.name);
        }

        QByteArray end;
        appendU32(end, 0x06054b50U);
        appendU16(end, 0);
        appendU16(end, 0);
        appendU16(end, static_cast<quint16>(m_entries.size()));
        appendU16(end, static_cast<quint16>(m_entries.size()));
        appendU32(end, static_cast<quint32>(central.size()));
        appendU32(end, static_cast<quint32>(localParts.size()));
        appendU16(end, 0);

        if (file.write(localParts) != localParts.size()
            || file.write(central) != central.size()
            || file.write(end) != end.size()) {
            errorMessage = QStringLiteral("Word 报告写入失败。");
            return false;
        }

        file.close();
        errorMessage.clear();
        return true;
    }

private:
    struct Entry
    {
        QByteArray name;
        quint32 crc = 0;
        quint32 size = 0;
        quint32 offset = 0;
    };

    static void appendU16(QByteArray &out, quint16 value)
    {
        const quint16 le = qToLittleEndian(value);
        out.append(
            reinterpret_cast<const char *>(&le),
            int(sizeof(le))
        );
    }

    static void appendU32(QByteArray &out, quint32 value)
    {
        const quint32 le = qToLittleEndian(value);
        out.append(
            reinterpret_cast<const char *>(&le),
            int(sizeof(le))
        );
    }

    QString m_filePath;
    QVector<Entry> m_entries;
    QVector<QByteArray> m_data;
};

int ptToHalfPoints(double pt)
{
    return qRound(pt * 2.0);
}

int mmToTwips(double mm)
{
    return qRound(mm * 1440.0 / 25.4);
}

qint64 mmToEmu(double mm)
{
    return qRound64(mm * 36000.0);
}

QString xmlEscape(const QString &text)
{
    QString out = text;
    out.replace(QLatin1Char('&'), QStringLiteral("&amp;"));
    out.replace(QLatin1Char('<'), QStringLiteral("&lt;"));
    out.replace(QLatin1Char('>'), QStringLiteral("&gt;"));
    out.replace(QLatin1Char('"'), QStringLiteral("&quot;"));
    out.replace(QLatin1Char('\''), QStringLiteral("&apos;"));
    return out;
}

QString bodyFonts()
{
    return QStringLiteral(
        "<w:rFonts w:ascii=\"Times New Roman\" "
        "w:hAnsi=\"Times New Roman\" w:eastAsia=\"宋体\"/>"
    );
}

QString headingFonts()
{
    return QStringLiteral(
        "<w:rFonts w:ascii=\"SimHei\" "
        "w:hAnsi=\"SimHei\" w:eastAsia=\"黑体\"/>"
    );
}

QString kaiFonts()
{
    return QStringLiteral(
        "<w:rFonts w:ascii=\"KaiTi\" "
        "w:hAnsi=\"KaiTi\" w:eastAsia=\"楷体\"/>"
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

QString pageSzMarXml(bool withHeaderFooterStart)
{
    QString xml = QStringLiteral(
        "<w:pgSz w:w=\"%1\" w:h=\"%2\"/>"
        "<w:pgMar w:top=\"%3\" w:right=\"%4\" "
        "w:bottom=\"%5\" w:left=\"%6\" "
        "w:header=\"%7\" w:footer=\"%8\"/>"
    ).arg(mmToTwips(PageWidthMm))
        .arg(mmToTwips(PageHeightMm))
        .arg(mmToTwips(MarginTopMm))
        .arg(mmToTwips(MarginRightMm))
        .arg(mmToTwips(MarginBottomMm))
        .arg(mmToTwips(MarginLeftMm))
        .arg(mmToTwips(HeaderMm))
        .arg(mmToTwips(FooterMm));

    if (withHeaderFooterStart) {
        xml += QStringLiteral("<w:pgNumType w:start=\"1\"/>");
    }
    return xml;
}

QString paragraphStyled(
    const QString &text,
    const QString &styleId,
    const QString &rFonts,
    int halfPoints,
    bool bold,
    const QString &align,
    bool firstLineIndent = false,
    bool bodyLineSpacing = false,
    int spaceBeforeTwips = 0,
    int spaceAfterTwips = 0,
    bool keepNext = false,
    bool pageBreakBefore = false)
{
    QString pPr;
    if (!styleId.isEmpty()) {
        pPr += QStringLiteral("<w:pStyle w:val=\"%1\"/>")
            .arg(styleId);
    }

    if (pageBreakBefore) {
        pPr += QStringLiteral("<w:pageBreakBefore/>");
    }

    QString spacingAttrs;
    if (bodyLineSpacing) {
        spacingAttrs += QStringLiteral(
            " w:line=\"360\" w:lineRule=\"auto\""
        );
    }
    if (spaceBeforeTwips > 0) {
        spacingAttrs += QStringLiteral(" w:before=\"%1\"")
            .arg(spaceBeforeTwips);
    }
    if (spaceAfterTwips > 0) {
        spacingAttrs += QStringLiteral(" w:after=\"%1\"")
            .arg(spaceAfterTwips);
    }
    if (!spacingAttrs.isEmpty()) {
        pPr += QStringLiteral("<w:spacing%1/>").arg(spacingAttrs);
    }

    if (firstLineIndent) {
        pPr += QStringLiteral("<w:ind w:firstLine=\"480\"/>");
    }
    if (keepNext) {
        pPr += QStringLiteral("<w:keepNext/>");
    }
    if (!align.isEmpty()) {
        pPr += QStringLiteral("<w:jc w:val=\"%1\"/>").arg(align);
    }

    const QString boldXml =
        bold ? QStringLiteral("<w:b/><w:bCs/>") : QString();

    return QStringLiteral(
        "<w:p>"
        "<w:pPr>%1</w:pPr>"
        "<w:r>"
        "<w:rPr>%2%3"
        "<w:sz w:val=\"%4\"/><w:szCs w:val=\"%4\"/></w:rPr>"
        "<w:t xml:space=\"preserve\">%5</w:t>"
        "</w:r>"
        "</w:p>"
    ).arg(
        pPr,
        rFonts,
        boldXml,
        QString::number(halfPoints),
        xmlEscape(text)
    );
}

QString captionParagraph(
    const QString &text,
    bool keepNext = false,
    int spaceAfterTwips = 0,
    bool pageBreakBefore = false)
{
    return paragraphStyled(
        text,
        QString(),
        bodyFonts(),
        ptToHalfPoints(CaptionPt),
        false,
        QStringLiteral("center"),
        false,
        true,
        0,
        spaceAfterTwips,
        keepNext,
        pageBreakBefore
    );
}

QString tableXml(
    const QString &caption,
    const SimulationReportTable &table,
    bool pageBreakBefore = false)
{
    QString xml;
    xml += captionParagraph(caption, true, 0, pageBreakBefore);

    const int tableWidthTwips = mmToTwips(
        (PageWidthMm - MarginLeftMm - MarginRightMm)
        * TableWidthPct
    );
    const int col1Twips =
        qRound(tableWidthTwips * TableColumn1Pct);
    const int col2Twips =
        qRound(tableWidthTwips * TableColumn2Pct);
    const int col3Twips =
        tableWidthTwips - col1Twips - col2Twips;
    const int tableBorderSize =
        qRound(TableRulePt * 8.0);

    xml += QStringLiteral(
        "<w:tbl>"
        "<w:tblPr>"
        "<w:tblW w:w=\"%1\" w:type=\"dxa\"/>"
        "<w:jc w:val=\"center\"/>"
        "<w:tblLayout w:type=\"fixed\"/>"
        "<w:tblBorders>"
        "<w:top w:val=\"single\" w:sz=\"%2\" w:space=\"0\" w:color=\"000000\"/>"
        "<w:left w:val=\"nil\"/>"
        "<w:bottom w:val=\"single\" w:sz=\"%2\" w:space=\"0\" w:color=\"000000\"/>"
        "<w:right w:val=\"nil\"/>"
        "<w:insideH w:val=\"nil\"/>"
        "<w:insideV w:val=\"nil\"/>"
        "</w:tblBorders>"
        "</w:tblPr>"
        "<w:tblGrid>"
        "<w:gridCol w:w=\"%3\"/>"
        "<w:gridCol w:w=\"%4\"/>"
        "<w:gridCol w:w=\"%5\"/>"
        "</w:tblGrid>"
    ).arg(tableWidthTwips)
        .arg(tableBorderSize)
        .arg(col1Twips)
        .arg(col2Twips)
        .arg(col3Twips);

    const int tableHalfPts = ptToHalfPoints(TablePt);

    auto addRow = [&](const QString &c1,
                      const QString &c2,
                      const QString &c3,
                      bool header,
                      bool keepWithNext) {
        const QString headerBottom = header
            ? QStringLiteral(
                "<w:tcBorders>"
                "<w:bottom w:val=\"single\" w:sz=\"%1\" "
                "w:space=\"0\" w:color=\"000000\"/>"
                "</w:tcBorders>"
            ).arg(tableBorderSize)
            : QString();
        const QString keepNextXml = keepWithNext
            ? QStringLiteral("<w:keepNext/>")
            : QString();
        const int rowHeightTwips = qRound(
            (header ? TableHeaderRowMinPt : TableRowMinPt) * 20.0
        );

        auto cell = [&](const QString &text, int widthTwips) {
            return QStringLiteral(
                "<w:tc>"
                "<w:tcPr>"
                "<w:tcW w:w=\"%1\" w:type=\"dxa\"/>"
                "<w:vAlign w:val=\"center\"/>"
                "<w:tcMar>"
                "<w:top w:w=\"0\" w:type=\"dxa\"/>"
                "<w:left w:w=\"108\" w:type=\"dxa\"/>"
                "<w:bottom w:w=\"0\" w:type=\"dxa\"/>"
                "<w:right w:w=\"108\" w:type=\"dxa\"/>"
                "</w:tcMar>"
                "%2"
                "</w:tcPr>"
                "<w:p>"
                "<w:pPr>"
                "<w:jc w:val=\"center\"/>"
                "<w:spacing w:line=\"360\" w:lineRule=\"auto\" "
                "w:before=\"120\" w:after=\"120\"/>"
                "%3"
                "</w:pPr>"
                "<w:r>"
                "<w:rPr>%4"
                "<w:sz w:val=\"%5\"/><w:szCs w:val=\"%5\"/></w:rPr>"
                "<w:t xml:space=\"preserve\">%6</w:t>"
                "</w:r>"
                "</w:p>"
                "</w:tc>"
            ).arg(widthTwips)
                .arg(headerBottom)
                .arg(keepNextXml)
                .arg(bodyFonts())
                .arg(tableHalfPts)
                .arg(xmlEscape(text));
        };

        xml += QStringLiteral(
            "<w:tr>"
            "<w:trPr>"
            "<w:cantSplit/>"
            "<w:trHeight w:val=\"%1\" w:hRule=\"atLeast\"/>"
            "</w:trPr>"
        ).arg(rowHeightTwips);
        xml += cell(c1, col1Twips);
        xml += cell(c2, col2Twips);
        xml += cell(c3, col3Twips);
        xml += QStringLiteral("</w:tr>");
    };

    addRow(
        QStringLiteral("参数"),
        QStringLiteral("数值"),
        QStringLiteral("单位"),
        true,
        true
    );
    for (int i = 0; i < table.rows.size(); ++i) {
        const SimulationReportRow &row = table.rows.at(i);
        const bool keepWithNext = (i + 1 < table.rows.size());
        addRow(
            row.name,
            row.value,
            row.unit,
            false,
            keepWithNext
        );
    }

    xml += QStringLiteral("</w:tbl>");
    xml += QStringLiteral(
        "<w:p>"
        "<w:pPr>"
        "<w:spacing w:before=\"0\" w:after=\"%1\" w:line=\"20\" w:lineRule=\"exact\"/>"
        "</w:pPr>"
        "</w:p>"
    ).arg(mmToTwips(TableAfterGapMm));
    return xml;
}

QString imageParagraph(
    int drawingId,
    const QString &relId,
    qint64 cx,
    qint64 cy,
    const QString &name,
    bool keepNext,
    int spaceAfterTwips)
{
    QString pPr = QStringLiteral("<w:jc w:val=\"center\"/>");
    if (keepNext) {
        pPr += QStringLiteral("<w:keepNext/>");
    }
    if (spaceAfterTwips > 0) {
        pPr += QStringLiteral("<w:spacing w:after=\"%1\"/>")
            .arg(spaceAfterTwips);
    }

    return QStringLiteral(
        "<w:p>"
        "<w:pPr>%6</w:pPr>"
        "<w:r>"
        "<w:drawing>"
        "<wp:inline distT=\"0\" distB=\"0\" distL=\"0\" distR=\"0\">"
        "<wp:extent cx=\"%1\" cy=\"%2\"/>"
        "<wp:docPr id=\"%3\" name=\"%4\"/>"
        "<wp:cNvGraphicFramePr>"
        "<a:graphicFrameLocks "
        "xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
        "noChangeAspect=\"1\"/>"
        "</wp:cNvGraphicFramePr>"
        "<a:graphic "
        "xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\">"
        "<a:graphicData "
        "uri=\"http://schemas.openxmlformats.org/drawingml/2006/picture\">"
        "<pic:pic "
        "xmlns:pic=\"http://schemas.openxmlformats.org/drawingml/2006/picture\">"
        "<pic:nvPicPr>"
        "<pic:cNvPr id=\"0\" name=\"%4\"/>"
        "<pic:cNvPicPr/>"
        "</pic:nvPicPr>"
        "<pic:blipFill>"
        "<a:blip r:embed=\"%5\"/>"
        "<a:stretch><a:fillRect/></a:stretch>"
        "</pic:blipFill>"
        "<pic:spPr>"
        "<a:xfrm>"
        "<a:off x=\"0\" y=\"0\"/>"
        "<a:ext cx=\"%1\" cy=\"%2\"/>"
        "</a:xfrm>"
        "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom>"
        "</pic:spPr>"
        "</pic:pic>"
        "</a:graphicData>"
        "</a:graphic>"
        "</wp:inline>"
        "</w:drawing>"
        "</w:r>"
        "</w:p>"
    ).arg(cx)
        .arg(cy)
        .arg(drawingId)
        .arg(xmlEscape(name))
        .arg(relId)
        .arg(pPr);
}

} // namespace

bool SimulationReportDocxWriter::write(
    const SimulationReportModel &model,
    const QString &outputPath,
    QString &errorMessage,
    int bodyTotalPages,
    const QVector<int> &figurePageStarts)
{
    if (bodyTotalPages < 1) {
        errorMessage = QStringLiteral("报告页数无效。");
        return false;
    }
    struct MediaItem
    {
        QString partName;
        QString relId;
        QByteArray bytes;
        qint64 cx = 0;
        qint64 cy = 0;
        QString displayName;
    };

    QVector<MediaItem> media;
    int mediaIndex = 0;

    auto addFigureMedia =
        [&](const SimulationReportFigure &figure,
            MediaItem &out) -> bool {
        QFile imageFile(figure.imagePath);
        if (!imageFile.open(QIODevice::ReadOnly)) {
            errorMessage =
                QStringLiteral("无法读取报告图片：\n%1")
                    .arg(figure.imagePath);
            return false;
        }

        if (!figure.imagePixelSize.isValid()
            || figure.imagePixelSize.width() <= 0
            || figure.imagePixelSize.height() <= 0) {
            errorMessage = QStringLiteral("报告图片尺寸无效。");
            return false;
        }

        const double scale = qMin(
            FigureMaxWidthMm / figure.imagePixelSize.width(),
            FigureMaxHeightMm / figure.imagePixelSize.height()
        );
        const double widthMm =
            figure.imagePixelSize.width() * scale;
        const double heightMm =
            figure.imagePixelSize.height() * scale;

        ++mediaIndex;
        out.partName =
            QStringLiteral("word/media/image%1.png")
                .arg(mediaIndex, 3, 10, QLatin1Char('0'));
        out.relId =
            QStringLiteral("rId%1").arg(mediaIndex + 4);
        out.bytes = imageFile.readAll();
        out.cx = mmToEmu(widthMm);
        out.cy = mmToEmu(heightMm);
        out.displayName =
            QFileInfo(figure.imagePath).fileName();
        if (out.bytes.isEmpty()) {
            errorMessage =
                QStringLiteral("报告图片内容为空：\n%1")
                    .arg(figure.imagePath);
            return false;
        }
        return true;
    };

    QString body;

    // Cover — top spacer aligns title with PDF (~70 mm from page top).
    body += paragraphStyled(
        model.reportTitle,
        QStringLiteral("Title"),
        bodyFonts(),
        ptToHalfPoints(CoverTitlePt),
        true,
        QStringLiteral("center"),
        false,
        false,
        mmToTwips(CoverTopSpacerMm),
        mmToTwips(CoverAfterTitleMm)
    );
    body += paragraphStyled(
        model.productName,
        QString(),
        headingFonts(),
        ptToHalfPoints(CoverSubtitlePt),
        false,
        QStringLiteral("center"),
        false,
        false,
        0,
        mmToTwips(CoverAfterSubtitleMm + CoverBeforeInfoMm)
    );
    body += paragraphStyled(
        QStringLiteral("工程名称：%1").arg(model.projectName),
        QString(),
        kaiFonts(),
        ptToHalfPoints(CoverInfoPt),
        false,
        QStringLiteral("center"),
        false,
        false,
        0,
        mmToTwips(CoverInfoGapMm)
    );
    body += paragraphStyled(
        QStringLiteral("Job名称：%1").arg(model.jobName),
        QString(),
        kaiFonts(),
        ptToHalfPoints(CoverInfoPt),
        false,
        QStringLiteral("center"),
        false,
        false,
        0,
        mmToTwips(CoverInfoGapMm)
    );
    body += paragraphStyled(
        QStringLiteral("软件版本：%1").arg(model.appVersion),
        QString(),
        kaiFonts(),
        ptToHalfPoints(CoverInfoPt),
        false,
        QStringLiteral("center"),
        false,
        false,
        0,
        0
    );
    body += paragraphStyled(
        formatCoverDate(model.generatedAt),
        QString(),
        kaiFonts(),
        ptToHalfPoints(CoverDatePt),
        false,
        QStringLiteral("center"),
        false,
        false,
        mmToTwips(CoverBeforeDateMm),
        0
    );

    body += QStringLiteral(
        "<w:p><w:pPr><w:sectPr>"
        "<w:type w:val=\"nextPage\"/>"
    );
    body += pageSzMarXml(false);
    body += QStringLiteral("</w:sectPr></w:pPr></w:p>");

    // 1 Overview
    body += paragraphStyled(
        QStringLiteral("1 仿真概况"),
        QStringLiteral("Heading1"),
        headingFonts(),
        ptToHalfPoints(Heading1Pt),
        true,
        QStringLiteral("left")
    );
    {
        SimulationReportTable overview;
        overview.rows = model.overviewRows;
        body += tableXml(
            QStringLiteral("表 1.1 仿真概况"),
            overview
        );
    }

    // 2 Parameters
    body += paragraphStyled(
        QStringLiteral("2 输入参数"),
        QStringLiteral("Heading1"),
        headingFonts(),
        ptToHalfPoints(Heading1Pt),
        true,
        QStringLiteral("left"),
        false,
        false,
        0,
        0,
        false,
        true
    );
    for (int i = 0; i < model.parameterTables.size(); ++i) {
        const SimulationReportTable &table =
            model.parameterTables.at(i);
        body += tableXml(
            QStringLiteral("表 2.%1 %2")
                .arg(i + 1)
                .arg(table.title),
            table,
            i == 2
        );
    }

    // 3/4/5 figures — follow PDF figure-block page plan
    int sectionNumber = 3;
    int drawingId = 1;
    int globalFigureIndex = 0;
    for (const SimulationReportResultSection &section
         : model.resultSections) {
        if (section.figures.isEmpty()) {
            ++sectionNumber;
            continue;
        }

        for (int i = 0; i < section.figures.size(); ++i) {
            const SimulationReportFigure &figure =
                section.figures.at(i);
            const int figureIndex = i + 1;
            const bool startNewPage =
                figurePageStarts.contains(globalFigureIndex);

            if (figureIndex == 1) {
                body += paragraphStyled(
                    QStringLiteral("%1 %2")
                        .arg(sectionNumber)
                        .arg(section.title),
                    QStringLiteral("Heading1"),
                    headingFonts(),
                    ptToHalfPoints(Heading1Pt),
                    true,
                    QStringLiteral("left"),
                    false,
                    true,
                    0,
                    0,
                    true,
                    startNewPage
                );
            }

            body += paragraphStyled(
                QStringLiteral("%1.%2 %3")
                    .arg(sectionNumber)
                    .arg(figureIndex)
                    .arg(figure.label),
                QStringLiteral("Heading2"),
                headingFonts(),
                ptToHalfPoints(Heading2Pt),
                true,
                QStringLiteral("left"),
                false,
                true,
                0,
                mmToTwips(FigureHeadingAfterMm),
                true,
                figureIndex > 1 && startNewPage
            );

            MediaItem item;
            if (!addFigureMedia(figure, item)) {
                return false;
            }
            media.append(item);
            body += imageParagraph(
                drawingId++,
                item.relId,
                item.cx,
                item.cy,
                item.displayName,
                true,
                mmToTwips(FigureCaptionGapMm)
            );
            body += captionParagraph(
                figureCaption(
                    sectionNumber,
                    figureIndex,
                    section.title,
                    figure
                ),
                false,
                mmToTwips(FigureBlockAfterMm)
            );
            ++globalFigureIndex;
        }
        ++sectionNumber;
    }

    body += paragraphStyled(
        QStringLiteral("6 结果文件说明"),
        QStringLiteral("Heading1"),
        headingFonts(),
        ptToHalfPoints(Heading1Pt),
        true,
        QStringLiteral("left"),
        false,
        false,
        0,
        0,
        false,
        true
    );
    for (const QString &note : model.notes) {
        body += paragraphStyled(
            note,
            QString(),
            bodyFonts(),
            ptToHalfPoints(BodyPt),
            false,
            QStringLiteral("both"),
            true,
            true
        );
    }

    body += paragraphStyled(
        QStringLiteral("7 报告追溯信息"),
        QStringLiteral("Heading1"),
        headingFonts(),
        ptToHalfPoints(Heading1Pt),
        true,
        QStringLiteral("left"),
        false,
        false,
        0,
        0,
        false,
        true
    );
    {
        SimulationReportTable trace;
        trace.rows = model.traceRows;
        body += tableXml(
            QStringLiteral("表 7.1 报告追溯信息"),
            trace
        );
    }

    body += QStringLiteral(
        "<w:sectPr>"
        "<w:headerReference w:type=\"default\" r:id=\"rId2\"/>"
        "<w:footerReference w:type=\"default\" r:id=\"rId3\"/>"
    );
    body += pageSzMarXml(true);
    body += QStringLiteral("</w:sectPr>");

    const QString documentXml = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:document "
        "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" "
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
        "xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing\">"
        "<w:body>%1</w:body>"
        "</w:document>"
    ).arg(body);

    const QString bodyFontXml = bodyFonts();
    const QString headingFontXml = headingFonts();
    const QString stylesXml =
        QStringLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<w:styles "
            "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
            "<w:style w:type=\"paragraph\" w:default=\"1\" w:styleId=\"Normal\">"
            "<w:name w:val=\"Normal\"/>"
            "<w:qFormat/>"
            "<w:pPr>"
            "<w:spacing w:line=\"360\" w:lineRule=\"auto\"/>"
            "<w:jc w:val=\"both\"/>"
            "</w:pPr>"
            "<w:rPr>"
        )
        + bodyFontXml
        + QStringLiteral(
            "<w:sz w:val=\"%1\"/><w:szCs w:val=\"%1\"/>"
            "</w:rPr>"
            "</w:style>"
            "<w:style w:type=\"paragraph\" w:styleId=\"Title\">"
            "<w:name w:val=\"Title\"/>"
            "<w:basedOn w:val=\"Normal\"/>"
            "<w:qFormat/>"
            "<w:pPr><w:jc w:val=\"center\"/>"
            "<w:spacing w:before=\"240\" w:after=\"240\"/></w:pPr>"
            "<w:rPr>"
        ).arg(ptToHalfPoints(BodyPt))
        + bodyFontXml
        + QStringLiteral(
            "<w:b/><w:bCs/>"
            "<w:sz w:val=\"%1\"/><w:szCs w:val=\"%1\"/></w:rPr>"
            "</w:style>"
            "<w:style w:type=\"paragraph\" w:styleId=\"Heading1\">"
            "<w:name w:val=\"heading 1\"/>"
            "<w:basedOn w:val=\"Normal\"/>"
            "<w:qFormat/>"
            "<w:pPr>"
            "<w:spacing w:line=\"360\" w:lineRule=\"auto\" "
            "w:before=\"50\" w:after=\"50\"/>"
            "<w:jc w:val=\"left\"/></w:pPr>"
            "<w:rPr>"
        ).arg(ptToHalfPoints(CoverTitlePt))
        + headingFontXml
        + QStringLiteral(
            "<w:b/><w:bCs/>"
            "<w:sz w:val=\"%1\"/><w:szCs w:val=\"%1\"/></w:rPr>"
            "</w:style>"
            "<w:style w:type=\"paragraph\" w:styleId=\"Heading2\">"
            "<w:name w:val=\"heading 2\"/>"
            "<w:basedOn w:val=\"Normal\"/>"
            "<w:qFormat/>"
            "<w:pPr>"
            "<w:spacing w:line=\"360\" w:lineRule=\"auto\"/>"
            "<w:jc w:val=\"left\"/></w:pPr>"
            "<w:rPr>"
        ).arg(ptToHalfPoints(Heading1Pt))
        + headingFontXml
        + QStringLiteral(
            "<w:b/><w:bCs/>"
            "<w:sz w:val=\"%1\"/><w:szCs w:val=\"%1\"/></w:rPr>"
            "</w:style>"
            "<w:style w:type=\"paragraph\" w:styleId=\"Heading3\">"
            "<w:name w:val=\"heading 3\"/>"
            "<w:basedOn w:val=\"Normal\"/>"
            "<w:qFormat/>"
            "<w:pPr>"
            "<w:spacing w:line=\"360\" w:lineRule=\"auto\"/>"
            "<w:jc w:val=\"left\"/></w:pPr>"
            "<w:rPr>"
        ).arg(ptToHalfPoints(Heading2Pt))
        + headingFontXml
        + QStringLiteral(
            "<w:sz w:val=\"%1\"/><w:szCs w:val=\"%1\"/></w:rPr>"
            "</w:style>"
            "</w:styles>"
        ).arg(ptToHalfPoints(Heading3Pt));

    const QString headerXml = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:hdr "
        "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
        "<w:p>"
        "<w:pPr>"
        "<w:jc w:val=\"center\"/>"
        "<w:pBdr>"
        "<w:bottom w:val=\"single\" w:sz=\"6\" w:space=\"1\" w:color=\"000000\"/>"
        "</w:pBdr>"
        "</w:pPr>"
        "<w:r>"
        "<w:rPr>%1"
        "<w:spacing w:val=\"20\"/>"
        "<w:sz w:val=\"%2\"/><w:szCs w:val=\"%2\"/></w:rPr>"
        "<w:t xml:space=\"preserve\">%3</w:t>"
        "</w:r>"
        "</w:p>"
        "</w:hdr>"
    ).arg(
        headingFonts(),
        QString::number(ptToHalfPoints(HeaderPt)),
        xmlEscape(model.reportTitle)
    );

    const QString footerFontXml = bodyFonts();
    const QString footerSizeXml =
        QString::number(ptToHalfPoints(FooterPt));
    const QString footerPagesXml =
        QString::number(bodyTotalPages);
    QString footerXml = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:ftr "
        "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
        "<w:p>"
        "<w:pPr><w:jc w:val=\"right\"/></w:pPr>"
        "<w:r>"
        "<w:rPr>__RF__"
        "<w:sz w:val=\"__SZ__\"/><w:szCs w:val=\"__SZ__\"/></w:rPr>"
        "<w:t xml:space=\"preserve\">第 </w:t>"
        "</w:r>"
        "<w:fldSimple w:instr=\" PAGE \" w:dirty=\"true\">"
        "<w:r>"
        "<w:rPr>__RF__"
        "<w:sz w:val=\"__SZ__\"/><w:szCs w:val=\"__SZ__\"/></w:rPr>"
        "<w:t>1</w:t>"
        "</w:r>"
        "</w:fldSimple>"
        "<w:r>"
        "<w:rPr>__RF__"
        "<w:sz w:val=\"__SZ__\"/><w:szCs w:val=\"__SZ__\"/></w:rPr>"
        "<w:t xml:space=\"preserve\"> 页 共 </w:t>"
        "</w:r>"
        "<w:fldSimple w:instr=\" SECTIONPAGES \" w:dirty=\"true\">"
        "<w:r>"
        "<w:rPr>__RF__"
        "<w:sz w:val=\"__SZ__\"/><w:szCs w:val=\"__SZ__\"/></w:rPr>"
        "<w:t>__PAGES__</w:t>"
        "</w:r>"
        "</w:fldSimple>"
        "<w:r>"
        "<w:rPr>__RF__"
        "<w:sz w:val=\"__SZ__\"/><w:szCs w:val=\"__SZ__\"/></w:rPr>"
        "<w:t xml:space=\"preserve\"> 页</w:t>"
        "</w:r>"
        "</w:p>"
        "</w:ftr>"
    );
    footerXml.replace(QStringLiteral("__RF__"), footerFontXml);
    footerXml.replace(QStringLiteral("__SZ__"), footerSizeXml);
    footerXml.replace(QStringLiteral("__PAGES__"), footerPagesXml);

    const QString created =
        QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    const QString coreXml = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<cp:coreProperties "
        "xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" "
        "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
        "xmlns:dcterms=\"http://purl.org/dc/terms/\" "
        "xmlns:dcmitype=\"http://purl.org/dc/dcmitype/\" "
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">"
        "<dc:title>%1</dc:title>"
        "<dc:creator>%2</dc:creator>"
        "<cp:lastModifiedBy>%2</cp:lastModifiedBy>"
        "<dcterms:created xsi:type=\"dcterms:W3CDTF\">%3</dcterms:created>"
        "<dcterms:modified xsi:type=\"dcterms:W3CDTF\">%3</dcterms:modified>"
        "</cp:coreProperties>"
    ).arg(
        xmlEscape(model.reportTitle),
        xmlEscape(model.productName),
        created
    );

    const QString appXml = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Properties "
        "xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\">"
        "<Application>%1</Application>"
        "<AppVersion>%2</AppVersion>"
        "</Properties>"
    ).arg(
        xmlEscape(model.productName),
        xmlEscape(model.appVersion)
    );

    const QString settingsXml = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:settings "
        "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
        "<w:updateFields w:val=\"true\"/>"
        "</w:settings>"
    );

    const QString contentTypes = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Types "
        "xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" "
        "ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" "
        "ContentType=\"application/xml\"/>"
        "<Default Extension=\"png\" "
        "ContentType=\"image/png\"/>"
        "<Override PartName=\"/word/document.xml\" "
        "ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>"
        "<Override PartName=\"/word/styles.xml\" "
        "ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml\"/>"
        "<Override PartName=\"/word/settings.xml\" "
        "ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml\"/>"
        "<Override PartName=\"/word/header1.xml\" "
        "ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml\"/>"
        "<Override PartName=\"/word/footer1.xml\" "
        "ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml\"/>"
        "<Override PartName=\"/docProps/core.xml\" "
        "ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/>"
        "<Override PartName=\"/docProps/app.xml\" "
        "ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/>"
        "</Types>"
    );

    const QString rootRels = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships "
        "xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" "
        "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" "
        "Target=\"word/document.xml\"/>"
        "<Relationship Id=\"rId2\" "
        "Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" "
        "Target=\"docProps/core.xml\"/>"
        "<Relationship Id=\"rId3\" "
        "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties\" "
        "Target=\"docProps/app.xml\"/>"
        "</Relationships>"
    );

    QString documentRels = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships "
        "xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" "
        "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" "
        "Target=\"styles.xml\"/>"
        "<Relationship Id=\"rId2\" "
        "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/header\" "
        "Target=\"header1.xml\"/>"
        "<Relationship Id=\"rId3\" "
        "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer\" "
        "Target=\"footer1.xml\"/>"
        "<Relationship Id=\"rId4\" "
        "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings\" "
        "Target=\"settings.xml\"/>"
    );
    for (const MediaItem &item : media) {
        const QString target =
            item.partName.mid(QStringLiteral("word/").size());
        documentRels += QStringLiteral(
            "<Relationship Id=\"%1\" "
            "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\" "
            "Target=\"%2\"/>"
        ).arg(item.relId, target);
    }
    documentRels += QStringLiteral("</Relationships>");

    DocxPackageWriter package(outputPath);
    package.addFile(
        QStringLiteral("[Content_Types].xml"),
        contentTypes.toUtf8()
    );
    package.addFile(
        QStringLiteral("_rels/.rels"),
        rootRels.toUtf8()
    );
    package.addFile(
        QStringLiteral("docProps/core.xml"),
        coreXml.toUtf8()
    );
    package.addFile(
        QStringLiteral("docProps/app.xml"),
        appXml.toUtf8()
    );
    package.addFile(
        QStringLiteral("word/document.xml"),
        documentXml.toUtf8()
    );
    package.addFile(
        QStringLiteral("word/styles.xml"),
        stylesXml.toUtf8()
    );
    package.addFile(
        QStringLiteral("word/settings.xml"),
        settingsXml.toUtf8()
    );
    package.addFile(
        QStringLiteral("word/header1.xml"),
        headerXml.toUtf8()
    );
    package.addFile(
        QStringLiteral("word/footer1.xml"),
        footerXml.toUtf8()
    );
    package.addFile(
        QStringLiteral("word/_rels/document.xml.rels"),
        documentRels.toUtf8()
    );
    for (const MediaItem &item : media) {
        package.addFile(item.partName, item.bytes);
    }

    return package.close(errorMessage);
}
