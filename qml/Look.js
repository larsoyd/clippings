// the look of the editor: the theme's [clippings] table, then shared controls, then plain values

.pragma library

function value(settings, key, fallback) {
    const table = settings.theme.clippings ?? ({})
    return table[key] ?? fallback
}

function background(settings) {
    return value(settings, "background", settings.theme.controls?.window_color ?? "#ededed")
}
