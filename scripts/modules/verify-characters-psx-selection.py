#!/usr/bin/env python3
"""Private, read-only validation of the six selected Red Ledger PSX character derivatives.

Diagnostic utility; does not publish any source or binary, sign an APK,
or assert that an in-headset runtime test has occurred.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

EXPECTED = {
    'rail_worker', 'night_clerk', 'militia_veteran', 'mechanic',
    'directorate_courier', 'salihorsk_hauler'
}

def check(folder):
    doc = json.loads((folder/'selection.json').read_text('utf-8'))
    assert doc.get('format') == 'RLBP-1' and doc.get('static_pose_only') is True
    entries = doc['characters']
    assert len(entries) == 6
    assert {entry['role'] for entry in entries} == EXPECTED
    actual = {p.name for p in folder.glob('*.rlprop')}
    wanted = {'patron_'+role+'.rlprop' for role in EXPECTED}
    assert actual == wanted, 'Unselected / missing RLBP assets'
    count = 0
    for entry in entries:
        name = 'patron_'+entry['role']+'.rlprop'
        assert entry['path'] == name
        raw = (folder/name).read_bytes()
        magic, version, vertices, side = struct.unpack_from('<4sIII', raw)
        assert (magic, version, side) == (b'RLBP', 1, 256)
        assert vertices > 0 and vertices <= 50000 and vertices % 3 == 0
        assert len(raw) == 16 + vertices * 40 + 256*256*2
        assert hashlib.sha256(raw).hexdigest() == entry['output_sha256']
        assert entry['triangles'] == vertices//3
        for row in struct.iter_unpack('<10f', memoryview(raw)[16:16+vertices*40]):
            assert all(math.isfinite(x) for x in row[:8])
            assert all(0 <= x <= 1 for x in row[3:8])
        assert len(set(raw[-256*256*2:])) > 4, 'Texture missing variation'
        count += vertices//3
    return {'characters':len(entries),'triangles':count,'status':'VALID PRIVATE RLBP DATA (not headset tested)'}

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('private_output',type=Path)
    print(json.dumps(check(parser.parse_args().private_output),sort_keys=True))
