"""Read-only Unreal Editor extraction of the 24 Between Deliveries product groups.

Run inside an Unreal Editor session with the original project Content and Python
Editor Script Plugin loaded. Does not edit the project or upload licensed files.

    BETW_DELIVERIES_EXPORT_DIR=<private folder> UnrealEditor <Project>.uproject \
        -ExecutePythonScript=<absolute path to this file>

A manifest in the project's docs folder supplies the 24 discovered group names.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import zlib


def valid_filename(name):
    return bool(re.fullmatch(r"[A-Za-z0-9_-]+", name))


def png_size(path):
    """Validate the complete PNG chunk chain and CRCs, not just its 24-byte header."""
    dimensions = None
    saw_idat = False
    saw_iend = False
    with open(path, "rb") as stream:
        if stream.read(8) != b"\x89PNG\r\n\x1a\n":
            raise ValueError("Invalid PNG signature")
        while True:
            chunk = stream.read(8)
            if len(chunk) != 8:
                raise ValueError("Missing PNG IEND or truncated chunk")
            length, kind = struct.unpack(">I4s", chunk)
            if length > 256 * 1024 * 1024:
                raise ValueError("Unreasonably large PNG chunk")
            if dimensions is None and (kind != b"IHDR" or length != 13):
                raise ValueError("PNG must begin with a 13-byte IHDR")
            crc = zlib.crc32(kind)
            remaining = length
            ihdr = bytearray()
            while remaining:
                data = stream.read(min(remaining, 1024 * 1024))
                if not data:
                    raise ValueError("Truncated PNG data")
                crc = zlib.crc32(data, crc)
                if kind == b"IHDR":
                    ihdr.extend(data)
                remaining -= len(data)
            tail = stream.read(4)
            if len(tail) != 4 or struct.unpack(">I", tail)[0] != (crc & 0xffffffff):
                raise ValueError("Truncated or CRC-corrupt PNG")
            if kind == b"IHDR":
                if dimensions is not None or length != 13:
                    raise ValueError("Malformed PNG IHDR")
                width, height = struct.unpack(">II", ihdr[:8])
                if not (16 <= width <= 16384 and 16 <= height <= 16384):
                    raise ValueError("Unreasonable texture dimensions")
                dimensions = (width, height)
            elif kind == b"IDAT":
                saw_idat = True
            elif kind == b"IEND":
                if length != 0 or not saw_idat or dimensions is None:
                    raise ValueError("Malformed PNG IEND")
                saw_iend = True
                if stream.read(1):
                    raise ValueError("Trailing unexpected PNG data")
                break
    if not saw_iend:
        raise ValueError("Incomplete PNG")
    return dimensions


def asset_sha(path):
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_catalog(root):
    location = root / "docs/project/BETWEEN_DELIVERIES_UNREAL_CATALOG.json"
    doc = json.loads(location.read_text(encoding="utf-8"))
    products = doc.get("products", [])
    if len(products) != 24:
        raise ValueError("Expected all 24 original product mappings")
    folders = set()
    for product in products:
        code, folder = str(product["supplier_id"]), product["folder_label"]
        if code not in ("101", "202", "303", "404", "505", "606"):
            raise ValueError("Unexpected supplier")
        if not valid_filename(folder) or folder in folders:
            raise ValueError("Duplicate or unsafe asset folder")
        folders.add(folder)
    return products


def validate_export(path, kind):
    if not path.is_file() or path.stat().st_size < 64:
        raise ValueError("No usable exported file")
    metadata = {"size_bytes": path.stat().st_size, "sha256": asset_sha(path)}
    if kind == "texture":
        width, height = png_size(path)
        metadata["width"] = width
        metadata["height"] = height
    elif kind == "mesh":
        with open(path, "rb") as stream:
            header = stream.read(24)
        if not (header.startswith(b"Kaydara FBX Binary") or header.startswith(b"; FBX")):
            raise ValueError("Mesh export does not have FBX header")
    return metadata


def export_group(unreal, product, output, records, failures):
    folder = product["folder_label"]
    prefix = "/Game/Models/Props/Booze/" + folder
    # list_assets enumerates the actual Unreal object paths; the offline
    # catalog's inferred individual object links are not assumed authoritative.
    paths = sorted(set(str(p) for p in unreal.EditorAssetLibrary.list_assets(prefix, True, False)))
    if not paths:
        failures.append({"product": folder, "error": "No resolved package objects"})
        return
    for package in paths:
        if not (package.startswith(prefix + "/") or package == prefix):
            continue
        asset = unreal.EditorAssetLibrary.load_asset(package)
        if asset is None:
            failures.append({"object": package, "error": "Unreal load failed"})
            continue
        if isinstance(asset, unreal.Texture2D):
            kind, suffix, exporter = "texture", ".png", unreal.TextureExporterPNG()
        elif isinstance(asset, unreal.StaticMesh):
            kind, suffix, exporter = "mesh", ".fbx", unreal.StaticMeshExporterFBX()
        else:
            continue  # materials are not portable; export associated textures
        name = str(asset.get_name())
        if not valid_filename(name):
            failures.append({"object": package, "error": "Unsafe object name"})
            continue
        target = output / "products" / product["supplier_id"] / folder / kind / (name + suffix)
        target.parent.mkdir(parents=True, exist_ok=True)
        record = {"source_unreal_object": package, "sku": product["design_name"],
                  "supplier_id": product["supplier_id"], "kind": kind,
                  "file": target.relative_to(output).as_posix(),
                  "rights_status": "owner-confirmed"}
        if target.exists():
            record["status"] = "existing - untouched"
        else:
            task = unreal.AssetExportTask()
            task.object = asset
            task.filename = str(target)
            task.exporter = exporter
            task.automated = True
            task.prompt = False
            task.replace_identical = False
            if not unreal.Exporter.run_asset_export_task(task):
                failures.append({"object": package, "error": "Exporter failed"})
                continue
            record["status"] = "exported"
        try:
            record.update(validate_export(target, kind))
            if kind == "texture" and name == "101draft_texture_0":
                if (record["width"], record["height"]) != (1024, 1024):
                    raise ValueError("101draft original 1024x1024 texture was degraded")
        except (OSError, ValueError) as error:
            failures.append({"object": package, "error": str(error)})
            # Never ship failed or incomplete outputs: leave originals alone.
            continue
        records.append(record)



def export_icons(unreal, output, records, failures):
    """Full-resolution Unreal Texture2D exports, not 256px asset thumbnails."""
    for slot in range(1, 25):
        name = "icon_{:02d}_300_v2_rt_final".format(slot)
        package = "/Game/texture/ICONS/" + name
        try:
            asset = unreal.EditorAssetLibrary.load_asset(package)
            if asset is None or not isinstance(asset, unreal.Texture2D):
                raise ValueError("Canonical original Texture2D cannot be loaded")
            destination = output / "icons" / (name + ".png")
            destination.parent.mkdir(parents=True, exist_ok=True)
            status = "existing - untouched"
            if not destination.exists():
                task = unreal.AssetExportTask()
                task.object = asset
                task.filename = str(destination)
                task.exporter = unreal.TextureExporterPNG()
                task.automated = True
                task.prompt = False
                task.replace_identical = False
                if not unreal.Exporter.run_asset_export_task(task):
                    raise ValueError("Full-resolution PNG export failed")
                status = "exported"
            metadata = validate_export(destination, "texture")
            if (metadata["width"], metadata["height"]) != (300, 300):
                raise ValueError("Degraded output: original canonical icon must be 300x300")
            records.append({"source_unreal_object": package,
                            "kind": "original_icon", "slot": slot,
                            "file": destination.relative_to(output).as_posix(),
                            "status": status,
                            "rights_status": "owner-confirmed",
                            **metadata})
        except (OSError, ValueError, RuntimeError) as error:
            failures.append({"object": package, "error": str(error)})


def export_table(unreal, output, records, failures):
    path = "/Game/Data_tables/DT_ShopItems"
    table = unreal.EditorAssetLibrary.load_asset(path)
    if table is None:
        failures.append({"object": path, "error": "Original DataTable or its row struct is unavailable"})
        return
    destination = output / "source_tables" / "DT_ShopItems.csv"
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        ok = destination.is_file() and destination.stat().st_size > 32
        status = "existing - untouched"
    else:
        ok = bool(unreal.DataTableFunctionLibrary.export_data_table_to_csv_file(table, str(destination)))
        status = "exported"
    if not ok or not destination.is_file() or destination.stat().st_size < 32:
        failures.append({"object": path, "error": "Could not export full source CSV"})
        return
    records.append({"source_unreal_object": path, "file": destination.relative_to(output).as_posix(),
                    "status": status, "size_bytes": destination.stat().st_size,
                    "sha256": asset_sha(destination), "rights_status": "owner-confirmed"})


def main():
    import unreal  # Unreal's bundled Python; intentionally unavailable in standard CPython
    root = Path(__file__).resolve().parents[2]
    products = load_catalog(root)
    output = Path(os.environ.get("BETW_DELIVERIES_EXPORT_DIR", str(Path.home() / "BetweenDeliveries_UnrealExports"))).expanduser().resolve()
    if output == root or root in output.parents:
        raise ValueError("Export output must be outside the public Git checkout")
    output.mkdir(parents=True, exist_ok=True)
    records, failures = [], []
    export_table(unreal, output, records, failures)
    export_icons(unreal, output, records, failures)
    for product in products:
        try:
            export_group(unreal, product, output, records, failures)
        except Exception as error:
            failures.append({"product": product["folder_label"], "error": str(error)})
    result = {"status": "owner-confirmed original export attempt; native runtime integration pending",
              "products_scanned": len(products), "files_validated": len(records),
              "records": records, "failures": failures}
    # Durable manifest; source inclusion is authorized by the asset owner.
    temp = output / "export_manifest.json.tmp"
    temp.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    temp.replace(output / "export_manifest.json")
    print("Between Deliveries export: {} validated files; {} issues. Output: {}".format(
        len(records), len(failures), output))
    if failures:
        raise RuntimeError("Some assets failed to export; see private manifest")


if __name__ == "__main__":
    main()
