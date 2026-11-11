# Changelog

本项目（`uu-remote-linux-bundle`）的所有变更记录都写在这里，
每个版本同时对应一个 GitHub Release。

## 0.2.57-3 — 源码编译 + 两个端合并成一个入口

- **改为源码编译**：PKGBUILD 直接拉上游 `v0.2.57` 源码包本地 `cargo build`，
  不再打包上游预编译二进制。运行库（GTK3 / ffmpeg / SDL2 等）仍然全部打进
  `/opt/uu-remote-linux`，目标机器只需要 `glibc`。
- **被控端与控制端合并成一个界面**：打了一个补丁
  （`uu-merged-entry.patch`），在主机窗口的「本设备」卡片底部加了
  **「打开控制端」** 按钮，点一下就在同一个界面里切到控制端；
  托盘菜单里也加了同名入口。应用菜单里从此只有**一个**「UU 远程 Linux」。
- **UI 不做改动**：沿用上游自己的 GTK 界面（跟随系统明暗主题、Noto Sans CJK 字体、
  卡片式布局），本仓库不再自带自绘界面。
- 构建修复：makepkg 默认给 `CFLAGS/LDFLAGS` 加 `-flto=auto`，
  会让 `cc` 编出的 `.o` 变成 LTO 字节码，归档后链接器认不出符号
  （`undefined symbol: uu_viewer_ui_*`、ring 的 `GFp_*`），构建脚本里已去掉该选项。

## 0.2.57-2 — 统一入口（自绘界面）

- 应用菜单里被控端与控制端合并为一个入口，用一个自绘的 GTK 界面在
  「我的设备 / 远程协助 / 开始协助 / 扫码登录」之间切换。
- PKGBUILD 改为从上游 release 源包构建，不再依赖 AUR 的 `uu-remote-linux-bin`。
- 修掉 `uu-virtual-desktop.service` 指向不存在的 `/usr/bin/uu-virtual-desktop` 的问题。
- 补 `lib/x86_64-linux-gnu` 软链，`UU_FFMPEG_ROOT` 模式下打包的 ffmpeg 才能正常启动。

## 0.2.57 — 首个自包含包

- 把上游 `uu-remote-linux` 的二进制 + 运行库（GTK3、ffmpeg、SDL2 等）打包成
  单一 Arch 包，目标机器只需要 `glibc`。
