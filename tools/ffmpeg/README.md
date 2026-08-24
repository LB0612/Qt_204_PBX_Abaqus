# FFmpeg 发布组件

将官方 FFmpeg 静态构建中的以下文件放入本目录后再打包发布：

- `ffmpeg.exe`
- `ffprobe.exe`

程序启动 t2 后处理前会检查这两个文件，并执行 `-version` 验证。

路径约定（相对可执行文件）：

```
<程序目录>/tools/ffmpeg/ffmpeg.exe
<程序目录>/tools/ffmpeg/ffprobe.exe
```

不要把 FFmpeg 放入 Qt `.qrc`；t2.py 通过环境变量 `PBX_FFMPEG_EXE` / `PBX_FFPROBE_EXE` 获取路径。
