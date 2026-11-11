# Maintainer: chyuxiao0622 <https://github.com/chyuxiao0622/uu-remote-linux-bundle>
# 自包含包：把上游二进制 + 运行库（GTK3 / ffmpeg / SDL2 ...）全部打进 /opt，
# 目标机器只需要 glibc。构建时直接用上游 release 源包，不依赖任何 AUR 包。
pkgname=uu-remote-linux-bundle
pkgver=0.2.57
pkgrel=2
pkgdesc='Unofficial Linux host and viewer for NetEase UU Remote (self-contained: all runtime deps bundled)'
arch=('x86_64')
url='https://github.com/VallierDev/uu-remote-linux'
license=('GPL-3.0-or-later')
depends=('glibc')
provides=('uu-remote' 'uu-remote-linux-bin')
conflicts=('uu-remote' 'uu-remote-linux-bin' 'uu-remote-linux')
options=('!debug' '!emptydirs')
install=uu-remote-linux-bundle.install
_pfx=/opt/uu-remote-linux

# 只在构建机上需要：运行库就是从这些包里收集的
makedepends=(
  'cairo'
  'ffmpeg'
  'gcc'
  'gdk-pixbuf2'
  'glib2'
  'gtk3'
  'libx11'
  'libxext'
  'libxfixes'
  'libxkbcommon'
  'libxtst'
  'pango'
  'sdl2'
  'wayland'
)

source=(
  'uuremote-native.service'
  '60-uu-remote-linux-uinput.rules'
  'uu-remote'
  'uu-remote-linux.desktop'
  'uu-remote-ui.c'
)
source_x86_64=("uu-remote-linux-v${pkgver}-x86_64-linux.tar.gz::https://github.com/VallierDev/uu-remote-linux/releases/download/v${pkgver}/uu-remote-linux-v${pkgver}-x86_64-linux.tar.gz")
sha256sums=(
  '1bba2b6ada40e980c23c4a78188d694e8c8bc39c3d39e9ffb8a41947501da48e'
  'df21d14e39e6912261e9ecdc2422d242612053beb2cbc3a8755938020abf3e6f'
  '745a0d5aef00725b0acafc8fae7c7b333bdb06f32455ff34ebbf57fe0372e2fe'
  '9cfa32380ec4bed148c4d5ddd03c05ddc3085729bcdc4bec89e39ed3d2b01382'
  'af0c2f2846a471edb7586ff2e1dcd7828b58569e970d82f1ba5ed7a9b2211e58'
)
sha256sums_x86_64=('f8893f63afafd239fcaaa177f64b9469e4d9f77b75357e5847a0726ad4dceeed')

build() {
  # 统一启动器：界面仿官方 UU 远程客户端
  gcc -O2 -Wall -o uu-remote-ui uu-remote-ui.c $(pkg-config --cflags --libs gtk+-3.0)
}

package() {
  local _up="uu-remote-linux-v${pkgver}-x86_64-linux"
  local d="$pkgdir$_pfx"
  install -d "$d/bin" "$d/lib" "$d/share/glib-2.0/schemas"
  install -d "$d/lib/gtk-3.0/3.0.0" "$d/lib/uu-remote-linux"

  # --- 主程序 ---
  install -Dm755 "$srcdir/$_up/uuremote-server" "$d/bin/uuremote-server"
  install -Dm755 "$srcdir/$_up/uuremote-viewer" "$d/bin/uuremote-viewer"
  install -Dm755 "$srcdir/$_up/contrib/virtual-desktop/uu-virtual-desktop" "$d/bin/uu-virtual-desktop"
  install -Dm755 "$(command -v ffmpeg)" "$d/bin/ffmpeg"

  # --- 统一入口（被控端 / 控制端 合二为一）---
  install -Dm755 "$srcdir/uu-remote" "$d/bin/uu-remote"
  install -Dm755 "$srcdir/uu-remote-ui" "$d/bin/uu-remote-ui"

  # --- 采集插件（server 按 <exe>/../lib/uu-remote-linux 查找）---
  install -Dm644 "$srcdir/$_up/libuu-portal.so" "$d/lib/uu-remote-linux/libuu-portal.so"
  install -Dm644 "$srcdir/$_up/libuu-wayland.so" "$d/lib/uu-remote-linux/libuu-wayland.so"

  # --- 运行库（viewer + server + ffmpeg 的全部依赖，排除 glibc 核心）---
  for b in "$d/bin/uuremote-viewer" "$d/bin/uuremote-server" "$d/bin/uu-virtual-desktop" "$d/bin/uu-remote-ui" "$d/bin/ffmpeg"; do
    ldd "$b" 2>/dev/null | awk '{print $3}' | grep '^/' || true
  done | sort -u | while read -r lib; do
    case "$(basename "$lib")" in
      ld-linux*|libc.so.*|libm.so.*|libpthread.so.*|libdl.so.*|librt.so.*|libresolv.so.*|libutil.so.*|libnsl.so.*|libanl.so.*|libnss_*.so.*) continue ;;
    esac
    install -Dm755 "$lib" "$d/lib/$(basename "$lib")"
  done

  # 通过 UU_FFMPEG_ROOT / config.ffmpeg_root 启动 ffmpeg 时，
  # 上游只会把 <root>/lib/x86_64-linux-gnu 放进 LD_LIBRARY_PATH，这里补一份软链。
  install -d "$d/lib/x86_64-linux-gnu"
  for lib in "$d"/lib/*.so*; do
    [ -e "$lib" ] || continue
    ln -sf "../$(basename "$lib")" "$d/lib/x86_64-linux-gnu/$(basename "$lib")"
  done

  # --- GTK 输入法模块 + 缓存（缓存里的路径改写到 /opt）---
  cp -a /usr/lib/gtk-3.0/3.0.0/immodules "$d/lib/gtk-3.0/3.0.0/"
  sed "s|/usr/lib/gtk-3.0/3.0.0/immodules|$_pfx/lib/gtk-3.0/3.0.0/immodules|g" \
    /usr/lib/gtk-3.0/3.0.0/immodules.cache > "$d/lib/gtk-3.0/3.0.0/immodules.cache"

  # --- glib schemas ---
  install -Dm644 /usr/share/glib-2.0/schemas/gschemas.compiled "$d/share/glib-2.0/schemas/gschemas.compiled"

  # --- 启动器 ---
  install -d "$pkgdir/usr/bin"
  for prog in uuremote-viewer uuremote-server uu-remote uu-virtual-desktop; do
    cat > "$pkgdir/usr/bin/$prog" <<EOF
#!/bin/sh
_pfx=$_pfx
export LD_LIBRARY_PATH="\$_pfx/lib\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}"
export PATH="\$_pfx/bin:\$PATH"
export GSETTINGS_SCHEMA_DIR="\$_pfx/share/glib-2.0/schemas\${GSETTINGS_SCHEMA_DIR:+:\$GSETTINGS_SCHEMA_DIR}"
export GTK_IM_MODULE_FILE="\$_pfx/lib/gtk-3.0/3.0.0/immodules.cache"
export XDG_DATA_DIRS="\$_pfx/share:\${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"
exec "\$_pfx/bin/$prog" "\$@"
EOF
    chmod 755 "$pkgdir/usr/bin/$prog"
  done

  # --- 桌面入口 / 图标 / 文档 / 服务 / udev ---
  # 只留一个菜单项：点开后在「被控端（主机）」和「控制端」之间选择
  install -Dm644 "$srcdir/uu-remote-linux.desktop" "$pkgdir/usr/share/applications/uu-remote-linux.desktop"
  install -Dm644 "$srcdir/$_up/contrib/desktop/uu-remote-linux.svg" "$pkgdir/usr/share/icons/hicolor/scalable/apps/uu-remote-linux.svg"
  install -Dm644 "$srcdir/uuremote-native.service" "$pkgdir/usr/lib/systemd/user/uuremote-native.service"
  sed 's|^ExecStart=.*|ExecStart=/usr/bin/uu-virtual-desktop|' \
    "$srcdir/$_up/contrib/virtual-desktop/uu-virtual-desktop.service" \
    > "$pkgdir/usr/lib/systemd/user/uu-virtual-desktop.service"
  install -Dm644 "$srcdir/60-uu-remote-linux-uinput.rules" "$pkgdir/usr/lib/udev/rules.d/60-uu-remote-linux-uinput.rules"
  install -Dm644 "$srcdir/$_up/config.example.json" "$pkgdir/usr/share/doc/uu-remote-linux-bundle/config.example.json"
  install -Dm644 "$srcdir/$_up/contrib/virtual-desktop/virtual-display.conf" "$pkgdir/usr/share/doc/uu-remote-linux-bundle/virtual-display.conf.example"
  install -Dm644 "$srcdir/$_up/LICENSE" "$pkgdir/usr/share/licenses/uu-remote-linux-bundle/LICENSE"
}
