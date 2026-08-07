# 地狱之下 (UnderHell)

一个架空世界构建项目,包含文档、模板、地图生成器和图片资源。

## 项目结构

本仓库使用 Git Submodule 管理各组件:

```
UnderHell/
├── 文档/          → UnderHellDocs       — Typst 文档源码(地狱之下正文)
├── 模板/          → UnderHellTemplate   — 架空世界 Typst 模板(已发布至 Typst Universe)
├── 图片/          → UnderHellImages     — 地图、插图等图片资源
├── 程序/          → UnderHellCodes      — 地图生成器(CLI + GUI)
└── .github/       — CI/CD 自动编译发布
```

## 文档编译

文档使用 [Typst](https://typst.app) 编译,支持三种输出版本:

| 版本 | 命令 | 说明 |
|---|---|---|
| 普通版 | `make all` | A4 双栏、彩色背景图、深红标题 |
| 打印版 | `make print` | A4 双栏、无背景图、纯黑标题、宽边距(省墨) |
| 小屏版 | `make screen` | A5 单栏、保留背景、窄边距、小字号(手机/平板阅读) |

一次性编译全部三种版本:

```bash
cd 文档 && make all print screen
```

也可直接调用 typst:

```bash
typst compile --root .. --font-path fonts 地狱之下.typ 输出.pdf
typst compile --root .. --font-path fonts --input print=true 地狱之下.typ 输出.pdf
typst compile --root .. --font-path fonts --input screen=true 地狱之下.typ 输出.pdf
```

或使用包装脚本:

```bash
./typst-print --print 地狱之下.typ 输出.pdf
./typst-print --screen 地狱之下.typ 输出.pdf
```

## 地图生成器

基于断层生长算法的程序化地图生成工具,支持 PNG/SVG 双格式输出。

- **CLI**:命令行版本,支持海岸线、分层设色、经纬网格、切片等功能
- **GUI**:Qt 图形界面,可视化参数调节与实时预览

```bash
cd 程序/地图生成器
cmake -B build-cli -DCMAKE_BUILD_TYPE=Release
cmake --build build-cli
./build-cli/地图生成器 42 0 60 512 256 1 output.png
./build-cli/地图生成器 42 0 60 512 256 1 output.svg --svg
```

详细用法参见 [程序/README.md](程序/README.md)。

## 模板

架空世界 Typst 模板,已发布至 Typst Universe(`@preview/underhell`)。

提供封面、分栏排版、属性方块(statbox)、NPC 卡片(npcbox)、法术卡片(spell)、跨页图片、附录编号等架空世界常用排版功能。

详细用法参见 [模板/README.md](模板/README.md)。

## CI/CD

GitHub Actions 会在推送 main 分支或打 tag 时自动编译并发布:

- 三种 PDF 版本(普通版、打印版、小屏版)
- 地图生成器 CLI(Windows / macOS / Linux)
- 地图生成器 GUI(Windows / macOS / Linux)

## 快速开始

```bash
# 克隆(含子模块)
git clone --recursive https://github.com/CrossDark/UnderHell.git

# 编译文档
cd UnderHell/文档
make all print screen

# 编译地图生成器
cd ../程序/地图生成器
cmake -B build-cli -DCMAKE_BUILD_TYPE=Release
cmake --build build-cli
```
