#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>

// ============================
// 文件名字常量定义
// ============================
const QString FILE_PROJECT = "project.json";
const QString FILE_NIEHE_BOUNDARY = "niehe_boundary.json";
const QString FILE_NIEHE_PARAMETERS = "niehe_parameters.json";
const QString FILE_NIEHE_EXPLOSIVE = "niehe_explosive.json";
const QString FILE_NIEHE_SIMULATION = "niehe_simulation.json";
const QString FILE_EXTRUSION_SCREW = "extrusion_screw.json";
const QString FILE_EXTRUSION_BOUNDARY = "extrusion_boundary.json";
const QString FILE_EXTRUSION_EXPLOSIVE = "extrusion_explosive.json";
const QString FILE_EXTRUSION_SIMULATION = "extrusion_simulation.json";

struct ProjectConfig {
    QString projectName;
    QString projectPath;
    int processType;        // 0-捏合, 1-挤压
    QString createdDate;
    QString version;
};

// ============================
// 捏合工艺数据结构 (Niehe)
// ============================
struct NieheExplosiveData {
    QString density = "1.6800000E-09";
    QString specificHeat = "1.2290000E+03";
    QString conductivity = "2.3260000E-01";
    QString initialTemp = "3.0000000E+02";
    QString powerLawFac = "3.0000000E-01";
    QString powerLawTnat = "1.0000000E+00";
    QString powerLawExpo = "1.0000000E-01";
};

struct NieheParameters {
    // 实心桨
    QString solidRotationPointX = "4.6270000E+01";
    QString solidRotationPointY = "-1.0100000E+01";
    QString solidRotationPointZ = "2.7950000E+02";
    QString solidRotationAxisX = "0.0000000E+00";
    QString solidRotationAxisY = "0.0000000E+00";
    QString solidRotationAxisZ = "1.0000000E+00";
    QString solidBladeDensity = "8.0300000E-09";
    QString solidBladeThermalConductivity = "1.4500000E+01";
    QString solidBladeSpecificHeat = "5.0200000E+02";
    QString solidBladeSpeed = "-1.0000000E+01";

    // 空心桨
    QString hollowRotationPointX = "-8.5730000E+01";
    QString hollowRotationPointY = "-1.0100000E+01";
    QString hollowRotationPointZ = "2.7950000E+02";
    QString hollowRotationAxisX = "0.0000000E+00";
    QString hollowRotationAxisY = "0.0000000E+00";
    QString hollowRotationAxisZ = "1.0000000E+00";
    QString hollowBladeDensity = "8.0300000E-09";
    QString hollowBladeThermalConductivity = "1.4500000E+01";
    QString hollowBladeSpecificHeat = "5.0200000E+02";
    QString hollowBladeSpeed = "2.0000000E+01";
};

struct NieheBoundaryData {
    QString wallUpTemp = "3.0315000E+02";
    QString wallTemp = "3.3815000E+02";
    QString wallDownTemp = "3.3315000E+02";
    QString rotationPoint1X = "2.2700000E+00";
    QString rotationPoint1Y = "-1.0100000E+01";
    QString rotationPoint1Z = "2.7650000E+02";
    QString rotationPoint2X = "0.0000000E+00";
    QString rotationPoint2Y = "0.0000000E+00";
    QString rotationPoint2Z = "1.0000000E+00";
    QString wallRotationSpeed = "2.1000000E+00";
};

struct NieheSimulationData {
    QString maxTime = "3.0000000E+01";
    QString initTimeStep = "1.0000000E+00";
    QString minTimeStep = "1.0000000E-03";
    QString maxTimeStep = "1.0000000E+00";
    QString tolerance = "1.0000000E+04";
    QString maxSuccessSteps = "20000";
};

struct NieheProjectData {
    NieheExplosiveData explosive;
    NieheBoundaryData boundary;
    NieheParameters parameters;
    NieheSimulationData simulation;
};

// ============================
// 挤压工艺数据结构 (Extrusion)
// ============================
struct ExtrusionExplosiveData {
    QString density = "1.6800000E-09";
    QString specificHeat = "1.2290000E+03";
    QString conductivity = "2.3260000E-01";
    QString initialTemp = "3.0000000E+02";
    QString powerLawFac = "2.3600000E+02";
    QString powerLawTnat = "1.0000000E+00";
    QString powerLawExpo = "1.0000000E-03";
};

struct ExtrusionScrewData {
    QString solidRotationPointX = "-1.0437400E+02";
    QString solidRotationPointY = "1.9626200E+01";
    QString solidRotationPointZ = "3.5600000E+02";
    QString solidRotationAxisX = "0.0000000E+00";
    QString solidRotationAxisY = "0.0000000E+00";
    QString solidRotationAxisZ = "1.0000000E+00";
    QString solidBladeSpeed = "3.0000000E+01";
    QString solidBladeDensity = "8.0300000E-09";
    QString solidBladeThermalConductivity = "1.4500000E+01";
    QString solidBladeHeatTransfer = "5.0200000E+02";
};

struct ExtrusionBoundaryData {
    QString wallUpTemp = "3.0000000E+02";
    QString wallTemp = "3.3300000E+02";
    QString wallDownTemp = "3.3300000E+02";
    QString rotationPoint1X = "0.0000000E+00";
    QString rotationPoint1Y = "0.0000000E+00";
    QString rotationPoint1Z = "1.0000000E+00";
};

struct ExtrusionSimulationData {
    QString maxTime = "5.0000000E+00";
    QString initTimeStep = "1.0000000E+00";
    QString minTimeStep = "1.0000000E-03";
    QString maxTimeStep = "1.0000000E+00";
    QString tolerance = "1.0000000E+04";
    QString maxSuccessSteps = "20000";
};

struct JiyaProjectData {
    ExtrusionSimulationData simulation;
    ExtrusionExplosiveData explosive;
    ExtrusionBoundaryData boundary;
    ExtrusionScrewData screw;
};

// ============================
// ProjectManager 类
// ============================
class ProjectManager
{
private:
    static bool writePolyflowFile(const QString &targetPath, const QString &templatePath, const QMap<QString, QString> &dict);
    static int findBlockIndexByComment(const QString &content, const QString &blockType, const QString &keyword);
    static bool prepareTemplate(const QString &targetPath, const QString &templatePath);

public:
    static bool saveProject(const QString &folderPath, const ProjectConfig &config);
    static bool loadProject(const QString &folderPath, ProjectConfig &config);
    static bool copyDir(const QString &sourcePath, const QString &destPath, bool coverFileIfExist);

    // 模板生成
    static bool generateNiehePolyflowFile(const QString &projectPath, const NieheProjectData &data);
    static bool generateJiyaPolyflowFile(const QString &projectPath, const JiyaProjectData &data);
    static bool createTemplateFromRaw(const QString &rawPath, const QString &templatePath);

    // 捏合数据存取
    static bool saveNieheExplosive(const QString &folderPath, const NieheExplosiveData &data);
    static bool loadNieheExplosive(const QString &folderPath, NieheExplosiveData &data);
    static NieheExplosiveData getDefaultNieheExplosive();

    static bool saveNieheSimulation(const QString &folderPath, const NieheSimulationData &data);
    static bool loadNieheSimulation(const QString &folderPath, NieheSimulationData &data);
    static NieheSimulationData getDefaultNieheSimulation();

    static bool saveNieheBoundary(const QString &folderPath, const NieheBoundaryData &data);
    static bool loadNieheBoundary(const QString &folderPath, NieheBoundaryData &data);
    static NieheBoundaryData getDefaultNieheBoundary();

    static bool saveNieheParameters(const QString &folderPath, const NieheParameters &params);
    static bool loadNieheParameters(const QString &folderPath, NieheParameters &params);
    static NieheParameters getDefaultNieheParameters();

    // 挤压数据存取
    static bool saveExtrusionExplosive(const QString &folderPath, const ExtrusionExplosiveData &data);
    static bool loadExtrusionExplosive(const QString &folderPath, ExtrusionExplosiveData &data);
    static ExtrusionExplosiveData getDefaultExtrusionExplosive();

    static bool saveExtrusionSimulation(const QString &folderPath, const ExtrusionSimulationData &data);
    static bool loadExtrusionSimulation(const QString &folderPath, ExtrusionSimulationData &data);
    static ExtrusionSimulationData getDefaultExtrusionSimulation();

    static bool saveExtrusionBoundary(const QString &folderPath, const ExtrusionBoundaryData &data);
    static bool loadExtrusionBoundary(const QString &folderPath, ExtrusionBoundaryData &data);
    static ExtrusionBoundaryData getDefaultExtrusionBoundary();

    static bool saveExtrusionScrew(const QString &folderPath, const ExtrusionScrewData &data);
    static bool loadExtrusionScrew(const QString &folderPath, ExtrusionScrewData &data);
    static ExtrusionScrewData getDefaultExtrusionScrew();
};

#endif // PROJECTMANAGER_H
