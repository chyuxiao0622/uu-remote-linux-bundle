# Maintainer: local bundle
pkgname=uu-remote-linux-bundle
pkgver=0.2.57
pkgrel=1
pkgdesc='Unofficial Linux host and viewer for NetEase UU Remote (self-contained: all runtime deps bundled)'
arch=('x86_64')
url='https://github.com/VallierDev/uu-remote-linux'
license=('GPL-3.0-or-later')
depends=('glibc')
provides=('uu-remote' 'uu-remote-linux-bin')
conflicts=('uu-remote' 'uu-remote-linux-bin' 'uu-remote-linux')
options=('!debug' '!emptydirs')
_pfx=/opt/uu-remote-linux

package() {
  local d="$pkgdir$_pfx"
  install -d "$d/bin" "$d/lib" "$d/share/glib-2.0/schemas"
  install -d "$d/lib/gtk-3.0/3.0.0" "$d/lib/uu-remote-linux"

  # --- 主程序 ---
  install -Dm755 /usr/bin/uuremote-viewer "$d/bin/uuremote-viewer"
  install -Dm755 /usr/bin/uuremote-server "$d/bin/uuremote-server"
  install -Dm755 /usr/bin/uu-virtual-desktop "$d/bin/uu-virtual-desktop"
  install -Dm755 /usr/bin/ffmpeg "$d/bin/ffmpeg"

  # --- 采集插件（server 按 <exe>/../lib/uu-remote-linux 查找）---
  install -Dm644 /usr/lib/uu-remote-linux/libuu-portal.so "$d/lib/uu-remote-linux/libuu-portal.so"
  install -Dm644 /usr/lib/uu-remote-linux/libuu-wayland.so "$d/lib/uu-remote-linux/libuu-wayland.so"

  # --- 运行库（viewer + server + ffmpeg 的全部依赖，排除 glibc 核心）---
  for b in "$d/bin/uuremote-viewer" "$d/bin/uuremote-server" "$d/bin/ffmpeg"; do
    ldd "$b" 2>/dev/null | awk '{print $3}' | grep '^/' || true
  done | sort -u | while read -r lib; do
    case "$(basename "$lib")" in
      ld-linux*|libc.so.*|libm.so.*|libpthread.so.*|libdl.so.*|librt.so.*|libresolv.so.*|libutil.so.*|libnsl.so.*|libanl.so.*|libnss_*.so.*) continue ;;
    esac
    install -Dm755 "$lib" "$d/lib/$(basename "$lib")"
  done

  # --- GTK 输入法模块 + 缓存（缓存里的路径改写到 /opt）---
  cp -a /usr/lib/gtk-3.0/3.0.0/immodules "$d/lib/gtk-3.0/3.0.0/"
  sed "s|/usr/lib/gtk-3.0/3.0.0/immodules|$_pfx/lib/gtk-3.0/3.0.0/immodules|g" \
    /usr/lib/gtk-3.0/3.0.0/immodules.cache > "$d/lib/gtk-3.0/3.0.0/immodules.cache"

  # --- glib schemas ---
  install -Dm644 /usr/share/glib-2.0/schemas/gschemas.compiled "$d/share/glib-2.0/schemas/gschemas.compiled"

  # --- 启动器 ---
  install -d "$pkgdir/usr/bin"
  for prog in uuremote-viewer uuremote-server; do
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
  install -Dm644 /usr/share/applications/uu-remote-linux-viewer.desktop "$pkgdir/usr/share/applications/uu-remote-linux-viewer.desktop"
  install -Dm644 /usr/share/applications/uu-remote-linux.desktop "$pkgdir/usr/share/applications/uu-remote-linux.desktop"
  install -Dm644 /usr/share/icons/hicolor/scalable/apps/uu-remote-linux.svg "$pkgdir/usr/share/icons/hicolor/scalable/apps/uu-remote-linux.svg"
  install -Dm644 /usr/lib/systemd/user/uuremote-native.service "$pkgdir/usr/lib/systemd/user/uuremote-native.service"
  install -Dm644 /usr/lib/systemd/user/uu-virtual-desktop.service "$pkgdir/usr/lib/systemd/user/uu-virtual-desktop.service"
  install -Dm644 /usr/lib/udev/rules.d/60-uu-remote-linux-uinput.rules "$pkgdir/usr/lib/udev/rules.d/60-uu-remote-linux-uinput.rules"
  install -Dm644 /usr/share/doc/uu-remote-linux-bin/config.example.json "$pkgdir/usr/share/doc/uu-remote-linux-bundle/config.example.json"
  install -Dm644 /usr/share/doc/uu-remote-linux-bin/virtual-display.conf.example "$pkgdir/usr/share/doc/uu-remote-linux-bundle/virtual-display.conf.example"
  install -Dm644 /usr/share/licenses/uu-remote-linux-bin/LICENSE "$pkgdir/usr/share/licenses/uu-remote-linux-bundle/LICENSE"
}
