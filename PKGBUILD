# Maintainer: chyuxiao0622 <https://github.com/chyuxiao0622/uu-remote-linux-bundle>
# 自包含包：上游源码本地编译，运行库（GTK3 / ffmpeg / SDL2 ...）全部打进 /opt，
# 目标机器只需要 glibc。
# 额外打一个补丁：把「被控端」和「控制端」合并成一个入口（主机窗口里直接拉起控制端）。
pkgname=uu-remote-linux-bundle
pkgver=0.2.57
pkgrel=3
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

# 运行时后端：Wayland/Portal + 引擎插件 + 控制端
_features='wayland,portal,engine-plugins,native-viewer,viewer-sdl'

# 只在构建机上需要
makedepends=(
  'cargo'
  'cairo'
  'ffmpeg'
  'gdk-pixbuf2'
  'glib2'
  'gtk3'
  'libei'
  'libpipewire'
  'libx11'
  'libxext'
  'libxfixes'
  'libxkbcommon'
  'libxtst'
  'pango'
  'pkg-config'
  'rust'
  'sdl2-compat'
  'wayland'
)

source=(
  "https://github.com/VallierDev/uu-remote-linux/archive/refs/tags/v${pkgver}.tar.gz"
  'uu-merged-entry.patch'
  'uuremote-native.service'
  '60-uu-remote-linux-uinput.rules'
)
sha256sums=(
  'd97db5a93d6cfd71b1973a41694032d6b81d1f894f6359ffa93b3b2b871cb04b'
  '37d082770b7ffc7f3d37787d6f91f1bdf22ed89c9eeaf8ae48a3c5fac7160686'
  '1bba2b6ada40e980c23c4a78188d694e8c8bc39c3d39e9ffb8a41947501da48e'
  'df21d14e39e6912261e9ecdc2422d242612053beb2cbc3a8755938020abf3e6f'
)

build() {
  cd "${srcdir}/uu-remote-linux-${pkgver}"

  # makepkg 默认给 CFLAGS/LDFLAGS 加 -flto=auto（OPTIONS 里有 lto）。这样 cc 编出来的 .o 是
  # LTO 字节码，归档成 .a 后链接器认不出符号（undefined symbol: uu_viewer_ui_* / ring 的
  # GFp_*），所以这里把 -flto 去掉。
  export CFLAGS="${CFLAGS//-flto=auto/}"
  export CXXFLAGS="${CXXFLAGS//-flto=auto/}"
  export LDFLAGS="${LDFLAGS//-flto=auto/}"

  # 合并被控端与控制端的入口：主机窗口里直接拉起同目录的控制端
  # （已经打过补丁时跳过，方便增量构建）
  if patch -Np1 --dry-run -i "${srcdir}/uu-merged-entry.patch" >/dev/null 2>&1; then
    patch -Np1 -i "${srcdir}/uu-merged-entry.patch"
  else
    echo "uu-merged-entry.patch 已经应用过了，跳过" >&2
  fi

  cargo build --release --locked --features "$_features"
}

package() {
  local _up="uu-remote-linux-${pkgver}"
  local d="$pkgdir$_pfx"
  install -d "$d/bin" "$d/lib" "$d/share/glib-2.0/schemas"
  install -d "$d/lib/gtk-3.0/3.0.0" "$d/lib/uu-remote-linux"

  # --- 主程序（本地源码编译）---
  install -Dm755 "${srcdir}/${_up}/target/release/uuremote-server" "$d/bin/uuremote-server"
  install -Dm755 "${srcdir}/${_up}/target/release/uuremote-viewer" "$d/bin/uuremote-viewer"
  install -Dm755 "${srcdir}/${_up}/contrib/virtual-desktop/uu-virtual-desktop" "$d/bin/uu-virtual-desktop"
  install -Dm755 "$(command -v ffmpeg)" "$d/bin/ffmpeg"

  # --- 采集/编码插件（server 按 <exe>/../lib/uu-remote-linux 查找）---
  install -Dm644 "${srcdir}/${_up}/target/release/libuu-portal.so" "$d/lib/uu-remote-linux/libuu-portal.so"
  install -Dm644 "${srcdir}/${_up}/target/release/libuu-wayland.so" "$d/lib/uu-remote-linux/libuu-wayland.so"

  # --- 运行库（viewer + server + ffmpeg 的全部依赖，排除 glibc 核心）---
  for b in "$d/bin/uuremote-viewer" "$d/bin/uuremote-server" "$d/bin/uu-virtual-desktop" "$d/bin/ffmpeg"; do
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
  for prog in uuremote-viewer uuremote-server uu-virtual-desktop; do
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
  # 只留一个菜单项：被控端和控制端合并在同一个窗口里
  install -Dm644 "$srcdir/${_up}/contrib/desktop/uu-remote-linux.desktop" "$pkgdir/usr/share/applications/uu-remote-linux.desktop"
  sed -i -e 's/^Name=.*/Name=UU 远程 Linux/' \
         -e 's/^Name\[en\]=.*/Name[en]=UU Remote Linux/' \
         -e 's/^Comment=.*/Comment=非官方 UU 远程：被控端（主机）与控制端/' \
         -e 's/^Comment\[en\]=.*/Comment[en]=Unofficial NetEase UU Remote: this host (remote assistance) and the viewer/' \
         "$pkgdir/usr/share/applications/uu-remote-linux.desktop"
  install -Dm644 "$srcdir/${_up}/contrib/desktop/uu-remote-linux.svg" "$pkgdir/usr/share/icons/hicolor/scalable/apps/uu-remote-linux.svg"
  install -Dm644 "$srcdir/uuremote-native.service" "$pkgdir/usr/lib/systemd/user/uuremote-native.service"
  sed 's|^ExecStart=.*|ExecStart=/usr/bin/uu-virtual-desktop|' \
    "$srcdir/${_up}/contrib/virtual-desktop/uu-virtual-desktop.service" \
    > "$pkgdir/usr/lib/systemd/user/uu-virtual-desktop.service"
  install -Dm644 "$srcdir/60-uu-remote-linux-uinput.rules" "$pkgdir/usr/lib/udev/rules.d/60-uu-remote-linux-uinput.rules"
  install -Dm644 "$srcdir/${_up}/config.example.json" "$pkgdir/usr/share/doc/uu-remote-linux-bundle/config.example.json"
  install -Dm644 "$srcdir/${_up}/contrib/virtual-desktop/virtual-display.conf" "$pkgdir/usr/share/doc/uu-remote-linux-bundle/virtual-display.conf.example"
  install -Dm644 "$srcdir/${_up}/LICENSE" "$pkgdir/usr/share/licenses/uu-remote-linux-bundle/LICENSE"
}
