#!/usr/bin/env python3
"""Evidence image: real USGS heights under official NOAA ENC chart land polygons."""
import argparse
from pathlib import Path
import numpy as np

def render(dem,land,output):
    import rasterio
    from pyproj import Transformer
    from shapely.geometry import shape
    from shapely.ops import transform
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import json
    with rasterio.open(dem) as src:
        z=src.read(1,masked=True)
        west=src.bounds.left
        east=src.bounds.right
        north=src.bounds.top
        south=src.bounds.bottom
        a=np.ma.masked_invalid(z)
        fig,ax=plt.subplots(figsize=(10,10),dpi=145)
        colors=ax.imshow(a,extent=[west,east,south,north],
                         cmap="terrain",origin="upper",interpolation="nearest",
                         vmin=float(np.percentile(a.compressed(),2)),
                         vmax=float(np.percentile(a.compressed(),98)))
        geo=json.loads(Path(land).read_text(encoding="utf-8"))
        if geo.get("type")!="FeatureCollection" or not geo.get("features"):
            raise ValueError("NOAA polygon overlay missing")
        to_utm=Transformer.from_crs(4326,src.crs,always_xy=True)
        count=0
        for f in geo["features"]:
            p=transform(to_utm.transform,shape(f["geometry"]))
            polys=[p] if p.geom_type=="Polygon" else list(p.geoms)
            for polygon in polys:
                x,y=polygon.exterior.xy
                ax.plot(x,y,lw=1.6,color="red",alpha=.95)
                for hole in polygon.interiors:
                    x,y=hole.xy
                    ax.plot(x,y,lw=1.1,color="red",alpha=.9)
                count+=1
        if count==0:raise ValueError("No real NOAA land polygon edges plotted")
        ax.set_xlim(west,east)
        ax.set_ylim(south,north)
        ax.set_aspect("equal")
        ax.set_xlabel("UTM Zone 8N easting (m)")
        ax.set_ylabel("UTM Zone 8N northing (m)")
        ax.set_title("Sitka / REAL USGS 3DEP elevation and NOAA ENC chart land polygons")
        fig.colorbar(colors,ax=ax,shrink=.7,label="3DEP ground elevation (m)")
        ax.text(.01,.01,
           "RED = NOAA ENC chart-polygon boundaries (includes clipped AOI edges).\n"
           "NOT a NOAA 2023 surveyed MHW line. No marine bathymetry verified.\n"
           "Source-only preview. NOT FOR NAVIGATION.",
           transform=ax.transAxes,va="bottom",ha="left",fontsize=8,
           bbox=dict(facecolor="white",alpha=.9,edgecolor="none"))
        fig.tight_layout()
        output=Path(output)
        output.parent.mkdir(parents=True,exist_ok=True)
        fig.savefig(output,dpi=145)
        plt.close(fig)
        print("Actual NOAA ENC / USGS DEM overlay saved:",output,"polygons:",count)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--dem",type=Path,required=True)
    p.add_argument("--land",type=Path,required=True)
    p.add_argument("--output",type=Path,required=True)
    a=p.parse_args()
    render(a.dem,a.land,a.output)
if __name__=="__main__":
    main()
