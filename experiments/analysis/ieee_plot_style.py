import matplotlib.pyplot as plt

CB_BLUE = "#0072B2"
CB_ORANGE = "#E69F00"
CB_GREEN = "#009E73"
CB_RED = "#D55E00"
CB_PURPLE = "#CC79A7"
CB_GRAY = "#999999"

def apply_ieee_style():
    plt.rcParams.update({
        "font.family": "serif",
        "font.serif": ["Times New Roman", "Times", "DejaVu Serif"],
        "mathtext.fontset": "stix",
        "font.size": 8,
        "axes.titlesize": 8,
        "axes.labelsize": 8,
        "xtick.labelsize": 7,
        "ytick.labelsize": 7,
        "legend.fontsize": 7,
        "figure.titlesize": 8,
        "axes.linewidth": 0.8,
        "lines.linewidth": 1.2,
        "lines.markersize": 4,
        "grid.linewidth": 0.4,
        "savefig.dpi": 300,
        "pdf.fonttype": 42,
        "ps.fonttype": 42,
    })

def set_ieee_size(fig, width="single", height_ratio=0.62):
    if width == "single":
        w = 3.5
    elif width == "double":
        w = 7.16
    else:
        w = float(width)
    fig.set_size_inches(w, w * height_ratio)
