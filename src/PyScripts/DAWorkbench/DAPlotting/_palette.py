"""Seaborn-style categorical colour palettes for DAPlotting.

These hex palettes are copied from ``seaborn``'s built-in defaults so that the
Python statistical plots can colour categorical ``hue`` groups without pulling
matplotlib/seaborn at runtime. Only the colour values are borrowed — rendering
still happens entirely in Qwt through ``da_figure``.
"""

PALETTES = {
    "deep": [
        "#4C72B0", "#DD8452", "#55A467", "#C44E52", "#8172B3",
        "#937860", "#DA8BC3", "#8C8C8C", "#CCB974", "#64B5CD",
    ],
    "muted": [
        "#4878D0", "#EE854A", "#6ACC64", "#D65F5F", "#956CB4",
        "#8C613C", "#DC7EC0", "#797979", "#D5BB67", "#82C6E2",
    ],
    "pastel": [
        "#A1C9F4", "#FFB482", "#8DE5A1", "#FF9F9A", "#D7BDE2",
        "#FAB0E4", "#C6BCD7", "#C9E5F0", "#D5E5A0", "#F0CDE2",
    ],
    "bright": [
        "#0173B2", "#DE8F05", "#029E73", "#D55E00", "#CC78BC",
        "#CA9161", "#FBAFE4", "#F1A9A0", "#A9C5E6", "#9E9E9E",
    ],
    "dark": [
        "#001C7F", "#B43E92", "#008B8B", "#8F4E00", "#783F00",
        "#3A0A0A", "#5E4DFC", "#8A6E00", "#5C002E", "#8B8B8B",
    ],
    "colorblind": [
        "#0173B2", "#DE8F05", "#029E73", "#D55E00", "#CC78BC",
        "#CA9161", "#FBAFE4", "#A9C5E6", "#9E9E9E", "#FFFFFF",
    ],
}

DEFAULT_PALETTE = "deep"


def get_palette(palette_name="deep", n=10):
    """Return a list of ``n`` hex colour strings from the named palette.

    If ``n`` exceeds the palette length, colours are cycled. Unknown palette
    names fall back to the ``deep`` palette.
    """
    colors = PALETTES.get(palette_name, PALETTES[DEFAULT_PALETTE])
    return [colors[i % len(colors)] for i in range(n)]
