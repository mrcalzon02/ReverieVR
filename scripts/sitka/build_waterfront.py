#!/usr/bin/env python3
"""Offline source-to-shoreline authoring for Sitka's optional .sitkash geometry.

Only supplied licensed GeoJSON is used. It NEVER downloads map data, invents
building positions or promotes estimated architecture to surveyed accuracy.
The output shares exactly the terrain.sitkam projected-metre origin/CRS.
"""
import argparse
import hashlib
import json
import math
import re
import struct
from pathlib import Path
from shore_reality import load_site_catalog, verify_feature_evidence

HEADER = struct.Struct('<8sIIdd')
MAGIC = b'SITKA01\0'
MAX_BYTES = 65536
MAX_FEATURES = 256
MAX_POINTS = 32
BUILDINGS = frozenset(('storefront', 'cannery', 'heritage', 'warehouse'))
BLOCKS = frozenset(('rock', 'forest', 'pier', 'pontoon', 'pile',
                    'harbor_shack', 'bridge_deck', 'bridge_ramp'))
SOURCE_FIELDS = ('provider', 'dataset', 'license', 'retrieved', 'url')


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as src:
        for chunk in iter(lambda: src.read(262144), b''):
            h.update(chunk)
    return h.hexdigest()


def number(value, name, minimum, maximum):
    if isinstance(value, bool):
        raise ValueError(f'{name} must be numeric')
    try:
        f = float(value)
    except (TypeError, ValueError, OverflowError) as exc:
        raise ValueError(f'{name} must be numeric') from exc
    if not math.isfinite(f) or not minimum <= f <= maximum:
        raise ValueError(f'{name} outside [{minimum}, {maximum}]')
    return f


def fmt(value):
    return f'{value:.3f}'


def provenance_record(metadata, key):
    entry = metadata.get(key)
    if not isinstance(entry, dict) or any(
        not isinstance(entry.get(f), str) or not entry[f].strip()
        for f in SOURCE_FIELDS
    ):
        raise ValueError(f'{key}: missing source attribution or license')
    if 'google' in (entry['provider'] + ' ' + entry['dataset']).lower():
        raise ValueError('Google Earth/Maps-derived offline geometry is not authorized')
    return entry


def feature_collection(path):
    data = json.loads(Path(path).read_text(encoding='utf-8'))
    if data.get('type') != 'FeatureCollection' or not isinstance(data.get('features'), list):
        raise ValueError('expected GeoJSON FeatureCollection')
    from pyproj import CRS
    crs_name = data.get('crs', {}).get('properties', {}).get('name', 'EPSG:4326')
    crs = CRS.from_user_input(crs_name)
    if not crs.is_geographic and not crs.is_projected:
        raise ValueError('unsupported source coordinate reference')
    return data['features'], crs


def feature_id(feature):
    props = feature.get('properties')
    if not isinstance(props, dict):
        raise ValueError('missing feature properties')
    value = props.get('feature_id', feature.get('id'))
    if not isinstance(value, str) or not re.fullmatch(r'[A-Za-z0-9_-]{1,64}', value):
        raise ValueError('feature_id must be stable 1..64 ASCII letters/digits/_/-')
    return value


def building_record(feature, transformer, origin, dem):
    from shapely.geometry import shape, Polygon
    from shapely.ops import transform as geom_transform
    props = feature['properties']
    kind = props['sitka_kind']
    geom = shape(feature.get('geometry'))
    if not isinstance(geom, Polygon) or geom.is_empty or geom.interiors or not geom.is_valid:
        raise ValueError('building must be one valid solid Polygon without holes')
    metric = geom_transform(transformer.transform, geom)
    if not metric.is_valid or metric.area < 0.02:
        raise ValueError('building footprint becomes invalid after CRS transform')
    coords = list(metric.exterior.coords)[:-1]
    if not 3 <= len(coords) <= MAX_POINTS:
        raise ValueError('building outline must contain 3..32 non-closing points')
    local = [(round(x - origin[0], 3), round(z - origin[1], 3)) for x, z in coords]
    if any(not math.isfinite(v) or abs(v) > 100000 for pt in local for v in pt):
        raise ValueError('building outside supported terrain-local geometry')
    polygon = Polygon(local)
    if not polygon.is_valid or polygon.area < .02 or not polygon.exterior.is_simple:
        raise ValueError('building invalid after millimetre quantization')
    # Sample all exterior vertices in the exact same DEM CRS used to bake land.
    samples = list(dem.sample(coords, masked=True))
    heights = []
    for sample in samples:
        value = sample[0]
        if getattr(value, 'mask', False) or not math.isfinite(float(value)):
            raise ValueError('building outside verified elevation coverage')
        heights.append(float(value))
    if max(heights) - min(heights) > 2.0:
        raise ValueError('building site needs explicit stepped/retaining foundation')
    base = min(heights)
    if 'base_m' in props:
        authored = number(props['base_m'], 'base_m', -2000, 2000)
        if abs(authored - base) > 2.0:
            raise ValueError('authored base_m inconsistent with DEM')
        base = authored
    if 'height_m' in props:
        height = number(props['height_m'], 'height_m', .25, 70)
        height_class = 'authored'
    elif 'building:levels' in props:
        levels = number(props['building:levels'], 'building:levels', 1, 20)
        height = number(levels * 3, 'derived approximate height', .25, 70)
        height_class = 'level_estimate'
    else:
        # Do not synthesize a generic Sitka building height just because
        # a real polygon happens to have a "storefront" material tag.
        raise ValueError('building needs sourced height_m or building:levels')
    fields = ';'.join(f'{fmt(x)},{fmt(z)}' for x, z in local)
    return f'P|{feature_id(feature)}|{kind}|{fmt(base)}|{fmt(height)}|{fields}', height_class


def block_record(feature, transformer, origin):
    from shapely.geometry import shape, Point
    props = feature['properties']
    geom = shape(feature.get('geometry'))
    if not isinstance(geom, Point) or geom.is_empty:
        raise ValueError('harbor block must be an explicitly located Point')
    east, north = transformer.transform(geom.x, geom.y)
    x = number(east - origin[0], 'block local east', -100000, 100000)
    z = number(north - origin[1], 'block local north', -100000, 100000)
    kind = props['sitka_kind']
    base = number(props.get('base_m'), 'base_m', -2000, 2000)
    width = number(props.get('width_m'), 'width_m', .05, 800)
    depth = number(props.get('depth_m'), 'depth_m', .05, 800)
    height = number(props.get('height_m'), 'height_m', .05, 80)
    yaw = number(props.get('yaw_rad'), 'yaw_rad', -3.141593, 3.141593)
    values = [fmt(v) for v in (x, z, base, width, depth, height, yaw)]
    if kind == 'bridge_ramp':
        rise = number(props.get('rise_m'), 'rise_m', -50, 50)
        if height + min(0, rise) < .05:
            raise ValueError('bridge ramp slopes below its base')
        values.append(fmt(rise))
    return '|'.join(['R' if kind == 'bridge_ramp' else 'B', feature_id(feature), kind] + values)


def bake(terrain_manifest, dem_path, buildings_path, blocks_path, ledger_path, output, site_catalog):
    from pyproj import CRS, Transformer
    from shapely.geometry import shape
    import rasterio

    terrain_manifest, dem_path, buildings_path, ledger_path, output = map(
        Path, (terrain_manifest, dem_path, buildings_path, ledger_path, output))
    blocks_path = Path(blocks_path) if blocks_path is not None else None
    manifest = json.loads(terrain_manifest.read_text(encoding='utf-8'))
    if manifest.get('format') != 'sitka-static-terrain-1':
        raise ValueError('expected verified sitka-static-terrain-1 manifest')
    mesh_name = manifest.get('mesh')
    if not isinstance(mesh_name, str) or Path(mesh_name).name != mesh_name:
        raise ValueError('terrain mesh path must be local to its manifest')
    mesh = terrain_manifest.parent / mesh_name
    if digest(mesh) != manifest.get('mesh_sha256'):
        raise ValueError('terrain mesh checksum mismatch')
    with mesh.open('rb') as src:
        data = src.read(HEADER.size)
    if len(data) != HEADER.size or HEADER.unpack(data)[0] != MAGIC:
        raise ValueError('invalid terrain header')
    _, _, _, east0, north0 = HEADER.unpack(data)
    if not all(math.isfinite(x) for x in (east0, north0)):
        raise ValueError('terrain origin nonfinite')
    if abs(east0 - manifest.get('east_origin', float('inf'))) > .001 or \
       abs(north0 - manifest.get('north_origin', float('inf'))) > .001:
        raise ValueError('terrain origin/manifest mismatch')
    ledger = json.loads(ledger_path.read_text(encoding='utf-8'))
    sites = load_site_catalog(site_catalog)
    provenance_record(ledger, 'buildings')
    if blocks_path is not None:
        provenance_record(ledger, 'harbor_structures')
    building_features, buildings_crs = feature_collection(buildings_path)
    block_features, blocks_crs = feature_collection(blocks_path) if blocks_path is not None else ([], None)
    if not building_features and not block_features:
        raise ValueError('no shore features')
    crs = CRS.from_user_input(manifest['coordinate_system'])
    if not crs.is_projected or not any(
        a.unit_name.lower() in ('metre', 'meter') for a in crs.axis_info[:2]
    ):
        raise ValueError('terrain must be in projected metres')
    locations = (east0, north0)
    to_building = Transformer.from_crs(buildings_crs, crs, always_xy=True)
    to_blocks = Transformer.from_crs(blocks_crs, crs, always_xy=True) if blocks_crs else None
    records = []
    height_classes = {}
    evidence_ids = {}
    site_links = {}
    ids = set()
    with rasterio.open(dem_path) as dem:
        if str(dem.crs) != str(crs) or dem.count != 1 or dem.nodata is None:
            raise ValueError('shore DEM must match terrain projected CRS and NoData')
        if digest(dem_path) != manifest.get('elevation_sha256'):
            raise ValueError('shore DEM does not match terrain source raster')
        for feature, transform, valid_kinds, converter in (
            *((f, to_building, BUILDINGS, 'building') for f in building_features),
            *((f, to_blocks, BLOCKS, 'block') for f in block_features),
        ):
            if not isinstance(feature, dict):
                raise ValueError('invalid GeoJSON feature')
            fid = feature_id(feature)
            if fid in ids:
                raise ValueError(f'duplicate shore feature id: {fid}')
            ids.add(fid)
            kind = feature['properties'].get('sitka_kind')
            if kind not in valid_kinds:
                raise ValueError(f'unsupported or incorrectly categorized shore type: {kind}')
            site_ref = verify_feature_evidence(feature['properties'], sites, kind)
            evidence_ids[fid] = feature['properties']['source_feature_id']
            if site_ref is not None:
                site_links[fid] = site_ref
            if converter == 'building':
                record, height_class = building_record(feature, transform, locations, dem)
                height_classes[fid] = height_class
            else:
                record = block_record(feature, transform, locations)
            records.append((fid, record))
    if len(records) > MAX_FEATURES:
        raise ValueError('shore feature count exceeds the native runtime budget')
    records.sort(key=lambda item: item[0])
    text = 'SITKA-SHORE-1\n' + '\n'.join(row for _, row in records) + '\n'
    if len(text.encode('utf-8')) > MAX_BYTES:
        raise ValueError('shore layout exceeds the native runtime byte limit')
    derived = {
        'format': 'sitka-shore-export-1',
        'asset': output.name,
        'layout_sha256': hashlib.sha256(text.encode('utf-8')).hexdigest(),
        'terrain_sha256': manifest['mesh_sha256'],
        'dem_sha256': manifest['elevation_sha256'],
        'buildings_sha256': digest(buildings_path),
        'structures_sha256': digest(blocks_path) if blocks_path else None,
        'source_ledger_sha256': digest(ledger_path),
        'coordinate_system': str(crs),
        'east_origin': east0, 'north_origin': north0,
        'feature_count': len(records),
        'building_count': len(building_features),
        'block_count': len(block_features),
        'building_height_quality': height_classes,
        'feature_source_ids': dict(sorted(evidence_ids.items())),
        'named_site_references': dict(sorted(site_links.items())),
        'source_attribution': ledger,
        'limitations': ['P0 visualization only', 'No surveyed facade/roof or public entry',
                        'No geographic claim for user-authored block dimensions',
                        'Complex foundations and polygon holes require source repair'],
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text, encoding='utf-8')
    report = output.with_suffix('.manifest.json')
    report.write_text(json.dumps(derived, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    return derived


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--terrain-manifest', required=True, type=Path)
    p.add_argument('--dem', required=True, type=Path)
    p.add_argument('--buildings', required=True, type=Path, help='source-licensed building Polygon GeoJSON')
    p.add_argument('--blocks', type=Path, help='optional authored dock/pier/rock/bridge Point GeoJSON')
    p.add_argument('--provenance', required=True, type=Path)
    p.add_argument('--site-catalog', required=True, type=Path,
                   help='verified site/presence register; no fake historic buildings')
    p.add_argument('--output', required=True, type=Path, help='sitka/shore/waterfront.sitkash')
    a = p.parse_args()
    m = bake(a.terrain_manifest, a.dem, a.buildings, a.blocks,
             a.provenance, a.output, a.site_catalog)
    print(f"Baked {m['feature_count']} anchored shoreline features: {a.output}")


if __name__ == '__main__':
    main()
