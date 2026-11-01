# Clippings

A screen capture app for [Mavernix](https://github.com/Mavernix/mavernix). Print Screen freezes the desktop, you choose a part, a window or a screen, and the picture opens in a document window to mark up, save, copy or drag elsewhere.

- Selection, window, screen and all screens, with a delay and an optional pointer
- Crop, arrows, lines, rectangles, ovals, text, sketching and highlighting
- Solid redaction that replaces the covered pixels, plus pixelate and blur
- Undo, zoom, PNG, JPEG, TIFF and WebP, a clipboard that outlives the program

Pixelate and blur hide detail but they are not a secure way to remove text. Use Redact for anything sensitive.

## Building

Needs Qt 6, LayerShellQt, `grim` and `wl-copy`. Window captures use Mavernix's `window-pictures` compositor plugin.

```sh
meson setup build && meson compile -C build && meson test -C build
```

## Installing for yourself

The icon and tool pictures aren't in this repository. They come from an OS X installer you own:

```sh
packaging/import-art /path/to/installer
packaging/install-local
```

Bind a key by adding this to `commands` in your Mavernix choices file:

```json
"clippings": {"key": "KEY_SYSRQ", "run": ["/path/to/opt/clippings/bin/clippings", "--capture", "selection"]}
```

## License

GPL-3.0-or-later. `instance-door.cpp` and the controls in `qml/shell` started in [Bandstand](https://github.com/Mavernix/bandstand).
