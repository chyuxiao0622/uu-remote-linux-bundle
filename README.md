# uu-remote-linux-bundle

网易 **UU 远程** Linux 被控端 + 控制端（非官方）的**自包含 Arch 包**。

- 上游源码**本地编译**（不再打包预编译二进制）
- 运行库（GTK3、ffmpeg、SDL2 等）全部打进 `/opt/uu-remote-linux`，目标机器只需要 `glibc`
- 应用菜单里**只有一个**「UU 远程 Linux」：被控端和控制端合并在同一个界面里

> 上游项目：<https://github.com/VallierDev/uu-remote-linux>（GPL-3.0）
> 本仓库只是把它的源码编译成 Arch 包并重新打包，与网易官方无关。

## 安装

从 [Releases](../../releases) 下载 `uu-remote-linux-bundle-0.2.57-4-x86_64.pkg.tar.zst`，然后：

```bash
# 校验（可选但推荐）
sha256sum -c <<< "028d939b71f13a7dfe9bbae13453889b8456cd7d8ef7b0a3402286d5c3f7769d  uu-remote-linux-bundle-0.2.57-4-x86_64.pkg.tar.zst"

# 如果之前装过上游的 uu-remote-linux-bin，先卸掉（本包与它 conflicts）
sudo pacman -R uu-remote-linux-bin

sudo pacman -U uu-remote-linux-bundle-0.2.57-4-x86_64.pkg.tar.zst
```

### 局域网分发

包是自包含的，可以直接丢到内网共享里给别的机器装，比如用 Samba 共享一个目录：

```bash
# 机器 A（共享方）
sudo pacman -S samba
sudo cp uu-remote-linux-bundle-0.2.57-4-x86_64.pkg.tar.zst /共享目录/
sudo systemctl enable --now smb.service

# 机器 B（安装方）
smbclient //机器A/共享名 -U 用户名 -c 'get uu-remote-linux-bundle-0.2.57-4-x86_64.pkg.tar.zst'
sudo pacman -U uu-remote-linux-bundle-0.2.57-4-x86_64.pkg.tar.zst
```

## 被控端和控制端怎么合并的

上游本来是两个程序、两个窗口：

| 程序 | 窗口 |
| --- | --- |
| `uuremote-server` | 被控端（主机）：设备 ID、验证码、允许他人远程协助开关、扫码登录 |
| `uuremote-viewer` | 控制端：设备列表、远程协助、开始协助、收藏设备 |

本仓库打了一个补丁 [`uu-merged-entry.patch`](uu-merged-entry.patch)，在**被控端窗口**的
「本设备」卡片底部加了一个 **「打开控制端」** 按钮，点一下就在同一个界面里切到控制端；
托盘菜单里也有同名入口。应用菜单从此只有一个「UU 远程 Linux」（`Exec=uuremote-server show`）。

界面本身完全沿用上游自己的 GTK 实现（跟随系统明暗主题、Noto Sans CJK 字体、卡片式布局），
本仓库不再自带自绘界面。


## 远控流畅度

本包默认按「流畅优先」调过（源码级补丁 `uu-perf-tune.patch`）：

| 项 | 默认 | 环境变量 |
| --- | --- | --- |
| 控制端请求帧率 | 30 fps | — |
| 宿主端采集帧率上限 | 30 fps | `UU_MAX_FPS`（0 = 不限） |
| 自动画质码率上限 | 12 Mbit/s | `UU_MAX_KBIT`（0 = 不限） |

想改回上游默认行为，在 systemd 用户服务里加环境变量即可：

```ini
[Service]
Environment=UU_MAX_FPS=60
Environment=UU_MAX_KBIT=0
```

其它可能有用的开关：

| 变量 | 作用 |
| --- | --- |
| `UU_VIDEO_ENCODER=nvenc` | 强制 NVENC（本机默认自动探测到 NVENC） |
| `UU_VIDEO_CODEC=h264` | 强制 H.264（软件解码端更省 CPU，代价是同画质码率更大） |
| `UU_PORTAL_DMABUF=0` | 关掉 Portal 的 DMA-BUF，改用进程内上传 |
| `UU_X11_MULTISCREEN=1` | X11 下把多显示器拼成一张画面 |

排查性能：`uuremote-server desktop-video-probe /tmp/probe.h264` 会打印实际使用的
编码器、分辨率、帧率和码率上限（`UU_PROBE_FPS` / `UU_PROBE_FRAMES` 可调）。

## 包里有什么

| 内容 | 路径 |
| --- | --- |
| 被控端 `uuremote-server` | `/usr/bin/uuremote-server` → `/opt/uu-remote-linux/bin/` |
| 控制端 `uuremote-viewer` | `/usr/bin/uuremote-viewer` → `/opt/uu-remote-linux/bin/` |
| 虚拟桌面脚本 | `/usr/bin/uu-virtual-desktop` → `/opt/uu-remote-linux/bin/` |
| 打包的 ffmpeg | `/opt/uu-remote-linux/bin/ffmpeg` |
| GTK3 / glib / ffmpeg 等运行库 | `/opt/uu-remote-linux/lib/` |
| 采集/编码插件 | `/opt/uu-remote-linux/lib/uu-remote-linux/libuu-{portal,wayland}.so` |
| GTK 输入法模块、glib schemas | `/opt/uu-remote-linux/lib/gtk-3.0/`、`/opt/uu-remote-linux/share/` |
| systemd 用户服务 | `uuremote-native.service`、`uu-virtual-desktop.service` |
| udev 规则（uinput） | `60-uu-remote-linux-uinput.rules` |

启动器只是设置 `LD_LIBRARY_PATH` / `PATH` / `GSETTINGS_SCHEMA_DIR` 等环境变量，
再执行 `/opt/uu-remote-linux/bin/` 里的真实程序，不影响系统库。

## 用法

```bash
# 首次使用：扫码登录
uuremote-server login

# 常驻（开机/登录后自动起）
systemctl --user enable --now uuremote-native.service

# 打开合并后的界面
uuremote-server show
```

界面里：

- **本设备**：本设备 ID、临时验证码、「允许他人远程协助」开关、「复制并分享」
- **打开控制端**：切到控制端，去看设备列表 / 发起远程协助
- **账号**：扫码登录 / 退出登录

Wayland（KDE/GNOME）下首次连接会弹出屏幕共享授权框，勾选「记住」后下次不用再确认。

## 排查

```bash
uuremote-server doctor
```

会输出当前会话类型、后端（portal / wayland / x11）、编码器（nvenc / cpu）、
输入设备、日志级别、是否已登录等，**不包含 ID、token、地址和文件内容**，可以放心贴出来。

常见问题：

| 现象 | 处理 |
| --- | --- |
| doctor 里 `signed in: false` | 还没扫码登录，跑 `uuremote-server login` |
| 提示多个主机实例 | 停掉手动启动的旧实例（`pkill -f uuremote-server` 后只留 systemd 那个） |
| Wayland 下共享后黑屏 | 改用 x11 后端或虚拟桌面，见上游 README「无头 / 虚拟桌面」 |
| 键鼠不生效 | 确认 `/dev/uinput` 可写（doctor 会显示），必要时 `sudo modprobe uinput` |

## 构建

```bash
git clone https://github.com/chyuxiao0622/uu-remote-linux-bundle.git
cd uu-remote-linux-bundle
makepkg -si
```

构建机上需要：`cargo`、`rust`、`pkg-config`、`gtk3`、`ffmpeg`、`sdl2-compat`、
`wayland`、`libx11`、`libxext`、`libxtst`、`libxfixes`、`libxkbcommon`、`glib2`、
`libpipewire`、`libei`、`cairo`、`pango`、`gdk-pixbuf2`。

构建过程：

1. 拉上游 `v0.2.57` 源码包
2. 打上 `uu-merged-entry.patch`（合并被控端与控制端的入口）
3. `cargo build --release --locked --features wayland,portal,engine-plugins,native-viewer,viewer-sdl`
4. 把二进制 + 运行库（`ldd` 收集）打进 `/opt/uu-remote-linux`

> 构建脚本里去掉了 `-flto=auto`：makepkg 默认加上它，会让 `cc` 编出的 `.o` 变成 LTO 字节码，
> 归档成 `.a` 后链接器认不出符号。

## 更新日志

见 [CHANGELOG.md](CHANGELOG.md)，每个版本都有对应的 GitHub Release。

## 卸载

```bash
systemctl --user disable --now uuremote-native.service
sudo pacman -R uu-remote-linux-bundle
```

配置和设备身份留在 `~/.config/uuremote/`，需要重置时手动删掉该目录。

## 许可

- 上游 `uu-remote-linux`：GPL-3.0-or-later，源码见 <https://github.com/VallierDev/uu-remote-linux>
- 本仓库的打包脚本与补丁：同上
- 包内附带的第三方运行库（GTK、ffmpeg 等）遵循各自的许可证
