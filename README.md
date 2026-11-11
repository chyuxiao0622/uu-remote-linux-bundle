# uu-remote-linux-bundle

网易 **UU 远程** Linux 被控端 + 控制端（非官方）的**自包含 Arch 包**。

把 `uu-remote-linux` 需要的 GTK3、ffmpeg、SDL2 等运行库全部打包进 `/opt/uu-remote-linux`，
目标机器只需要 `glibc`，**不用再装任何依赖**。

> 上游项目：<https://github.com/VallierDev/uu-remote-linux>（GPL-3.0）
> 本仓库只是把它的二进制重新打包成离线可装的 Arch 包，与网易官方无关。

## 安装

从 [Releases](../../releases) 下载 `uu-remote-linux-bundle-0.2.57-2-x86_64.pkg.tar.zst`，然后：

```bash
# 校验（可选但推荐）
sha256sum -c <<< "1ea3cb41f003929793873a70a03410362464b1196f8da67e86a969a97fc2a97f  uu-remote-linux-bundle-0.2.57-2-x86_64.pkg.tar.zst"

# 如果之前装过上游的 uu-remote-linux-bin，先卸掉（本包与它 conflicts）
sudo pacman -R uu-remote-linux-bin

sudo pacman -U uu-remote-linux-bundle-0.2.57-2-x86_64.pkg.tar.zst
```

本包 `provides` 了 `uu-remote` / `uu-remote-linux-bin`，并与它们 `conflicts`，
所以装上之后 `pacman -Qs uu-remote` 只显示 `uu-remote-linux-bundle`。

### 局域网分发

包是自包含的，可以直接丢到内网共享里给别的机器装，比如用 Samba 共享一个目录：

```bash
# 机器 A（共享方）
sudo pacman -S samba
sudo cp uu-remote-linux-bundle-0.2.57-2-x86_64.pkg.tar.zst /共享目录/
sudo systemctl enable --now smb.service

# 机器 B（安装方）
smbclient //机器A/共享名 -U 用户名 -c 'get uu-remote-linux-bundle-0.2.57-2-x86_64.pkg.tar.zst'
sudo pacman -U uu-remote-linux-bundle-0.2.57-2-x86_64.pkg.tar.zst
```

## 包里有什么

| 内容 | 路径 |
| --- | --- |
| 统一启动器（被控端 / 控制端） | `/usr/bin/uu-remote` → `/opt/uu-remote-linux/bin/uu-remote` |
| 统一界面（仿官方客户端） | `/opt/uu-remote-linux/bin/uu-remote-ui` |
| 被控端 `uuremote-server` | `/usr/bin/uuremote-server` → `/opt/uu-remote-linux/bin/` |
| 控制端 `uuremote-viewer` | `/usr/bin/uuremote-viewer` → `/opt/uu-remote-linux/bin/` |
| 打包的 ffmpeg | `/opt/uu-remote-linux/bin/ffmpeg` |
| GTK3 / glib / ffmpeg 等运行库 | `/opt/uu-remote-linux/lib/` |
| GTK 输入法模块、glib schemas | `/opt/uu-remote-linux/lib/gtk-3.0/`、`/opt/uu-remote-linux/share/` |
| systemd 用户服务 | `uuremote-native.service`、`uu-virtual-desktop.service` |
| udev 规则（uinput） | `60-uu-remote-linux-uinput.rules` |

启动器只是设置 `LD_LIBRARY_PATH` / `PATH` / `GSETTINGS_SCHEMA_DIR` 等环境变量，
再执行 `/opt/uu-remote-linux/bin/` 里的真实程序，不影响系统库。

## 用法

应用菜单里**只有一个**「UU 远程 Linux」入口（被控端和控制端合并在一起）：

- 点开后是一个仿官方 UU 远程客户端的窗口：左侧导航「我的设备 / 远程协助 / 开始协助 / 扫码登录」，
  右侧显示本机设备 ID、账号、是否允许他人远程协助
- 「打开主机窗口」= 打开被控端（显示设备 ID 和验证码，开关「允许他人远程协助」）
- 「打开控制端」= 打开控制端（设备列表、远程协助、开始协助）

命令行同样可用：

```bash
uu-remote              # 打开统一界面
uu-remote host         # 直接打开被控端窗口
uu-remote viewer       # 直接打开控制端窗口
uu-remote login        # 扫码登录
```

### 别人控制这台机器（被控端）

```bash
# 开机/登录后自动常驻
systemctl --user enable --now uuremote-native.service
```

然后打开统一界面里的「打开主机窗口」：

- 窗口里会显示 **本设备 ID** 和 **验证码**
- 把这两个发给对方，对方在 UU 远程「开始协助」里输入即可
- 需要先打开右上角 **「允许他人远程协助」** 开关（默认关闭）
- 用完可以关掉开关或刷新验证码

Wayland（KDE/GNOME）下首次连接会弹出屏幕共享授权框，勾选「记住」后下次不用再确认。

### 扫码登录（让这台机器出现在官方 App 里）

```bash
uuremote-server login          # 图形界面扫码
uuremote-server login --no-gui # 终端里扫码，适合 SSH / 无桌面
```

未登录时服务也能以**访客身份**提供远程协助（设备 ID + 验证码方式），
但这台机器不会出现在你 UU 账号的设备列表里。登录信息保存在
`~/.config/uuremote/config.json`（权限 0600）。

> ⚠️ 不要在机器之间拷贝 `config.json`。同一份设备身份（`device_id` / `client_id` /
> `system_id`）跑两个实例会互相把对方踢出房间，表现为「Cloud room leave without peer
> identifier」反复重连、控制端一连就断。克隆出来的机器请先删掉这几个键再重新登录。

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
| 键鼠不生效 | 确认 `/dev/uinput` 可写（`uuremote-server doctor` 会显示），必要时 `sudo modprobe uinput` |

## 卸载

```bash
systemctl --user disable --now uuremote-native.service
sudo pacman -R uu-remote-linux-bundle
```

配置和设备身份留在 `~/.config/uuremote/`，需要重置时手动删掉该目录。

## 构建

本包自带上游的 release 源包，**不需要先装任何 AUR 包**，直接：

```bash
git clone https://github.com/chyuxiao0622/uu-remote-linux-bundle.git
cd uu-remote-linux-bundle
makepkg -si
```

构建机上需要装这些库（`makepkg` 会自动用 `makedepends` 检查）：
`gtk3`、`ffmpeg`、`cairo`、`pango`、`gdk-pixbuf2`、`glib2`、`wayland`、
`libx11`、`libxext`、`libxfixes`、`libxkbcommon`、`libxtst`、`sdl2`、`gcc`。

`uu-remote-ui.c` 是本仓库自己写的统一界面（仿官方客户端），用 GTK3 编译，
运行时不额外依赖——GTK 库已经打进包里了。

## 许可

- 上游 `uu-remote-linux`：GPL-3.0-or-later，源码见 <https://github.com/VallierDev/uu-remote-linux>
- 本仓库的打包脚本与 `uu-remote-ui.c`：同上
- 包内附带的第三方运行库（GTK、ffmpeg 等）遵循各自的许可证
