#include "AbaqusFileGenerator.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>

QString AbaqusFileGenerator::number(double value)
{
    return QString::number(value, 'g', 15);
}

bool AbaqusFileGenerator::loadTemplate(
    const QString &resourcePath,
    QString &content,
    QString &errorMessage)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorMessage = QStringLiteral("无法读取模板资源：%1").arg(resourcePath);
        return false;
    }

    content = QString::fromUtf8(file.readAll());
    return true;
}

bool AbaqusFileGenerator::saveFile(
    const QString &filePath,
    const QString &content,
    QString &errorMessage)
{
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorMessage = QStringLiteral("无法写入文件：%1").arg(filePath);
        return false;
    }

    file.write(content.toUtf8());
    if (!file.commit()) {
        errorMessage = QStringLiteral("保存文件失败：%1").arg(filePath);
        return false;
    }

    return true;
}

bool AbaqusFileGenerator::generate(
    const QString &projectPath,
    const StructureConfig &structure,
    QString &errorMessage)
{
    QDir projectDir(projectPath);
    if (!projectDir.exists()) {
        errorMessage = QStringLiteral("工程目录不存在。");
        return false;
    }

    if (!projectDir.exists(QStringLiteral("abaqus"))
        && !projectDir.mkdir(QStringLiteral("abaqus"))) {
        errorMessage = QStringLiteral("无法创建 abaqus 目录。");
        return false;
    }

    QString content;
    if (!loadTemplate(
            QStringLiteral(":/simulation/templates/t0.py"),
            content,
            errorMessage)) {
        return false;
    }

    content.replace(
        QStringLiteral("{{CHARGE_RADIUS}}"),
        number(structure.chargeRadius)
    );
    content.replace(
        QStringLiteral("{{CHARGE_HEIGHT}}"),
        number(structure.chargeHeight)
    );
    content.replace(
        QStringLiteral("{{SHELL_THICKNESS}}"),
        number(structure.shellThickness)
    );

    if (content.contains(QStringLiteral("{{CHARGE_RADIUS}}"))
        || content.contains(QStringLiteral("{{CHARGE_HEIGHT}}"))
        || content.contains(QStringLiteral("{{SHELL_THICKNESS}}"))) {
        errorMessage = QStringLiteral("模板占位符替换失败。");
        return false;
    }

    const QString outputPath = projectDir.filePath(QStringLiteral("abaqus/t0.py"));
    return saveFile(outputPath, content, errorMessage);
}
