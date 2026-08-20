#include "AbaqusFileGenerator.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QStringList>

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
    const ExplosiveConfig &explosive,
    const MoldConfig &mold,
    const BoundaryConfig &boundary,
    const SimulationConfig &simulation,
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

    const QStringList requiredPlaceholders = {
        QStringLiteral("{{CHARGE_RADIUS}}"),
        QStringLiteral("{{CHARGE_HEIGHT}}"),
        QStringLiteral("{{SHELL_THICKNESS}}"),

        QStringLiteral("{{PBX_DENSITY}}"),
        QStringLiteral("{{PBX_INITIAL_ELASTIC_MODULUS}}"),
        QStringLiteral("{{PBX_INITIAL_POISSON_RATIO}}"),
        QStringLiteral("{{PBX_FINAL_ELASTIC_MODULUS}}"),
        QStringLiteral("{{PBX_FINAL_POISSON_RATIO}}"),
        QStringLiteral("{{PBX_THERMAL_CONDUCTIVITY}}"),
        QStringLiteral("{{PBX_YIELD_STRESS}}"),
        QStringLiteral("{{PBX_SPECIFIC_HEAT}}"),
        QStringLiteral("{{PBX_EXPANSION_COEFFICIENT}}"),

        QStringLiteral("{{MOLD_DENSITY}}"),
        QStringLiteral("{{MOLD_ELASTIC_MODULUS}}"),
        QStringLiteral("{{MOLD_POISSON_RATIO}}"),
        QStringLiteral("{{MOLD_THERMAL_CONDUCTIVITY}}"),
        QStringLiteral("{{MOLD_SPECIFIC_HEAT}}"),

        QStringLiteral("{{AMBIENT_TEMPERATURE}}"),

        QStringLiteral("{{SIMULATION_TIME_LENGTH}}"),

        QStringLiteral("{{CAE_SAVE_PATH}}")
    };

    for (const QString &placeholder : requiredPlaceholders) {
        if (!content.contains(placeholder)) {
            errorMessage = QStringLiteral("模板缺少占位符：%1").arg(placeholder);
            return false;
        }
    }

    content.replace(QStringLiteral("{{CHARGE_RADIUS}}"), number(structure.chargeRadius));
    content.replace(QStringLiteral("{{CHARGE_HEIGHT}}"), number(structure.chargeHeight));
    content.replace(QStringLiteral("{{SHELL_THICKNESS}}"), number(structure.shellThickness));

    content.replace(QStringLiteral("{{PBX_DENSITY}}"), number(explosive.density));
    content.replace(QStringLiteral("{{PBX_INITIAL_ELASTIC_MODULUS}}"), number(explosive.initialElasticModulus));
    content.replace(QStringLiteral("{{PBX_INITIAL_POISSON_RATIO}}"), number(explosive.initialPoissonRatio));
    content.replace(QStringLiteral("{{PBX_FINAL_ELASTIC_MODULUS}}"), number(explosive.finalElasticModulus));
    content.replace(QStringLiteral("{{PBX_FINAL_POISSON_RATIO}}"), number(explosive.finalPoissonRatio));
    content.replace(QStringLiteral("{{PBX_THERMAL_CONDUCTIVITY}}"), number(explosive.thermalConductivity));
    content.replace(QStringLiteral("{{PBX_YIELD_STRESS}}"), number(explosive.yieldStress));
    content.replace(QStringLiteral("{{PBX_SPECIFIC_HEAT}}"), number(explosive.specificHeat));
    content.replace(QStringLiteral("{{PBX_EXPANSION_COEFFICIENT}}"), number(explosive.expansionCoefficient));

    content.replace(QStringLiteral("{{MOLD_DENSITY}}"), number(mold.density));
    content.replace(QStringLiteral("{{MOLD_ELASTIC_MODULUS}}"), number(mold.elasticModulus));
    content.replace(QStringLiteral("{{MOLD_POISSON_RATIO}}"), number(mold.poissonRatio));
    content.replace(QStringLiteral("{{MOLD_THERMAL_CONDUCTIVITY}}"), number(mold.thermalConductivity));
    content.replace(QStringLiteral("{{MOLD_SPECIFIC_HEAT}}"), number(mold.specificHeat));

    content.replace(QStringLiteral("{{AMBIENT_TEMPERATURE}}"), number(boundary.ambientTemperature));

    content.replace(QStringLiteral("{{SIMULATION_TIME_LENGTH}}"), number(simulation.timeLength));

    QString caeSavePath =
        QDir::fromNativeSeparators(
            projectDir.filePath(QStringLiteral("abaqus/guhua"))
        );
    caeSavePath.replace(QStringLiteral("'"), QStringLiteral("\\'"));
    content.replace(QStringLiteral("{{CAE_SAVE_PATH}}"), caeSavePath);

    if (content.contains(QLatin1String("{{"))) {
        errorMessage = QStringLiteral("模板占位符替换失败。");
        return false;
    }

    const QString t0OutputPath = projectDir.filePath(QStringLiteral("abaqus/t0.py"));
    if (!saveFile(t0OutputPath, content, errorMessage)) {
        return false;
    }

    // =========================================================
    // 生成 335K.for
    // =========================================================
    QString forContent;
    if (!loadTemplate(
            QStringLiteral(":/simulation/templates/335K.for"),
            forContent,
            errorMessage)) {
        return false;
    }

    const QString forOutputPath = projectDir.filePath(QStringLiteral("abaqus/335K.for"));
    if (!saveFile(forOutputPath, forContent, errorMessage)) {
        return false;
    }

    // =========================================================
    // 生成 t1.py
    // =========================================================
    QString t1Content;
    if (!loadTemplate(
            QStringLiteral(":/simulation/templates/t1.py"),
            t1Content,
            errorMessage)) {
        return false;
    }

    const QStringList t1RequiredPlaceholders = {
        QStringLiteral("{{ABAQUS_WORK_DIR}}"),
        QStringLiteral("{{CAE_FILE_PATH}}"),
        QStringLiteral("{{USER_SUBROUTINE_PATH}}")
    };

    for (const QString &placeholder : t1RequiredPlaceholders) {
        if (!t1Content.contains(placeholder)) {
            errorMessage =
                QStringLiteral("t1 模板缺少占位符：%1").arg(placeholder);
            return false;
        }
    }

    QString abaqusWorkDir =
        QDir::fromNativeSeparators(
            projectDir.filePath(QStringLiteral("abaqus"))
        );
    QString caeFilePath =
        QDir::fromNativeSeparators(
            projectDir.filePath(QStringLiteral("abaqus/guhua.cae"))
        );
    QString userSubroutinePath =
        QDir::fromNativeSeparators(
            projectDir.filePath(QStringLiteral("abaqus/335K.for"))
        );

    abaqusWorkDir.replace(QStringLiteral("'"), QStringLiteral("\\'"));
    caeFilePath.replace(QStringLiteral("'"), QStringLiteral("\\'"));
    userSubroutinePath.replace(QStringLiteral("'"), QStringLiteral("\\'"));

    t1Content.replace(QStringLiteral("{{ABAQUS_WORK_DIR}}"), abaqusWorkDir);
    t1Content.replace(QStringLiteral("{{CAE_FILE_PATH}}"), caeFilePath);
    t1Content.replace(QStringLiteral("{{USER_SUBROUTINE_PATH}}"), userSubroutinePath);

    if (t1Content.contains(QLatin1String("{{"))) {
        errorMessage = QStringLiteral("t1 模板占位符替换失败。");
        return false;
    }

    const QString t1OutputPath = projectDir.filePath(QStringLiteral("abaqus/t1.py"));
    if (!saveFile(t1OutputPath, t1Content, errorMessage)) {
        return false;
    }

    return true;
}
