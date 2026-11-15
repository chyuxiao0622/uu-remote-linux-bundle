# Changelog

本项目（`uu-remote-linux-bundle`）的所有变更记录都写在这里，
每个版本同时对应一个 GitHub Release。

## 0.2.57-5 — 降低编码器缓冲延迟（实测省约 0.8 秒）

上一版把帧率和码率封住了，但延迟还有一大块没动：**编码器的 VBV 缓冲**。

上游 NVENC 的默认缓冲是「1 秒码率上限」（钳在 500–8000 kbit）。在 12 Mbit/s 上限下
就是 `-bufsize 8000k`，也就是**编码器内部压了将近 0.9 秒的码流**才吐出去 —— 这段
纯等待，直接变成端到端延迟。上游文档里其实提过 `UU_NVENC_BUFFER_MS`（10–1000 ms），
但默认没开。

这一版让包直接带上 systemd 调优 drop-in：

```ini
# /usr/lib/systemd/user/uuremote-native.service.d/performance.conf
[Service]
Environment=UU_NVENC_BUFFER_MS=50
Environment=UU_MAX_FPS=30
Environment=UU_MAX_KBIT=12000
```

实测（同一台机器，NVENC，12 Mbit/s 上限）：

| | `-bufsize` | 折算缓冲 |
| --- | --- | --- |
| 默认 | 8000k | ≈ 0.89 秒 |
| 调优后 | 600k | ≈ 0.07 秒 |

**端到端延迟直接减少约 0.8 秒**，而且不用改任何代码。

想改回上游默认：注释掉 drop-in 里的三行，或者在用户级放一份覆盖

```
~/.config/systemd/user/uuremote-native.service.d/performance.conf
```

（用户级优先级高于 /usr/lib 级的系统 drop-in。）

## 0.2.57-4 — 远控流畅度优化

实测定位到卡顿的三个主要开销（4K 采集 + 60fps + 不限码率），用第二个补丁
`uu-perf-tune.patch` 一起解决：

- **控制端默认 30 帧**（原来 60）：管道里 rgb24 原始帧的传输量、软解码开销、
  SDL 上传和渲染全部减半。远控场景 30 帧已经跟手，60 帧只是白烧 CPU。
- **宿主端帧率封顶 30 帧**：控制端（无论官方客户端还是本项目的控制端）请求
  60/90/144 帧时，宿主端按 30 帧采集编码，上限可用 `UU_MAX_FPS` 调整（0 = 不限）。
- **「自动」画质加码率上限 12 Mbit/s**：上游的自动画质是 CQ18 不限码率，
  实测动态画面能到几十 Mbit/s，带宽一抖就出延迟和丢帧。现在默认封顶，
  `UU_MAX_KBIT` 可调（单位 kbit/s，0 = 不限）。

同时更新了上游里固化旧行为的单元测试（帧率透传、自动画质不限码率），并新增
两个针对性用例。上游完整测试套件 **374 + 218 全通过**。

实测数据（本机 KDE Wayland / Portal / NVENC）：

| 指标 | 优化前 | 优化后 |
| --- | --- | --- |
| 控制端请求帧率 | 60 | 30 |
| 宿主端编码码率上限 | 30000k（不限码率） | 12000k |
| 宿主端默认帧率 | 60 | 30 |

> 采集分辨率仍是 KDE Portal 决定的 4K（3840x2160），这一步无法从应用侧压缩；
> 编码器会把画面缩放到控制端请求的 1920x1080 再编码。

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
