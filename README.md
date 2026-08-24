# Qt_204_PBX_Abaqus

PBX 浇注固化 Abaqus 仿真桌面软件（Qt 6 + CMake）。

## 构建

1. 安装 Qt 6（Core / Gui / Widgets）与 CMake 3.16+
2. 在 Qt Creator 中打开 `CMakeLists.txt`，或命令行：

```bash
cmake -S . -B build
cmake --build build
```

可执行目标：`QT_PBX_204_ABAQUS`

## 工程目录（用户数据，格式不变）

```
工程名/
├── project.json
├── config/          # 五组参数 JSON
├── abaqus/          # 生成的 t0/t1/335K、ODB/STA/MSG、flag
├── results/         # 应力/温度/固化度 PNG 与 AVI
└── logs/            # t0.log、t1.log
```

## 源码职责

| 模块 | 职责 |
|------|------|
| `MainWindow` | 主界面、工具栏、树导航、信号连接 |
| `ProjectManager` | 新建/打开/保存工程 |
| `*ConfigManager` + `*ParamWidget` | 参数 JSON 与参数页 UI |
| `AbaqusFileGenerator` | 从 QRC 模板生成 `t0.py` / `t1.py` / `335K.for` |
| `SimulationManager` | Abaqus 启动/停止、日志、进度、锁文件、完成判定 |
| `SimulationPrepareWidget` | 仿真前确认 |
| `SimulationMonitorWidget` | 状态/进度/日志显示 |

Abaqus 模板位于 `PBX_Simulate/Templates/`，通过 `SimulationTemplates.qrc` 打包进 exe。

## 仿真流程

1. 新建或打开工程，填写并保存五组参数  
2. **生成文件** → 写入 `generation_complete.flag`  
3. **开始仿真** → `SimulationManager` 依次运行 t0（建模）与 t1（求解与后处理）  
4. 监控页实时读取 `.msg` / `.sta`；终止仅作用于当前 Job  

## 依赖

- Abaqus（路径在 **系统设置** 中配置，默认 `C:/SIMULIA/Commands/abaqus.bat`）
