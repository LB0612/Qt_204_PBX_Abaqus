/**
 * @file ProjectManager.cpp
 * @brief 项目管理类的完整实现文件
 * 包含：JSON数据存取、Polyflow模板智能生成、文件生成等核心逻辑
 */

#include "ProjectManager.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QSaveFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>

// ==========================================================================
// 1. 内部辅助函数
// ==========================================================================

// 将字符串数值强制转为 Polyflow 标准科学计数法 (1.2345678E+01)
static QString toSci(const QString& val) {
    bool ok;
    double d = val.toDouble(&ok);
    if (!ok) return val;
    return QString::asprintf("%1.7E", d);
}

// 执行通用的占位符替换逻辑
static void performUnifiedReplace(QString &content, const QMap<QString, QString> &dict) {
    static const QRegularExpression tagRe("#[A-Z0-9_]+#") ;
    QRegularExpressionMatchIterator it = tagRe.globalMatch(content);
    
    // 使用结构体存储匹配信息
    struct MatchInfo {
        int  start;
        int  length;
        QString tag;
    };
    QList<MatchInfo> matches;

    // 1. 收集所有标签 (必须从后往前替换，否则坐标会乱，所以这里先收集再倒序处理)
    while  (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        matches.append({match.capturedStart(), match.capturedLength(), match.captured()});
    }

    // 2. 倒序执行替换 (从文件末尾开始)
    for (int i = matches.size() - 1; i >= 0 ; --i) {
        const  MatchInfo &info = matches[i];
        if  (dict.contains(info.tag)) {
            QString valStr = dict[info.tag];
            content.replace(info.start, info.length, valStr);
        }
    }
}

// 通用文件写入函数
bool ProjectManager::writePolyflowFile(const QString &targetPath, const QString &templatePath, const QMap<QString, QString> &dict) {
    QFile tplFile(templatePath);
    if (!tplFile.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QString content = tplFile.readAll();
    tplFile.close();

    QString mutableContent = content;
    performUnifiedReplace(mutableContent, dict);

    QSaveFile outFile(targetPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&outFile);
    out << mutableContent;
    return outFile.commit();
}

// 根据注释查找 Block 索引
int ProjectManager::findBlockIndexByComment(const QString &content, const QString &blockName, const QString &commentSnippet)
{
    int index = 1;
    int searchPos = 0;
    while (true) {
        int blockStart = content.indexOf("BEGIN " + blockName, searchPos);
        if (blockStart == -1) break;

        int endIndex = content.indexOf("END " + blockName, blockStart);
        int endofIndex = content.indexOf("ENDOF " + blockName, blockStart);
        int blockEnd = -1;

        if (endIndex != -1 && endofIndex != -1) blockEnd = qMin(endIndex, endofIndex);
        else if (endIndex != -1) blockEnd = endIndex;
        else if (endofIndex != -1) blockEnd = endofIndex;
        else break;

        QString blockContent = content.mid(blockStart, blockEnd - blockStart);
        if (blockContent.contains(commentSnippet, Qt::CaseInsensitive)) return index;

        searchPos = blockEnd + 1;
        index++;
    }
    return -1;
}

// ==========================================================================
// 2. 基础项目管理 (ProjectConfig)
// ==========================================================================

bool ProjectManager::saveProject(const QString &folderPath, const ProjectConfig &config)
{
    QJsonObject jsonObj;
    jsonObj["projectName"] = config.projectName;
    jsonObj["processType"] = config.processType;
    jsonObj["createdDate"] = config.createdDate;
    jsonObj["version"] = config.version;

    QSaveFile file(QDir(folderPath).filePath(FILE_PROJECT));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    file.write(QJsonDocument(jsonObj).toJson(QJsonDocument::Indented));
    return file.commit();
}

bool ProjectManager::loadProject(const QString &folderPath, ProjectConfig &config)
{
    QDir dir(folderPath);
    QFile file(dir.filePath(FILE_PROJECT));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QJsonObject jsonObj = QJsonDocument::fromJson(file.readAll()).object();

    config.projectName = dir.dirName(); // 强制使用文件夹名
    config.processType = jsonObj.value("processType").toInt(0);
    config.createdDate = jsonObj.value("createdDate").toString();
    config.version = jsonObj.value("version").toString();
    config.projectPath = folderPath;
    return true;
}

// ==========================================================================
// 3. 捏合工艺数据存取 (Niehe Functions) - 全部补全
// ==========================================================================

// --- 3.1 炸药参数 ---
bool ProjectManager::saveNieheExplosive(const QString &folderPath, const NieheExplosiveData &data) {
    QJsonObject jsonObj;
    jsonObj["density"] = data.density;
    jsonObj["specificHeat"] = data.specificHeat;
    jsonObj["conductivity"] = data.conductivity;
    jsonObj["initialTemp"] = data.initialTemp;
    jsonObj["powerLawFac"] = data.powerLawFac;
    jsonObj["powerLawTnat"] = data.powerLawTnat;
    jsonObj["powerLawExpo"] = data.powerLawExpo;
    QSaveFile file(QDir(folderPath).filePath(FILE_NIEHE_EXPLOSIVE));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    file.write(QJsonDocument(jsonObj).toJson(QJsonDocument::Indented));
    return file.commit();
}
bool ProjectManager::loadNieheExplosive(const QString &folderPath, NieheExplosiveData &data) {
    QFile file(QDir(folderPath).filePath(FILE_NIEHE_EXPLOSIVE));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { data = getDefaultNieheExplosive(); return false; }
    QJsonObject jsonObj = QJsonDocument::fromJson(file.readAll()).object();
    data.density = jsonObj.value("density").toString(data.density);
    data.specificHeat = jsonObj.value("specificHeat").toString(data.specificHeat);
    data.conductivity = jsonObj.value("conductivity").toString(data.conductivity);
    data.initialTemp = jsonObj.value("initialTemp").toString(data.initialTemp);
    data.powerLawFac = jsonObj.value("powerLawFac").toString(data.powerLawFac);
    data.powerLawTnat = jsonObj.value("powerLawTnat").toString(data.powerLawTnat);
    data.powerLawExpo = jsonObj.value("powerLawExpo").toString(data.powerLawExpo);
    return true;
}
NieheExplosiveData ProjectManager::getDefaultNieheExplosive() { return NieheExplosiveData(); }

// --- 3.2 桨叶参数 ---
bool ProjectManager::saveNieheParameters(const QString &folderPath, const NieheParameters &params) {
    QJsonObject jsonObj;
    jsonObj["solidRotationPointX"] = params.solidRotationPointX;
    jsonObj["solidRotationPointY"] = params.solidRotationPointY;
    jsonObj["solidRotationPointZ"] = params.solidRotationPointZ;
    jsonObj["solidRotationAxisX"] = params.solidRotationAxisX;
    jsonObj["solidRotationAxisY"] = params.solidRotationAxisY;
    jsonObj["solidRotationAxisZ"] = params.solidRotationAxisZ;
    jsonObj["solidBladeDensity"] = params.solidBladeDensity;
    jsonObj["solidBladeThermalConductivity"] = params.solidBladeThermalConductivity;
    jsonObj["solidBladeSpecificHeat"] = params.solidBladeSpecificHeat;
    jsonObj["solidBladeSpeed"] = params.solidBladeSpeed;
    jsonObj["hollowRotationPointX"] = params.hollowRotationPointX;
    jsonObj["hollowRotationPointY"] = params.hollowRotationPointY;
    jsonObj["hollowRotationPointZ"] = params.hollowRotationPointZ;
    jsonObj["hollowRotationAxisX"] = params.hollowRotationAxisX;
    jsonObj["hollowRotationAxisY"] = params.hollowRotationAxisY;
    jsonObj["hollowRotationAxisZ"] = params.hollowRotationAxisZ;
    jsonObj["hollowBladeDensity"] = params.hollowBladeDensity;
    jsonObj["hollowBladeThermalConductivity"] = params.hollowBladeThermalConductivity;
    jsonObj["hollowBladeSpecificHeat"] = params.hollowBladeSpecificHeat;
    jsonObj["hollowBladeSpeed"] = params.hollowBladeSpeed;
    QSaveFile file(QDir(folderPath).filePath(FILE_NIEHE_PARAMETERS));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    file.write(QJsonDocument(jsonObj).toJson(QJsonDocument::Indented));
    return file.commit();
}
bool ProjectManager::loadNieheParameters(const QString &folderPath, NieheParameters &params) {
    QFile file(QDir(folderPath).filePath(FILE_NIEHE_PARAMETERS));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { params = getDefaultNieheParameters(); return false; }
    QJsonObject jsonObj = QJsonDocument::fromJson(file.readAll()).object();
    params.solidRotationPointX = jsonObj.value("solidRotationPointX").toString(params.solidRotationPointX);
    params.solidRotationPointY = jsonObj.value("solidRotationPointY").toString(params.solidRotationPointY);
    params.solidRotationPointZ = jsonObj.value("solidRotationPointZ").toString(params.solidRotationPointZ);
    params.solidRotationAxisX = jsonObj.value("solidRotationAxisX").toString(params.solidRotationAxisX);
    params.solidRotationAxisY = jsonObj.value("solidRotationAxisY").toString(params.solidRotationAxisY);
    params.solidRotationAxisZ = jsonObj.value("solidRotationAxisZ").toString(params.solidRotationAxisZ);
    params.solidBladeDensity = jsonObj.value("solidBladeDensity").toString(params.solidBladeDensity);
    params.solidBladeThermalConductivity = jsonObj.value("solidBladeThermalConductivity").toString(params.solidBladeThermalConductivity);
    params.solidBladeSpecificHeat = jsonObj.value("solidBladeSpecificHeat").toString(params.solidBladeSpecificHeat);
    params.solidBladeSpeed = jsonObj.value("solidBladeSpeed").toString(params.solidBladeSpeed);
    params.hollowRotationPointX = jsonObj.value("hollowRotationPointX").toString(params.hollowRotationPointX);
    params.hollowRotationPointY = jsonObj.value("hollowRotationPointY").toString(params.hollowRotationPointY);
    params.hollowRotationPointZ = jsonObj.value("hollowRotationPointZ").toString(params.hollowRotationPointZ);
    params.hollowRotationAxisX = jsonObj.value("hollowRotationAxisX").toString(params.hollowRotationAxisX);
    params.hollowRotationAxisY = jsonObj.value("hollowRotationAxisY").toString(params.hollowRotationAxisY);
    params.hollowRotationAxisZ = jsonObj.value("hollowRotationAxisZ").toString(params.hollowRotationAxisZ);
    params.hollowBladeDensity = jsonObj.value("hollowBladeDensity").toString(params.hollowBladeDensity);
    params.hollowBladeThermalConductivity = jsonObj.value("hollowBladeThermalConductivity").toString(params.hollowBladeThermalConductivity);
    params.hollowBladeSpecificHeat = jsonObj.value("hollowBladeSpecificHeat").toString(params.hollowBladeSpecificHeat);
    params.hollowBladeSpeed = jsonObj.value("hollowBladeSpeed").toString(params.hollowBladeSpeed);
    return true;
}
NieheParameters ProjectManager::getDefaultNieheParameters() { return NieheParameters(); }

// --- 3.3 边界条件 ---
bool ProjectManager::saveNieheBoundary(const QString &folderPath, const NieheBoundaryData &data) {
    QJsonObject jsonObj;
    jsonObj["wallUpTemp"] = data.wallUpTemp;
    jsonObj["wallTemp"] = data.wallTemp;
    jsonObj["wallDownTemp"] = data.wallDownTemp;
    jsonObj["rotationPoint1X"] = data.rotationPoint1X;
    jsonObj["rotationPoint1Y"] = data.rotationPoint1Y;
    jsonObj["rotationPoint1Z"] = data.rotationPoint1Z;
    jsonObj["rotationPoint2X"] = data.rotationPoint2X;
    jsonObj["rotationPoint2Y"] = data.rotationPoint2Y;
    jsonObj["rotationPoint2Z"] = data.rotationPoint2Z;
    jsonObj["wallRotationSpeed"] = data.wallRotationSpeed;
    QSaveFile file(QDir(folderPath).filePath(FILE_NIEHE_BOUNDARY));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    file.write(QJsonDocument(jsonObj).toJson(QJsonDocument::Indented));
    return file.commit();
}
bool ProjectManager::loadNieheBoundary(const QString &folderPath, NieheBoundaryData &data) {
    QFile file(QDir(folderPath).filePath(FILE_NIEHE_BOUNDARY));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { data = getDefaultNieheBoundary(); return false; }
    QJsonObject jsonObj = QJsonDocument::fromJson(file.readAll()).object();
    data.wallUpTemp = jsonObj.value("wallUpTemp").toString(data.wallUpTemp);
    data.wallTemp = jsonObj.value("wallTemp").toString(data.wallTemp);
    data.wallDownTemp = jsonObj.value("wallDownTemp").toString(data.wallDownTemp);
    data.rotationPoint1X = jsonObj.value("rotationPoint1X").toString(data.rotationPoint1X);
    data.rotationPoint1Y = jsonObj.value("rotationPoint1Y").toString(data.rotationPoint1Y);
    data.rotationPoint1Z = jsonObj.value("rotationPoint1Z").toString(data.rotationPoint1Z);
    data.rotationPoint2X = jsonObj.value("rotationPoint2X").toString(data.rotationPoint2X);
    data.rotationPoint2Y = jsonObj.value("rotationPoint2Y").toString(data.rotationPoint2Y);
    data.rotationPoint2Z = jsonObj.value("rotationPoint2Z").toString(data.rotationPoint2Z);
    data.wallRotationSpeed = jsonObj.value("wallRotationSpeed").toString(data.wallRotationSpeed);
    return true;
}
NieheBoundaryData ProjectManager::getDefaultNieheBoundary() { return NieheBoundaryData(); }

// --- 3.4 仿真设置 ---
bool ProjectManager::saveNieheSimulation(const QString &folderPath, const NieheSimulationData &data) {
    QJsonObject jsonObj;
    jsonObj["maxTime"] = data.maxTime;
    jsonObj["initTimeStep"] = data.initTimeStep;
    jsonObj["minTimeStep"] = data.minTimeStep;
    jsonObj["maxTimeStep"] = data.maxTimeStep;
    jsonObj["tolerance"] = data.tolerance;
    jsonObj["maxSuccessSteps"] = data.maxSuccessSteps;
    QSaveFile file(QDir(folderPath).filePath(FILE_NIEHE_SIMULATION));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    file.write(QJsonDocument(jsonObj).toJson(QJsonDocument::Indented));
    return file.commit();
}
bool ProjectManager::loadNieheSimulation(const QString &folderPath, NieheSimulationData &data) {
    QFile file(QDir(folderPath).filePath(FILE_NIEHE_SIMULATION));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { data = getDefaultNieheSimulation(); return false; }
    QJsonObject jsonObj = QJsonDocument::fromJson(file.readAll()).object();
    data.maxTime = jsonObj.value("maxTime").toString(data.maxTime);
    data.initTimeStep = jsonObj.value("initTimeStep").toString(data.initTimeStep);
    data.minTimeStep = jsonObj.value("minTimeStep").toString(data.minTimeStep);
    data.maxTimeStep = jsonObj.value("maxTimeStep").toString(data.maxTimeStep);
    data.tolerance = jsonObj.value("tolerance").toString(data.tolerance);
    data.maxSuccessSteps = jsonObj.value("maxSuccessSteps").toString(data.maxSuccessSteps);
    return true;
}
NieheSimulationData ProjectManager::getDefaultNieheSimulation() { return NieheSimulationData(); }


// ==========================================================================
// 4. 挤压工艺数据存取 (Extrusion Functions) - 全部补全
// ==========================================================================

// --- 4.1 炸药参数 ---
bool ProjectManager::saveExtrusionExplosive(const QString &folderPath, const ExtrusionExplosiveData &data) {
    QJsonObject jsonObj;
    jsonObj["density"] = data.density;
    jsonObj["specificHeat"] = data.specificHeat;
    jsonObj["conductivity"] = data.conductivity;
    jsonObj["initialTemp"] = data.initialTemp;
    jsonObj["powerLawFac"] = data.powerLawFac;
    jsonObj["powerLawTnat"] = data.powerLawTnat;
    jsonObj["powerLawExpo"] = data.powerLawExpo;
    QSaveFile file(QDir(folderPath).filePath(FILE_EXTRUSION_EXPLOSIVE));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    file.write(QJsonDocument(jsonObj).toJson(QJsonDocument::Indented));
    return file.commit();
}
bool ProjectManager::loadExtrusionExplosive(const QString &folderPath, ExtrusionExplosiveData &data) {
    QFile file(QDir(folderPath).filePath(FILE_EXTRUSION_EXPLOSIVE));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { data = getDefaultExtrusionExplosive(); return false; }
    QJsonObject jsonObj = QJsonDocument::fromJson(file.readAll()).object();
    data.density = jsonObj.value("density").toString(data.density);
    data.specificHeat = jsonObj.value("specificHeat").toString(data.specificHeat);
    data.conductivity = jsonObj.value("conductivity").toString(data.conductivity);
    data.initialTemp = jsonObj.value("initialTemp").toString(data.initialTemp);
    data.powerLawFac = jsonObj.value("powerLawFac").toString(data.powerLawFac);
    data.powerLawTnat = jsonObj.value("powerLawTnat").toString(data.powerLawTnat);
    data.powerLawExpo = jsonObj.value("powerLawExpo").toString(data.powerLawExpo);
    return true;
}
ExtrusionExplosiveData ProjectManager::getDefaultExtrusionExplosive() { return ExtrusionExplosiveData(); }

// --- 4.2 螺杆参数 ---
bool ProjectManager::saveExtrusionScrew(const QString &folderPath, const ExtrusionScrewData &data) {
    QJsonObject jsonObj;
    jsonObj["solidRotationPointX"] = data.solidRotationPointX;
    jsonObj["solidRotationPointY"] = data.solidRotationPointY;
    jsonObj["solidRotationPointZ"] = data.solidRotationPointZ;
    jsonObj["solidRotationAxisX"] = data.solidRotationAxisX;
    jsonObj["solidRotationAxisY"] = data.solidRotationAxisY;
    jsonObj["solidRotationAxisZ"] = data.solidRotationAxisZ;
    jsonObj["solidBladeSpeed"] = data.solidBladeSpeed;
    jsonObj["solidBladeDensity"] = data.solidBladeDensity;
    jsonObj["solidBladeThermalConductivity"] = data.solidBladeThermalConductivity;
    jsonObj["solidBladeHeatTransfer"] = data.solidBladeHeatTransfer;
    QSaveFile file(QDir(folderPath).filePath(FILE_EXTRUSION_SCREW));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    file.write(QJsonDocument(jsonObj).toJson(QJsonDocument::Indented));
    return file.commit();
}
bool ProjectManager::loadExtrusionScrew(const QString &folderPath, ExtrusionScrewData &data) {
    QFile file(QDir(folderPath).filePath(FILE_EXTRUSION_SCREW));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { data = getDefaultExtrusionScrew(); return false; }
    QJsonObject jsonObj = QJsonDocument::fromJson(file.readAll()).object();
    data.solidRotationPointX = jsonObj.value("solidRotationPointX").toString(data.solidRotationPointX);
    data.solidRotationPointY = jsonObj.value("solidRotationPointY").toString(data.solidRotationPointY);
    data.solidRotationPointZ = jsonObj.value("solidRotationPointZ").toString(data.solidRotationPointZ);
    data.solidRotationAxisX = jsonObj.value("solidRotationAxisX").toString(data.solidRotationAxisX);
    data.solidRotationAxisY = jsonObj.value("solidRotationAxisY").toString(data.solidRotationAxisY);
    data.solidRotationAxisZ = jsonObj.value("solidRotationAxisZ").toString(data.solidRotationAxisZ);
    data.solidBladeSpeed = jsonObj.value("solidBladeSpeed").toString(data.solidBladeSpeed);
    data.solidBladeDensity = jsonObj.value("solidBladeDensity").toString(data.solidBladeDensity);
    data.solidBladeThermalConductivity = jsonObj.value("solidBladeThermalConductivity").toString(data.solidBladeThermalConductivity);
    data.solidBladeHeatTransfer = jsonObj.value("solidBladeHeatTransfer").toString(data.solidBladeHeatTransfer);
    return true;
}
ExtrusionScrewData ProjectManager::getDefaultExtrusionScrew() { return ExtrusionScrewData(); }

// --- 4.3 边界条件 ---
bool ProjectManager::saveExtrusionBoundary(const QString &folderPath, const ExtrusionBoundaryData &data) {
    QJsonObject jsonObj;
    jsonObj["wallUpTemp"] = data.wallUpTemp;
    jsonObj["wallTemp"] = data.wallTemp;
    jsonObj["wallDownTemp"] = data.wallDownTemp;
    jsonObj["rotationPoint1X"] = data.rotationPoint1X;
    jsonObj["rotationPoint1Y"] = data.rotationPoint1Y;
    jsonObj["rotationPoint1Z"] = data.rotationPoint1Z;
    QSaveFile file(QDir(folderPath).filePath(FILE_EXTRUSION_BOUNDARY));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    file.write(QJsonDocument(jsonObj).toJson(QJsonDocument::Indented));
    return file.commit();
}
bool ProjectManager::loadExtrusionBoundary(const QString &folderPath, ExtrusionBoundaryData &data) {
    QFile file(QDir(folderPath).filePath(FILE_EXTRUSION_BOUNDARY));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { data = getDefaultExtrusionBoundary(); return false; }
    QJsonObject jsonObj = QJsonDocument::fromJson(file.readAll()).object();
    data.wallUpTemp = jsonObj.value("wallUpTemp").toString(data.wallUpTemp);
    data.wallTemp = jsonObj.value("wallTemp").toString(data.wallTemp);
    data.wallDownTemp = jsonObj.value("wallDownTemp").toString(data.wallDownTemp);
    data.rotationPoint1X = jsonObj.value("rotationPoint1X").toString(data.rotationPoint1X);
    data.rotationPoint1Y = jsonObj.value("rotationPoint1Y").toString(data.rotationPoint1Y);
    data.rotationPoint1Z = jsonObj.value("rotationPoint1Z").toString(data.rotationPoint1Z);
    return true;
}
ExtrusionBoundaryData ProjectManager::getDefaultExtrusionBoundary() { return ExtrusionBoundaryData(); }

// --- 4.4 仿真设置 ---
bool ProjectManager::saveExtrusionSimulation(const QString &folderPath, const ExtrusionSimulationData &data) {
    QJsonObject jsonObj;
    jsonObj["maxTime"] = data.maxTime;
    jsonObj["initTimeStep"] = data.initTimeStep;
    jsonObj["minTimeStep"] = data.minTimeStep;
    jsonObj["maxTimeStep"] = data.maxTimeStep;
    jsonObj["tolerance"] = data.tolerance;
    jsonObj["maxSuccessSteps"] = data.maxSuccessSteps;
    QSaveFile file(QDir(folderPath).filePath(FILE_EXTRUSION_SIMULATION));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    file.write(QJsonDocument(jsonObj).toJson(QJsonDocument::Indented));
    return file.commit();
}
bool ProjectManager::loadExtrusionSimulation(const QString &folderPath, ExtrusionSimulationData &data) {
    QFile file(QDir(folderPath).filePath(FILE_EXTRUSION_SIMULATION));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { data = getDefaultExtrusionSimulation(); return false; }
    QJsonObject jsonObj = QJsonDocument::fromJson(file.readAll()).object();
    data.maxTime = jsonObj.value("maxTime").toString(data.maxTime);
    data.initTimeStep = jsonObj.value("initTimeStep").toString(data.initTimeStep);
    data.minTimeStep = jsonObj.value("minTimeStep").toString(data.minTimeStep);
    data.maxTimeStep = jsonObj.value("maxTimeStep").toString(data.maxTimeStep);
    data.tolerance = jsonObj.value("tolerance").toString(data.tolerance);
    data.maxSuccessSteps = jsonObj.value("maxSuccessSteps").toString(data.maxSuccessSteps);
    return true;
}
ExtrusionSimulationData ProjectManager::getDefaultExtrusionSimulation() { return ExtrusionSimulationData(); }


// ==========================================================================
// 5. 核心逻辑：模板生成与占位符替换 (Core Template Logic)
// ==========================================================================

// 辅助函数：准备模板文件（检查坏模板并生成新模板）
bool ProjectManager::prepareTemplate(const QString &targetPath, const QString &templatePath) {
    // 【核心修复：坏模板自毁逻辑】
    // 如果模板存在，但里面不包含任何标签（#），说明是无效模板，强制删除
    if (QFile::exists(templatePath)) {
        QFile f(templatePath);
        if (f.open(QIODevice::ReadOnly)) {
            QString c = f.readAll();
            // 只要没检测到 #EXP 或 #SIM 这种典型标签，就认为是坏的
            if (!c.contains("#EXP_") && !c.contains("#SIM_")) {
                f.close();
                QFile::remove(templatePath);
            } else {
                f.close();
            }
        }
    }

    // 只有模板真的不存在时，才去尝试生成 
    if (!QFile::exists(templatePath)) {
        if (!QFile::exists(targetPath)) {
            qDebug() << "[Error] 既无模板也无目标文件，无法生成。";
            return false;
        }
        if (!createTemplateFromRaw(targetPath, templatePath)) {
            qDebug() << "[Error] 从目标文件反向生成模板失败。";
            return false;
        }
    }
    return true;
}

bool ProjectManager::createTemplateFromRaw(const QString &rawPath, const QString &templatePath)
{
    QFile file(rawPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {

        return false;
    }
    QString content = file.readAll();
    file.close();

    // 正则匹配科学计数法 (如 1.23E+01)
    static const QRegularExpression numRe(R"([-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?)");

    // 定义核心替换逻辑 Lambda
    // 返回 bool 用于统计是否替换成功
    auto replaceVal = [&](const QString &blockKey, int nthBlock, const QString &prefix, const QString &tag, int nthPrefix, int nthValue) -> bool {
        int blockStartPos = -1;
        int searchPos = 0;
        
        // 1. 定位 Block (如 BEGIN MASPE)
        for (int i = 0; i < nthBlock; ++i) {
            blockStartPos = content.indexOf(blockKey, searchPos);
            if (blockStartPos == -1) {

                return false;
            }
            searchPos = blockStartPos + blockKey.length();
        }

        // 2. 定位 Block 结束位置，防止越界
        // 尝试匹配 END 或 ENDOF
        int endIndex = content.indexOf("END " + blockKey.mid(6), blockStartPos);
        int endofIndex = content.indexOf("ENDOF " + blockKey.mid(6), blockStartPos);
        
        // 取最近的一个结束标记
        if (endIndex == -1) endIndex = endofIndex;
        else if (endofIndex != -1) endIndex = qMin(endIndex, endofIndex);

        if (endIndex == -1) {

             return false;
        }

        // 3. 构建高容错正则表达式
        // 原始 prefix 可能是 "D 1|"
        // 我们需要把它变成 "D\\s+1\\s*\\|" 以匹配 "D 1 |", "D   1|", "D  1  |" 等各种情况
        QString fuzzyPrefix = prefix;
        // (1) 先转义竖线，并允许竖线前有任意空格 (\\s*)
        fuzzyPrefix.replace("|", "\\s*\\|");
        // (2) 将原字符串中的空格转为匹配至少一个空格 (\\s+)
        fuzzyPrefix.replace(" ", "\\s+");     
        
        QRegularExpression prefixRe(fuzzyPrefix);

        // 4. 在 Block 范围内查找前缀
        int targetPrefixEndPos = -1;
        int foundCount = 0;
        int scanPos = blockStartPos;

        while (foundCount < nthPrefix && scanPos < endIndex) {
            QRegularExpressionMatch m = prefixRe.match(content, scanPos);
            // 没找到，或者找到的位置跑出了 Block 范围
            if (!m.hasMatch() || m.capturedStart() > endIndex) {

                 return false;
            }
            targetPrefixEndPos = m.capturedEnd();
            scanPos = targetPrefixEndPos;
            foundCount++;
        }

        // 5. 查找数值并替换
        QRegularExpressionMatchIterator it = numRe.globalMatch(content, targetPrefixEndPos);

        for (int i = 0; i < nthValue; ++i) {
            if (!it.hasNext()) {

                 return false;
            }
            QRegularExpressionMatch match = it.next();
            // 再次检查是否越界
            if (match.capturedStart() > endIndex) {

                return false;
            }

            if (i == nthValue - 1) {
                // 【核心操作】执行替换
                // 强制在 tag 前加一个空格，防止与前缀粘连
                content.replace(match.capturedStart(), match.capturedLength(), " " + tag);

                return true;
            }
        }
        return false;
    };

    replaceVal("BEGIN MASPE", 1, "D 1|", "#EXP_DENSITY#",    1, 1);
    replaceVal("BEGIN HECAP", 1, "D 1|", "#EXP_SPEC_HEAT#",  1, 1);
    replaceVal("BEGIN CONDU", 1, "D 1|", "#EXP_COND#",       1, 1);
    replaceVal("BEGIN TINIT", 1, "D 5|", "#EXP_INIT_TEMP#",  1, 1);
    replaceVal("BEGIN POWER", 1, "D 1|", "#EXP_PL_FAC#",     1, 1);
    replaceVal("BEGIN POWER", 1, "D 1|", "#EXP_PL_TNAT#",    2, 1);
    replaceVal("BEGIN POWER", 1, "D 1|", "#EXP_PL_EXPO#",    3, 1);

    bool isJiya = rawPath.contains("jiya", Qt::CaseInsensitive);

    if (isJiya) {
        replaceVal("BEGIN IMPMO", 1, "D 3|", "#SB_ROT_Z#", 3, 3);
        replaceVal("BEGIN IMPMO", 1, "D 3|", "#SB_ROT_Y#", 3, 2);
        replaceVal("BEGIN IMPMO", 1, "D 3|", "#SB_ROT_X#", 3, 1);
        replaceVal("BEGIN IMPMO", 1, "D 3|", "#SB_AX_Z#",  4, 3);
        replaceVal("BEGIN IMPMO", 1, "D 3|", "#SB_AX_Y#",  4, 2);
        replaceVal("BEGIN IMPMO", 1, "D 3|", "#SB_AX_X#",  4, 1);
        replaceVal("BEGIN IMPMO", 1, "D 1|", "#SB_SPD#",   2, 1);

        replaceVal("BEGIN MASPE", 2, "D 1|", "#SB_DENS#",  1, 1);
        replaceVal("BEGIN CONDU", 2, "D 1|", "#SB_COND#",  1, 1);
        replaceVal("BEGIN HECAP", 2, "D 1|", "#SB_HEAT#",  1, 1);
    } else {
        // 捏合逻辑
        replaceVal("BEGIN IMPMO", 1, "D 3|", "#HB_ROT_Z#", 3, 3);
        replaceVal("BEGIN IMPMO", 1, "D 3|", "#HB_ROT_Y#", 3, 2);
        replaceVal("BEGIN IMPMO", 1, "D 3|", "#HB_ROT_X#", 3, 1);
        replaceVal("BEGIN IMPMO", 1, "D 3|", "#HB_AX_Z#",  4, 3);
        replaceVal("BEGIN IMPMO", 1, "D 3|", "#HB_AX_Y#",  4, 2);
        replaceVal("BEGIN IMPMO", 1, "D 3|", "#HB_AX_X#",  4, 1);
        replaceVal("BEGIN IMPMO", 1, "D 1|", "#HB_SPD#",   2, 1);
        replaceVal("BEGIN MASPE", 2, "D 1|", "#HB_DENS#",  1, 1);
        replaceVal("BEGIN CONDU", 2, "D 1|", "#HB_COND#",  1, 1);
        replaceVal("BEGIN HECAP", 2, "D 1|", "#HB_HEAT#",  1, 1);

        replaceVal("BEGIN IMPMO", 2, "D 3|", "#SB_ROT_Z#", 3, 3);
        replaceVal("BEGIN IMPMO", 2, "D 3|", "#SB_ROT_Y#", 3, 2);
        replaceVal("BEGIN IMPMO", 2, "D 3|", "#SB_ROT_X#", 3, 1);
        replaceVal("BEGIN IMPMO", 2, "D 3|", "#SB_AX_Z#",  4, 3);
        replaceVal("BEGIN IMPMO", 2, "D 3|", "#SB_AX_Y#",  4, 2);
        replaceVal("BEGIN IMPMO", 2, "D 3|", "#SB_AX_X#",  4, 1);
        replaceVal("BEGIN IMPMO", 2, "D 1|", "#SB_SPD#",   2, 1);
        replaceVal("BEGIN MASPE", 3, "D 1|", "#SB_DENS#",  1, 1);
        replaceVal("BEGIN CONDU", 3, "D 1|", "#SB_COND#",  1, 1);
        replaceVal("BEGIN HECAP", 3, "D 1|", "#SB_HEAT#",  1, 1);
    }

    if (isJiya) { 
         int idxIn   = findBlockIndexByComment(content, "BDSTP", "along IN"); 
         int idxOut  = findBlockIndexByComment(content, "BDSTP", "along OUT"); 
         int idxWall = findBlockIndexByComment(content, "BDSTP", "along WALL"); 
         if (idxIn == -1) idxIn = 1; if (idxOut == -1) idxOut = 2; if (idxWall == -1) idxWall = 3; 
 
         replaceVal("BEGIN BDSTP", idxIn,   "D 5|", "#BND_WALL_UP#",   2, 1); 
         replaceVal("BEGIN BDSTP", idxOut,  "D 5|", "#BND_WALL_DOWN#", 2, 1); 
         replaceVal("BEGIN BDSTP", idxWall, "D 5|", "#BND_WALL_SIDE#", 2, 1); 
 
         int bdsveWallIdx = findBlockIndexByComment(content, "BDSVE", "along WALL"); 
         if (bdsveWallIdx == -1) bdsveWallIdx = 3; 
         replaceVal("BEGIN BDSVE", bdsveWallIdx, "D 5|", "#BND_ROT1_Z#", 2, 3); 
         replaceVal("BEGIN BDSVE", bdsveWallIdx, "D 5|", "#BND_ROT1_Y#", 2, 2); 
         replaceVal("BEGIN BDSVE", bdsveWallIdx, "D 5|", "#BND_ROT1_X#", 2, 1); 
     } else { 
         int bdsveIndex = findBlockIndexByComment(content, "BDSVE", "along WALL"); 
         if (bdsveIndex == -1) bdsveIndex = 3; 
         replaceVal("BEGIN BDSVE", bdsveIndex, "D 5|", "#WALL_ROT_SPD#", 2, 4); 
         replaceVal("BEGIN BDSVE", bdsveIndex, "D 5|", "#BND_ROT2_Z#",   2, 3); 
         replaceVal("BEGIN BDSVE", bdsveIndex, "D 5|", "#BND_ROT2_Y#",   2, 2); 
         replaceVal("BEGIN BDSVE", bdsveIndex, "D 5|", "#BND_ROT2_X#",   2, 1); 
         replaceVal("BEGIN BDSVE", bdsveIndex, "D 5|", "#BND_ROT1_Z#",   4, 3); 
         replaceVal("BEGIN BDSVE", bdsveIndex, "D 5|", "#BND_ROT1_Y#",   4, 2); 
         replaceVal("BEGIN BDSVE", bdsveIndex, "D 5|", "#BND_ROT1_X#",   4, 1); 
 
         int idxDown = findBlockIndexByComment(content, "BDSTP", "along DOWN"); 
         int idxUp   = findBlockIndexByComment(content, "BDSTP", "along UP"); 
         int idxSide = findBlockIndexByComment(content, "BDSTP", "along WALL"); 
         if (idxDown == -1) idxDown = 1; if (idxUp == -1) idxUp = 2; if (idxSide == -1) idxSide = 3; 
         replaceVal("BEGIN BDSTP", idxDown, "D 5|", "#BND_WALL_DOWN#", 2, 1); 
         replaceVal("BEGIN BDSTP", idxUp,   "D 5|", "#BND_WALL_UP#",   2, 1); 
         replaceVal("BEGIN BDSTP", idxSide, "D 5|", "#BND_WALL_SIDE#", 2, 1); 
     } 
 
     replaceVal("BEGIN NUPAR", 1, "D 6|", "#SIM_TOLERANCE#",  1, 6); 
     replaceVal("BEGIN NUPAR", 1, "D 6|", "#SIM_MAX_STEP#",   1, 5); 
     replaceVal("BEGIN NUPAR", 1, "D 6|", "#SIM_MIN_STEP#",   1, 4); 
     replaceVal("BEGIN NUPAR", 1, "D 6|", "#SIM_INIT_STEP#",  1, 3); 
     replaceVal("BEGIN NUPAR", 1, "D 6|", "#SIM_FINAL_TIME#", 1, 2); 
     replaceVal("BEGIN NUPAR", 1, "I 1|", "#SIM_MAX_STEPS#",  5, 1); 
 
     QSaveFile tplFile(templatePath); 
     if (!tplFile.open(QIODevice::WriteOnly | QIODevice::Text)) return false; 
     tplFile.write(content.toUtf8()); 
     bool ok = tplFile.commit(); 

     return ok;
}



bool ProjectManager::generateNiehePolyflowFile(const QString &projectPath, const NieheProjectData &data)
{
    // 【优化】添加详细的错误处理和日志
    QDir projectDir(projectPath); 
    QString targetPath = projectDir.filePath("25L/Simulation/mix/polyflow.dat"); 
    QString templatePath = projectDir.filePath("25L/Simulation/mix/polyflow_template.dat"); 

    // 【新增核心修复：使用辅助函数处理模板准备】
    if (!prepareTemplate(targetPath, templatePath)) {
        return false;
    } 

    QMap<QString, QString> dict; 
    dict["#EXP_DENSITY#"]    = toSci(data.explosive.density); 
    dict["#EXP_SPEC_HEAT#"]  = toSci(data.explosive.specificHeat); 
    dict["#EXP_COND#"]       = toSci(data.explosive.conductivity); 
    dict["#EXP_INIT_TEMP#"]  = toSci(data.explosive.initialTemp); 
    dict["#EXP_PL_FAC#"]     = toSci(data.explosive.powerLawFac); 
    dict["#EXP_PL_TNAT#"]    = toSci(data.explosive.powerLawTnat); 
    dict["#EXP_PL_EXPO#"]    = toSci(data.explosive.powerLawExpo); 

    dict["#HB_ROT_X#"] = toSci(data.parameters.hollowRotationPointX); 
    dict["#HB_ROT_Y#"] = toSci(data.parameters.hollowRotationPointY); 
    dict["#HB_ROT_Z#"] = toSci(data.parameters.hollowRotationPointZ); 
    dict["#HB_AX_X#"]  = toSci(data.parameters.hollowRotationAxisX); 
    dict["#HB_AX_Y#"]  = toSci(data.parameters.hollowRotationAxisY); 
    dict["#HB_AX_Z#"]  = toSci(data.parameters.hollowRotationAxisZ); 
    dict["#HB_SPD#"]   = toSci(data.parameters.hollowBladeSpeed); 
    dict["#HB_DENS#"]  = toSci(data.parameters.hollowBladeDensity); 
    dict["#HB_COND#"]  = toSci(data.parameters.hollowBladeThermalConductivity); 
    dict["#HB_HEAT#"]  = toSci(data.parameters.hollowBladeSpecificHeat); 

    dict["#SB_ROT_X#"] = toSci(data.parameters.solidRotationPointX); 
    dict["#SB_ROT_Y#"] = toSci(data.parameters.solidRotationPointY); 
    dict["#SB_ROT_Z#"] = toSci(data.parameters.solidRotationPointZ); 
    dict["#SB_AX_X#"]  = toSci(data.parameters.solidRotationAxisX); 
    dict["#SB_AX_Y#"]  = toSci(data.parameters.solidRotationAxisY); 
    dict["#SB_AX_Z#"]  = toSci(data.parameters.solidRotationAxisZ); 
    dict["#SB_SPD#"]   = toSci(data.parameters.solidBladeSpeed); 
    dict["#SB_DENS#"]  = toSci(data.parameters.solidBladeDensity); 
    dict["#SB_COND#"]  = toSci(data.parameters.solidBladeThermalConductivity); 
    dict["#SB_HEAT#"]  = toSci(data.parameters.solidBladeSpecificHeat); 

    dict["#BND_WALL_UP#"]    = toSci(data.boundary.wallUpTemp); 
    dict["#BND_WALL_DOWN#"]  = toSci(data.boundary.wallDownTemp); 
    dict["#BND_WALL_SIDE#"]  = toSci(data.boundary.wallTemp); 
    dict["#BND_ROT1_X#"]     = toSci(data.boundary.rotationPoint1X); 
    dict["#BND_ROT1_Y#"]     = toSci(data.boundary.rotationPoint1Y); 
    dict["#BND_ROT1_Z#"]     = toSci(data.boundary.rotationPoint1Z); 
    dict["#BND_ROT2_X#"]     = toSci(data.boundary.rotationPoint2X); 
    dict["#BND_ROT2_Y#"]     = toSci(data.boundary.rotationPoint2Y); 
    dict["#BND_ROT2_Z#"]     = toSci(data.boundary.rotationPoint2Z); 
    dict["#WALL_ROT_SPD#"]   = toSci(data.boundary.wallRotationSpeed); 

    dict["#SIM_FINAL_TIME#"] = toSci(data.simulation.maxTime); 
    dict["#SIM_INIT_STEP#"]  = toSci(data.simulation.initTimeStep); 
    dict["#SIM_MIN_STEP#"]   = toSci(data.simulation.minTimeStep); 
    dict["#SIM_MAX_STEP#"]   = toSci(data.simulation.maxTimeStep); 
    dict["#SIM_TOLERANCE#"]  = toSci(data.simulation.tolerance); 
    dict["#SIM_MAX_STEPS#"]  = data.simulation.maxSuccessSteps; 

    return writePolyflowFile(targetPath, templatePath, dict); 
}

bool ProjectManager::generateJiyaPolyflowFile(const QString &projectPath, const JiyaProjectData &data)
{
    // 1. 定义路径
    QDir projectDir(projectPath);
    QString targetPath = projectDir.filePath("jiya/Simulation/jiya/polyflow.dat");
    QString templatePath = projectDir.filePath("jiya/Simulation/jiya/polyflow_template.dat");

    // 【核心修复：使用辅助函数处理模板准备】
    if (!prepareTemplate(targetPath, templatePath)) {
        return false;
    }

    // 3. 准备数据字典
    QMap<QString, QString> dict;
    dict["#EXP_DENSITY#"]    = toSci(data.explosive.density);
    dict["#EXP_SPEC_HEAT#"]  = toSci(data.explosive.specificHeat);
    dict["#EXP_COND#"]       = toSci(data.explosive.conductivity);
    dict["#EXP_INIT_TEMP#"]  = toSci(data.explosive.initialTemp);
    dict["#EXP_PL_FAC#"]     = toSci(data.explosive.powerLawFac);
    dict["#EXP_PL_TNAT#"]    = toSci(data.explosive.powerLawTnat);
    dict["#EXP_PL_EXPO#"]    = toSci(data.explosive.powerLawExpo);

    dict["#SB_ROT_X#"] = toSci(data.screw.solidRotationPointX);
    dict["#SB_ROT_Y#"] = toSci(data.screw.solidRotationPointY);
    dict["#SB_ROT_Z#"] = toSci(data.screw.solidRotationPointZ);
    dict["#SB_AX_X#"]  = toSci(data.screw.solidRotationAxisX);
    dict["#SB_AX_Y#"]  = toSci(data.screw.solidRotationAxisY);
    dict["#SB_AX_Z#"]  = toSci(data.screw.solidRotationAxisZ);
    dict["#SB_SPD#"]   = toSci(data.screw.solidBladeSpeed);
    dict["#SB_DENS#"]  = toSci(data.screw.solidBladeDensity);
    dict["#SB_COND#"]  = toSci(data.screw.solidBladeThermalConductivity);
    dict["#SB_HEAT#"]  = toSci(data.screw.solidBladeHeatTransfer);

    dict["#BND_WALL_UP#"]   = toSci(data.boundary.wallUpTemp);
    dict["#BND_WALL_DOWN#"] = toSci(data.boundary.wallDownTemp);
    dict["#BND_WALL_SIDE#"] = toSci(data.boundary.wallTemp);

    dict["#BND_ROT1_X#"]    = toSci(data.boundary.rotationPoint1X);
    dict["#BND_ROT1_Y#"]    = toSci(data.boundary.rotationPoint1Y);
    dict["#BND_ROT1_Z#"]    = toSci(data.boundary.rotationPoint1Z);

    dict["#SIM_FINAL_TIME#"] = toSci(data.simulation.maxTime);
    dict["#SIM_INIT_STEP#"]  = toSci(data.simulation.initTimeStep);
    dict["#SIM_MIN_STEP#"]   = toSci(data.simulation.minTimeStep);
    dict["#SIM_MAX_STEP#"]   = toSci(data.simulation.maxTimeStep);
    dict["#SIM_TOLERANCE#"]  = toSci(data.simulation.tolerance);
    dict["#SIM_MAX_STEPS#"]  = data.simulation.maxSuccessSteps; // 注意：整数不转科学计数法

    // 4. 写入最终文件
    return writePolyflowFile(targetPath, templatePath, dict);
}

bool ProjectManager::copyDir(const QString &sourcePath, const QString &destPath, bool coverFileIfExist)
{
    QDir sourceDir(sourcePath);
    QDir destDir(destPath);
    if (destPath.startsWith(sourcePath)) return false;
    if (!sourceDir.exists()) return false;
    if (!destDir.exists() && !destDir.mkpath(destPath)) return false;

    QFileInfoList fileInfoList = sourceDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (QFileInfo fileInfo, fileInfoList) {
        if (fileInfo.isDir()) {
            if (!copyDir(fileInfo.filePath(), destPath + "/" + fileInfo.fileName(), coverFileIfExist)) return false;
        } else {
            QString destFile = destPath + "/" + fileInfo.fileName();
            if (coverFileIfExist && QFile::exists(destFile)) QFile::remove(destFile);
            if (!QFile::copy(fileInfo.filePath(), destFile)) return false;
        }
    }
    return true;
}
