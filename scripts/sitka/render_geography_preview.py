#!/usr/bin/env python3
"""Render an evidentiary preview from a VERIFIED USGS GeoTIFF (not concept art)."""
import argparse
from pathlib import Path

from acquire_3dep_geography import load_grid, NODATA


def preview(dem, grid_file, output):
    import numpy as np
    import rasterio
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    cfg = load_grid(grid_file)
    g = cfg["grid"]
    output = Path(output)
    with rasterio.open(dem) as ds:
        data = ds.read(1)
        valid = (data != NODATA) & np.isfinite(data)
        if np.count_nonzero(valid) < 100:
            raise ValueError("refuse preview: DEM has no real elevation coverage")
        data = np.ma.masked_where(~valid,data)
        west = float(g["west_grid_node_m"])
        north = float(g["north_grid_node_m"])
        size = g["tile_intervals"] * g["sample_spacing_m"]
        east = west + size*g["columns"]
        south = north - size*g["rows"]
        fig, ax = plt.subplots(figsize=(9, 9), dpi=160)
        p2,p98 = np.percentile(data.compressed(), [2,98])
        rendered = ax.imshow(
            data, extent=[west,east,south,north], origin="upper",
            interpolation="nearest", cmap="terrain", vmin=float(p2), vmax=float(p98))
        for col in range(g["columns"]+1):
            x=west+col*size
            ax.axvline(x,color="black",lw=.9,alpha=.72)
        for row in range(g["rows"]+1):
            y=north-row*size
            ax.axhline(y,color="black",lw=.9,alpha=.72)
        for row in range(g["rows"]):
            for col in range(g["columns"]):
                x=west+(col+.5)*size
                y=north-(row+.5)*size
                ax.text(x,y,f"{row:02d}/{col:02d}",ha="center",va="center",
                        fontsize=8,color="black",bbox=dict(
                            facecolor="white",alpha=.65,edgecolor="none",pad=1.5))
        ax.set_title("Sitka, Alaska | authentic USGS 3DEP elevation samples\n"
                     "512 m grid / 4 m samples / EPSG:32608")
        ax.set_xlabel("UTM Easting (m)")
        ax.set_ylabel("UTM Northing (m)")
        fig.colorbar(rendered,ax=ax,shrink=.73,label="Interpolated ground elevation (m)")
        ax.text(.01,.01,
                "3DEP SOURCE HEIGHTS ONLY: not a coastline, harbor, shoreline or sonar map.\n"
                "Vertical datum not independently reconciled. Not for navigation.",
                transform=ax.transAxes,ha="left",va="bottom",
                fontsize=8,bbox=dict(facecolor="white",alpha=.88,edgecolor="none"))
        fig.tight_layout()
        output.parent.mkdir(parents=True,exist_ok=True)
        fig.savefig(output,dpi=160)
        plt.close(fig)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--dem",required=True,type=Path)
    p.add_argument("--grid",required=True,type=Path)
    p.add_argument("--output",required=True,type=Path)
    args=p.parse_args()
    preview(args.dem,args.grid,args.output)
    print("Saved actual DEM diagnostic preview:",args.output)


if __name__=="__main__":
    main()
