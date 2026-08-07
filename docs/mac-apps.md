# Mac 68k Applications

本项目使用 [Retro68](https://github.com/autc04/Retro68) 交叉编译工具链，
在 Linux/macOS/WSL2 上构建经典 Mac OS 68k 应用程序。

## 前置条件

- Docker Engine（已安装配置）
- Retro68 Docker 镜像

```bash
docker pull ghcr.io/autc04/retro68:latest
```

## 目录结构

```
mac-app/
├── Makefile                 # 编译入口
└── CounterApp/              # 示例应用：自加计数器
    ├── CMakeLists.txt       # CMake 构建配置（Retro68 toolchain）
    ├── main.c               # C 源码
    └── Resources.r          # Rez 资源文件（菜单、对话框等）
```

## 编译

### 编译所有应用

```bash
# 从项目根目录
make mac-all

# 或从 mac-app 目录
cd mac-app && make
```

### 编译指定应用

```bash
make mac-CounterApp
```

### 清理构建产物

```bash
make mac-clean
```

## 构建产物

每个应用编译后在 `mac-app/<AppName>/build/` 下生成：

| 文件 | 格式 | 用途 |
|---|---|---|
| `<App>.bin` | MacBinary | 拖入模拟器直接运行 |
| `<App>.dsk` | 800K HFS 磁盘镜像 | 空盘，需先拷入应用 |
| `<App>.APPL` | 占位文件 | Finder 标识 |
| `<App>.code.bin` | 纯代码段 | 调试用 |
| `<App>.code.bin.gdb` | ELF | GDB 调试符号 |

## 添加新应用

1. 在 `mac-app/` 下创建 `<NewApp>/` 目录
2. 创建 `CMakeLists.txt`（参考 CounterApp），至少包含：
   ```cmake
   add_application(<AppName>
       main.c
       Resources.r
       TYPE "APPL"
       CREATOR "XXXX"
   )
   ```
3. 编写 `main.c` 和 `Resources.r`
4. 编译：`make mac-<AppName>`

## 技术要点

- 使用经典 Macintosh Toolbox API（QuickDraw、Window Manager、Control Manager）
- 资源文件（`.r`）用 Rez 编译，定义菜单、对话框、图标等
- 编译环境运行在 Docker 容器内，宿主只需 Docker Engine
