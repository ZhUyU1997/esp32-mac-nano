# ROM 与系统镜像管理

Macintosh Plus ROM 版权归 Apple 所有，**不随本仓库分发**。本文档说明前置工具安装、ROM / 系统镜像的获取与构建。

## 1. 前置条件

工具按使用路径区分（均不入库，需自行安装）：

| 路径 | 工具 | 用途 | 安装 |
|------|------|------|------|
| A | —（无需命令行工具） | Snow 模拟器 GUI 操作 | — |
| B | [djjr (Disk Jockey Jr)](https://diskjockey.onegeekarmy.eu/djjr/) | `.dsk` → `.hda` 转换 | 官网下载 Linux 版（静态链接 Swift，无外部依赖），放置到 `tools/djjr` |
| B | hfsutils | 裁剪脚本读写 HFS 卷（`hformat`、`hls`、`hcopy`） | Debian/Ubuntu：`apt install hfsutils` |
| B | python3 | 裁剪/验证脚本 | 系统自带 |

## 2. ROM（编译必需）

`macintosh/rom.bin`（Mac Plus v3，`4D1F8172`，128 KB）会被编译进固件，缺失时 `idf.py build` 无法完成。获取方式：

- 从你合法持有的 Macintosh Plus 中提取（Mini vMac 的 [CopyRoms](https://www.gryphel.com/c/minivmac/extras/copyroms/index.html) 工具）
- 或从 [archive.org（mac_rom_archive 集合）](https://archive.org/download/mac_rom_archive_-_as_of_8-19-2011/mac_rom_archive_-_as_of_8-19-2011.zip/4D1F8172%20-%20MacPlus%20v3.ROM) 获取（版权归 Apple，请自行确认你所在司法管辖区是否允许）

下载后命名为 `macintosh/rom.bin` 即可。

## 3. 基础系统镜像（路径 A：Snow 模拟器安装）

从 [WinWorldPC](https://winworldpc.com/product/mac-os-0-6/system-3x) 获取 Mac OS **安装软盘**，在 [Snow](https://snowemu.com) 模拟器中引导软盘、安装系统到硬盘镜像：

1. 获取安装软盘（WinWorldPC）
2. Snow 中引导软盘，初始化硬盘并安装 System
3. 导出为 `macintosh/hd_v0.img`（纯净系统）
4. 安装 DashApp、CounterApp 等应用后导出为 `macintosh/hd_v1.img`

> `make install`（hd_v1.img → hd.img）属部署阶段，在 Linux 上执行，需 hfsutils，见第 5 节。

## 4. 数据盘（路径 B：djjr 转换）

现场生成的 `.dsk` 或 [Infinite Mac](https://infinitemac.org) 保存的 `.dsk`，用 **djjr** 转换为设备镜像：

```bash
./tools/djjr convert to-device <input.dsk> <output.hda>
```
> 注：仅支持 HFS 卷，MFS 卷不支持。

裁剪为数据盘见附录。

## 附录：数据盘裁剪脚本（可选）

`macintosh/disk/hd0.img` 和 `macintosh/disk/hd1.img` 由以下脚本从 `.hda` 裁剪（一般用户不需要）：

| 脚本 | 源 | 输出 | 说明 |
|------|----|------|------|
| `scripts/build-saved-hd-v2.sh` | `macintosh/disk/Saved HD.hda` | `macintosh/disk/hd0.img` | 裁剪为 Mac Plus 4 MB 兼容 (171 MB) |
| `scripts/build-hd1-kiosk.sh` | `macintosh/disk/HD30_512 - Mac Plus Kiosk.hda` | `macintosh/disk/hd1.img` | 去除 System 和 Desktop 缓存 (21 MB) |

```bash
bash scripts/build-saved-hd-v2.sh -f    # 强制重建 hd0.img
bash scripts/build-hd1-kiosk.sh -f      # 强制重建 hd1.img
```

**build-saved-hd-v2.sh**：从 Infinite Mac 导出的 1.1 GB Saved HD.hda 中，筛选出能在 Mac Plus 4 MB 上运行的软件（移除 PPC/68020 独占游戏、低版本旧文件；保留 35 个经典 68k 游戏及 Productivity/Developer/Graphics 工具），从 1.1 GB 裁剪为 171 MB。
> 注意：保留的软件版权归各自公司，请确保你合法持有原始镜像后再构建。

**build-hd1-kiosk.sh**：从 Kiosk 演示盘中去除 System 文件夹和 Desktop 缓存，保留 Applications、Games、HyperCard。

**验证脚本**：`scripts/verify-saved-hd-v2.py` 对比原始 Saved HD.hda 和 hd0.img，检查是否有文件遗漏：

```bash
python3 scripts/verify-saved-hd-v2.py
```

输出示例：
```
✓ ALL 2305 files present in v2. No omissions!
```

## 5. 镜像文件一览

| 文件 | 类别 | 说明 | 纳入 git |
|------|------|------|----------|
| `macintosh/rom.bin` | 编译依赖 | Mac Plus v3 ROM (128 KB)，嵌入固件 | 否（用户自备） |
| `macintosh/hd_v0.img` | 运行依赖 | 纯净系统镜像（路径 A） | 否（用户自备） |
| `macintosh/hd_v1.img` | 运行依赖 | 基础系统镜像，`make install` 复制为 `hd.img`（路径 A） | 否（用户自备） |
| `macintosh/hd.img` | 运行产物 | 最终运行镜像，由 `make install` 生成 | 否 |
| `macintosh/disk/hd0.img` | 构建产物 | 从 Saved HD.hda 裁剪 (171 MB)（路径 B） | 否 |
| `macintosh/disk/hd1.img` | 构建产物 | 从 Kiosk HDA 裁剪 (21 MB)（路径 B） | 否 |

```bash
make install              # 从 hd_v1.img 复制到 hd.img（依赖 hd_v1.img 已存在，需 hfsutils）
```

## 6. 开发者工作流

- MAME 制作系统盘、HFS 脚本用法见 [image_workflows.md](image_workflows.md)。

## 参考

- [pico-mac](https://github.com/evansm7/pico-mac)、[minimacplus](https://github.com/spritetm/minimacplus) — 同类项目，均不随源码分发 ROM，仅提供生成/烧录流程。
