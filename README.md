# uu-remote-linux-bundle

网易 **UU 远程** Linux 被控端 + 控制端（非官方）的**自包含 Arch 包**。

把 `uu-remote-linux` 需要的 GTK3、ffmpeg、SDL2 等运行库全部打包进 `/opt/uu-remote-linux`，
目标机器只需要 `glibc`，**不用再装任何依赖**。

> 上游项目：<https://github.com/VallierDev/uu-remote-linux>（GPL-3.0）
> 本仓库只是把它的二进制重新打包成离线可装的 Arch 包，与网易官方无关。

## 安装

从 [Releases](../../releases) 下载 `uu-remote-linux-bundle-0.2.57-1-x86_64.pkg.tar.zst`，然后：

```bash
sudo pacman -U uu-remote-linux-bundle-0.2.57-1-x86_64.pkg.tar.zst
```

包会替换同名的 `uu-remote-linux-bin`（provides/conflicts 已声明）。

## 包里有什么

| 内容 | 路径 |
| --- | --- |
| 控制端 `uuremote-viewer` | `/usr/bin/uuremote-viewer` → `/opt/uu-remote-linux/bin/` |
| 被控端 `uuremote-server` | `/usr/bin/uuremote-server` → `/opt/uu-remote-linux/bin/` |
| 打包的 ffmpeg | `/opt/uu-remote-linux/bin/ffmpeg` |
| GTK3 / glib / ffmpeg 等运行库 | `/opt/uu-remote-linux/lib/` |
| GTK 输入法模块、glib schemas | `/opt/uu-remote-linux/lib/gtk-3.0/`、`/opt/uu-remote-linux/share/` |
| systemd 用户服务 | `uuremote-native.service` |
| udev 规则（uinput） | `60-uu-remote-linux-uinput.rules` |

启动器只是设置 `LD_LIBRARY_PATH` / `PATH` / `GSETTINGS_SCHEMA_DIR` 等环境变量，
再执行 `/opt/uu-remote-linux/bin/` 里的真实程序，不影响系统库。

## 用法

### 控制别人的电脑

应用菜单打开 **「UU 远程 Linux 控制端」**，用手机 UU 远程 App 扫码登录，
然后在「远程协助」里选设备，或在「开始协助」输入对方给的设备 ID + 验证码。

### 别人控制这台机器

```bash
systemctl --user enable --now uuremote-native.service
```

然后打开 **「UU 远程 Linux 主机」**（或 `uuremote-server show`）：

- 窗口里会显示 **本设备 ID** 和 **验证码**
- 把这两个发给对方，对方在 UU 远程「开始协助」里输入即可
- 需要先打开右上角 **「允许他人远程协助」** 开关（默认关闭）
- 用完可以关掉开关或刷新验证码

Wayland（KDE/GNOME）下首次连接会弹出屏幕共享授权框，勾选「记住」后下次不用再确认。

## 构建

需要先安装上游的 AUR 包（用来提供原始二进制和插件）：

```bash
yay -S uu-remote-linux-bin
makepkg -si
```

## 许可

- 上游 `uu-remote-linux`：GPL-3.0-or-later，源码见 <https://github.com/VallierDev/uu-remote-linux>
- 本仓库的打包脚本：同上
- 包内附带的第三方运行库（GTK、ffmpeg 等）遵循各自的许可证
