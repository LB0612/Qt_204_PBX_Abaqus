#ifndef APPINFO_H
#define APPINFO_H

#include <QString>

namespace AppInfo {

inline const QString HomeTitleLine1 =
    QStringLiteral("浇注XX固化仿真与三维参数重构");

inline const QString HomeTitleLine2 =
    QStringLiteral("分析软件");

inline const QString ProductName =
    HomeTitleLine1 + HomeTitleLine2;

inline const QString OrganizationName =
    QStringLiteral("PBXSimulationSoftware");

inline const QString ProjectTypeDisplayName =
    ProductName;

inline const QString ProjectTypeId =
    QStringLiteral("PBX_CASTING_CURING");

} // namespace AppInfo

#endif
