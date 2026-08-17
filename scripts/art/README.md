# vterm ASCII-art gallery (16colo.rs packs)

Local collection for the vterm's `--art` gallery mode
(`scripts/telnet_srv.js`).

## 目录结构

```
scripts/art/
  README.md
  packs/              ← 16colo.rs 源包（.zip），只存压缩包
    acid_a-d.zip
```

## 使用

服务器**直接加载 .zip**（内存解压，Node 内置 zlib，无外部依赖），不用手动解压：

```bash
# 指向 packs 目录（自动加载里面所有 .zip）
node scripts/telnet_srv.js 2324 --art scripts/art/packs

# 或指定单个 zip
node scripts/telnet_srv.js 2324 --art scripts/art/packs/acid_a-d.zip

# 多个 pack 合并成一个画廊
node scripts/telnet_srv.js 2324 --art scripts/art/packs --art /tmp/其他.zip
```

连接后自动显示第一张，回车切换下一张（循环）。

## 添加新 pack

```bash
curl -o scripts/art/packs/<pack>.zip https://16colo.rs/archive/<年份>/<pack>.zip
```

## 说明

- 服务器启动日志打印每个作品的尺寸：`loaded xxx.ANS (6877 B, 76x92)`；
  >80 列自动跳过（会折行），>30 行提示滚屏（可用滚轮回看）。
- 老 artpack 常有成人内容，下载时注意文件名/tag。
- 本目录内容来自 [16colo.rs](https://16colo.rs/) 公开存档，版权归原作者，
  **不入库**（见根目录 `.gitignore`）。
