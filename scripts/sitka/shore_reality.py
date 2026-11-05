#!/usr/bin/env python3
"""Guard against inventing present-day Sitka features from historic names.

This is an offline authoring check, not a network fetch or a geometry source.
A catalog's existence evidence cannot be promoted to surveyed geometry.
"""
import json
import re
from pathlib import Path
from urllib.parse import urlparse

_ALLOWED_PRESENCE = frozenset(
    ("existing", "demolished", "relocated", "needs_current_verification"))
_ALLOWED_GEOMETRY = frozenset(("not_acquired", "source_checked"))
_ID = re.compile(r"[a-z0-9_]{3,64}\Z")
_SOURCE = re.compile(r"[A-Za-z0-9_:./-]{3,128}\Z")


def load_site_catalog(path):
    source = json.loads(Path(path).read_text(encoding="utf-8"))
    if source.get("format") != "sitka-shore-reality-1":
        raise ValueError("Sitka shore reality register version mismatch")
    sites = source.get("sites")
    if not isinstance(sites, list) or not sites:
        raise ValueError("Sitka shore reality register has no sites")
    result = {}
    for item in sites:
        if not isinstance(item, dict):
            raise ValueError("Malformed Sitka shore site record")
        key = item.get("id")
        if not isinstance(key, str) or not _ID.fullmatch(key) or key in result:
            raise ValueError("Duplicate or invalid Sitka shore site ID")
        if item.get("presence") not in _ALLOWED_PRESENCE or (
                item.get("geometry_state") not in _ALLOWED_GEOMETRY):
            raise ValueError("Sitka site presence/geometry state invalid")
        if not isinstance(item.get("name"), str) or not item["name"].strip():
            raise ValueError("Unnamed Sitka shore reference")
        urls = item.get("references")
        if not isinstance(urls, list) or not urls or any(
            not isinstance(u, str) or urlparse(u).scheme != "https"
            or not urlparse(u).hostname for u in urls
        ):
            raise ValueError("Sitka shore reference requires https evidence")
        if item["geometry_state"] == "source_checked":
            evidence = item.get("geometry_evidence")
            if not isinstance(evidence, dict) or not (
                isinstance(evidence.get("source_feature_id"), str)
                and _SOURCE.fullmatch(evidence["source_feature_id"])
                and isinstance(evidence.get("source_checksum_sha256"), str)
                and re.fullmatch(r"[0-9a-f]{64}", evidence["source_checksum_sha256"])
                and isinstance(evidence.get("crs"), str) and evidence["crs"].strip()
            ):
                raise ValueError("Source-checked geometry lacks CRS and hash")
        result[key] = item
    return result


def verify_feature_evidence(properties, sites, kind):
    """Reject anonymous/fictive placements and false historic reconstructions."""
    if not isinstance(properties, dict):
        raise ValueError("Shore feature properties missing")
    src = properties.get("source_feature_id")
    if not isinstance(src, str) or not _SOURCE.fullmatch(src) or (
            ".." in src or "//" in src):
        raise ValueError("Each shore feature needs a real source_feature_id")
    if properties.get("physical_status") != "existing":
        raise ValueError("No demolished, relocated or unverified shore object")
    ref = properties.get("site_ref")
    if ref is not None:
        if not isinstance(ref, str) or ref not in sites:
            raise ValueError("Unknown named Sitka shoreline site")
        site = sites[ref]
        if site["presence"] != "existing":
            raise ValueError("Historical/relocated/unconfirmed site cannot be rendered")
        if site["geometry_state"] != "source_checked":
            raise ValueError("Named site has no licensed verified footprint")
        if src != site["geometry_evidence"]["source_feature_id"]:
            raise ValueError("Shore geometry source ID does not match named site")
    elif kind in ("cannery", "heritage", "bridge_deck", "bridge_ramp",
                  "harbor_shack"):
        raise ValueError("Distinctive shoreline structure requires site_ref")
    return ref
