#include "SimulationReportDocxWriter.h"

#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QVector>
#include <QtEndian>
#include <QtGlobal>

namespace {

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

qint64 mmToEmu(double mm)
{
    return qRound64(mm * 36000.0);
}

QString pageSzMarXml(bool withHeaderFooterStart)
{
    QString xml;
    xml += QStringLiteral(
        "<w:pgSz w:w=\"11906\" w:h=\"16838\"/>"
        "<w:pgMar w:top=\"1134\" w:right=\"1134\" "
        "w:bottom=\"1134\" w:left=\"1134\" "
        "w:header=\"567\" w:footer=\"567\"/>"
    );
    if (withHeaderFooterStart) {
        xml += QStringLiteral("<w:pgNumType w:start=\"1\"/>");
    }
    return xml;
}

QString paragraph(
    const QString &text,
    const QString &styleId = QString(),
    int fontSizeHalfPoints = 24)
{
    QString pPr;
    if (!styleId.isEmpty()) {
        pPr += QStringLiteral("<w:pStyle w:val=\"%1\"/>")
            .arg(styleId);
    }

    return QStringLiteral(
        "<w:p>"
        "<w:pPr>%1</w:pPr>"
        "<w:r>"
        "<w:rPr><w:rFonts w:ascii=\"Microsoft YaHei\" "
        "w:hAnsi=\"Microsoft YaHei\" w:eastAsia=\"Microsoft YaHei\"/>"
        "<w:sz w:val=\"%2\"/><w:szCs w:val=\"%2\"/></w:rPr>"
        "<w:t xml:space=\"preserve\">%3</w:t>"
        "</w:r>"
        "</w:p>"
    ).arg(pPr)
        .arg(fontSizeHalfPoints)
        .arg(xmlEscape(text));
}

QString pageBreakParagraph()
{
    return QStringLiteral(
        "<w:p><w:r><w:br w:type=\"page\"/></w:r></w:p>"
    );
}

QString tableXml(const SimulationReportTable &table)
{
    QString xml;
    xml += paragraph(table.title, QStringLiteral("Heading2"), 28);
    xml += QStringLiteral(
        "<w:tbl>"
        "<w:tblPr>"
        "<w:tblW w:w=\"0\" w:type=\"auto\"/>"
        "<w:tblBorders>"
        "<w:top w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"AAAAAA\"/>"
        "<w:left w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"AAAAAA\"/>"
        "<w:bottom w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"AAAAAA\"/>"
        "<w:right w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"AAAAAA\"/>"
        "<w:insideH w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"AAAAAA\"/>"
        "<w:insideV w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"AAAAAA\"/>"
        "</w:tblBorders>"
        "</w:tblPr>"
        "<w:tblGrid>"
        "<w:gridCol w:w=\"3120\"/>"
        "<w:gridCol w:w=\"3960\"/>"
        "<w:gridCol w:w=\"2160\"/>"
        "</w:tblGrid>"
    );

    auto addRow = [&](const QString &c1,
                      const QString &c2,
                      const QString &c3,
                      bool header) {
        const QString bold = header
            ? QStringLiteral("<w:b/>")
            : QString();
        auto cell = [&](const QString &text) {
            return QStringLiteral(
                "<w:tc>"
                "<w:tcPr><w:tcW w:w=\"0\" w:type=\"auto\"/></w:tcPr>"
                "<w:p><w:r>"
                "<w:rPr><w:rFonts w:ascii=\"Microsoft YaHei\" "
                "w:hAnsi=\"Microsoft YaHei\" "
                "w:eastAsia=\"Microsoft YaHei\"/>"
                "%1"
                "<w:sz w:val=\"20\"/><w:szCs w:val=\"20\"/></w:rPr>"
                "<w:t xml:space=\"preserve\">%2</w:t>"
                "</w:r></w:p></w:tc>"
            ).arg(bold, xmlEscape(text));
        };
        xml += QStringLiteral("<w:tr>");
        xml += cell(c1);
        xml += cell(c2);
        xml += cell(c3);
        xml += QStringLiteral("</w:tr>");
    };

    addRow(
        QStringLiteral("参数"),
        QStringLiteral("数值"),
        QStringLiteral("单位"),
        true
    );
    for (const SimulationReportRow &row : table.rows) {
        addRow(row.name, row.value, row.unit, false);
    }

    xml += QStringLiteral("</w:tbl>");
    xml += paragraph(QString());
    return xml;
}

QString imageParagraph(
    int drawingId,
    const QString &relId,
    qint64 cx,
    qint64 cy,
    const QString &name)
{
    return QStringLiteral(
        "<w:p>"
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
        .arg(relId);
}

} // namespace

bool SimulationReportDocxWriter::write(
    const SimulationReportModel &model,
    const QString &outputPath,
    QString &errorMessage)
{
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
            165.0 / figure.imagePixelSize.width(),
            190.0 / figure.imagePixelSize.height()
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
            QStringLiteral("rId%1").arg(mediaIndex + 3);
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

    // Cover
    body += paragraph(model.reportTitle, QStringLiteral("Title"), 44);
    body += paragraph(
        QStringLiteral("工程名称：") + model.projectName,
        QString(),
        28
    );
    body += paragraph(
        QStringLiteral("Job名称：") + model.jobName,
        QString(),
        28
    );
    body += paragraph(
        QStringLiteral("软件名称：") + model.productName,
        QString(),
        28
    );
    body += paragraph(
        QStringLiteral("软件版本：") + model.appVersion,
        QString(),
        28
    );
    body += paragraph(
        QStringLiteral("生成时间：") + model.generatedAt,
        QString(),
        28
    );

    body += QStringLiteral(
        "<w:p><w:pPr><w:sectPr>"
        "<w:type w:val=\"nextPage\"/>"
    );
    body += pageSzMarXml(false);
    body += QStringLiteral("</w:sectPr></w:pPr></w:p>");

    // 1 Overview
    body += paragraph(
        QStringLiteral("1 仿真概况"),
        QStringLiteral("Heading1"),
        32
    );
    {
        SimulationReportTable overview;
        overview.title = QStringLiteral("概况");
        overview.rows = model.overviewRows;
        body += tableXml(overview);
    }

    // 2 Parameters
    body += paragraph(
        QStringLiteral("2 输入参数"),
        QStringLiteral("Heading1"),
        32
    );
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
        body += tableXml(table);
    }

    // 3/4/5 result sections
    int sectionNumber = 3;
    int drawingId = 1;
    bool firstResultFigure = true;
    for (const SimulationReportResultSection &section
         : model.resultSections) {
        for (const SimulationReportFigure &figure
             : section.figures) {
            body += pageBreakParagraph();
            firstResultFigure = false;

            body += paragraph(
                QStringLiteral("%1 %2")
                    .arg(sectionNumber)
                    .arg(section.title),
                QStringLiteral("Heading1"),
                32
            );
            body += paragraph(figure.label, QString(), 26);
            body += paragraph(figure.frameText, QString(), 22);
            if (!figure.timeText.isEmpty()) {
                body += paragraph(figure.timeText, QString(), 22);
            }

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
                item.displayName
            );
        }
        ++sectionNumber;
    }

    if (!firstResultFigure) {
        body += pageBreakParagraph();
    }
    body += paragraph(
        QStringLiteral("6 结果文件说明"),
        QStringLiteral("Heading1"),
        32
    );
    for (const QString &note : model.notes) {
        body += paragraph(
            QStringLiteral("• %1").arg(note),
            QString(),
            22
        );
    }

    body += pageBreakParagraph();
    body += paragraph(
        QStringLiteral("7 报告追溯信息"),
        QStringLiteral("Heading1"),
        32
    );
    {
        SimulationReportTable trace;
        trace.title = QStringLiteral("追溯信息");
        trace.rows = model.traceRows;
        body += tableXml(trace);
    }

    // Final body section properties
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

    const QString stylesXml = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:styles "
        "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
        "<w:style w:type=\"paragraph\" w:default=\"1\" w:styleId=\"Normal\">"
        "<w:name w:val=\"Normal\"/>"
        "<w:qFormat/>"
        "<w:rPr>"
        "<w:rFonts w:ascii=\"Microsoft YaHei\" w:hAnsi=\"Microsoft YaHei\" "
        "w:eastAsia=\"Microsoft YaHei\"/>"
        "<w:sz w:val=\"22\"/><w:szCs w:val=\"22\"/>"
        "</w:rPr>"
        "</w:style>"
        "<w:style w:type=\"paragraph\" w:styleId=\"Title\">"
        "<w:name w:val=\"Title\"/>"
        "<w:basedOn w:val=\"Normal\"/>"
        "<w:qFormat/>"
        "<w:pPr><w:spacing w:before=\"240\" w:after=\"240\"/></w:pPr>"
        "<w:rPr><w:b/><w:sz w:val=\"44\"/><w:szCs w:val=\"44\"/></w:rPr>"
        "</w:style>"
        "<w:style w:type=\"paragraph\" w:styleId=\"Heading1\">"
        "<w:name w:val=\"heading 1\"/>"
        "<w:basedOn w:val=\"Normal\"/>"
        "<w:qFormat/>"
        "<w:pPr><w:spacing w:before=\"240\" w:after=\"120\"/></w:pPr>"
        "<w:rPr><w:b/><w:sz w:val=\"32\"/><w:szCs w:val=\"32\"/></w:rPr>"
        "</w:style>"
        "<w:style w:type=\"paragraph\" w:styleId=\"Heading2\">"
        "<w:name w:val=\"heading 2\"/>"
        "<w:basedOn w:val=\"Normal\"/>"
        "<w:qFormat/>"
        "<w:pPr><w:spacing w:before=\"160\" w:after=\"80\"/></w:pPr>"
        "<w:rPr><w:b/><w:sz w:val=\"26\"/><w:szCs w:val=\"26\"/></w:rPr>"
        "</w:style>"
        "</w:styles>"
    );

    const QString headerXml = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:hdr "
        "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
        "<w:p>"
        "<w:pPr>"
        "<w:tabs><w:tab w:val=\"right\" w:pos=\"9026\"/></w:tabs>"
        "<w:pBdr>"
        "<w:bottom w:val=\"single\" w:sz=\"6\" w:space=\"1\" w:color=\"AAAAAA\"/>"
        "</w:pBdr>"
        "</w:pPr>"
        "<w:r>"
        "<w:rPr><w:rFonts w:ascii=\"Microsoft YaHei\" "
        "w:hAnsi=\"Microsoft YaHei\" w:eastAsia=\"Microsoft YaHei\"/>"
        "<w:sz w:val=\"18\"/><w:szCs w:val=\"18\"/>"
        "<w:color w:val=\"555555\"/></w:rPr>"
        "<w:t xml:space=\"preserve\">PBX浇注固化仿真分析报告</w:t>"
        "</w:r>"
        "<w:r><w:tab/></w:r>"
        "<w:r>"
        "<w:rPr><w:rFonts w:ascii=\"Microsoft YaHei\" "
        "w:hAnsi=\"Microsoft YaHei\" w:eastAsia=\"Microsoft YaHei\"/>"
        "<w:sz w:val=\"18\"/><w:szCs w:val=\"18\"/>"
        "<w:color w:val=\"555555\"/></w:rPr>"
        "<w:t xml:space=\"preserve\">%1</w:t>"
        "</w:r>"
        "</w:p>"
        "</w:hdr>"
    ).arg(xmlEscape(model.projectName));

    const QString footerXml = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:ftr "
        "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
        "<w:p>"
        "<w:pPr><w:jc w:val=\"center\"/></w:pPr>"
        "<w:r>"
        "<w:rPr><w:rFonts w:ascii=\"Microsoft YaHei\" "
        "w:hAnsi=\"Microsoft YaHei\" w:eastAsia=\"Microsoft YaHei\"/>"
        "<w:sz w:val=\"18\"/><w:szCs w:val=\"18\"/></w:rPr>"
        "<w:t xml:space=\"preserve\">第 </w:t>"
        "</w:r>"
        "<w:fldSimple w:instr=\" PAGE \">"
        "<w:r>"
        "<w:rPr><w:rFonts w:ascii=\"Microsoft YaHei\" "
        "w:hAnsi=\"Microsoft YaHei\" w:eastAsia=\"Microsoft YaHei\"/>"
        "<w:sz w:val=\"18\"/><w:szCs w:val=\"18\"/></w:rPr>"
        "<w:t>1</w:t>"
        "</w:r>"
        "</w:fldSimple>"
        "<w:r>"
        "<w:rPr><w:rFonts w:ascii=\"Microsoft YaHei\" "
        "w:hAnsi=\"Microsoft YaHei\" w:eastAsia=\"Microsoft YaHei\"/>"
        "<w:sz w:val=\"18\"/><w:szCs w:val=\"18\"/></w:rPr>"
        "<w:t xml:space=\"preserve\"> 页 / 共 </w:t>"
        "</w:r>"
        "<w:fldSimple w:instr=\" SECTIONPAGES \">"
        "<w:r>"
        "<w:rPr><w:rFonts w:ascii=\"Microsoft YaHei\" "
        "w:hAnsi=\"Microsoft YaHei\" w:eastAsia=\"Microsoft YaHei\"/>"
        "<w:sz w:val=\"18\"/><w:szCs w:val=\"18\"/></w:rPr>"
        "<w:t>1</w:t>"
        "</w:r>"
        "</w:fldSimple>"
        "<w:r>"
        "<w:rPr><w:rFonts w:ascii=\"Microsoft YaHei\" "
        "w:hAnsi=\"Microsoft YaHei\" w:eastAsia=\"Microsoft YaHei\"/>"
        "<w:sz w:val=\"18\"/><w:szCs w:val=\"18\"/></w:rPr>"
        "<w:t xml:space=\"preserve\"> 页</w:t>"
        "</w:r>"
        "</w:p>"
        "</w:ftr>"
    );

    const QString created =
        QDateTime::currentDateTimeUtc()
            .toString(Qt::ISODate);
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

    QString contentTypes = QStringLiteral(
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
