#!/usr/bin/env python3
"""Decode the owner's *specific* UE5.7 Between Deliveries DT_ShopItems.

Read-only; hashes guard against guessing offsets in a different UE package.
Emits metadata/Unreal object references, NEVER image or mesh bytes.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys

SHA = '06440a1dafa96234140a585185ec3d5159d4e3fd59a8695a5121e8ba3824e62c'


def recover(data):
    if hashlib.sha256(data).hexdigest() != SHA or data[:4] != b'\xc1\x83\x2a\x9e':
        raise ValueError('Unrecognized or incomplete original Unreal shop table')

    def number(at):
        if at < 0 or at + 4 > len(data):
            raise ValueError('Out-of-bounds package field')
        return struct.unpack_from('<i', data, at)[0]

    names, at = [], 408
    for _ in range(159):
        count = number(at)
        if not 2 <= count <= 256 or at + count + 8 > len(data):
            raise ValueError('Invalid name table')
        raw = data[at + 4:at + 4 + count]
        if raw[-1] != 0:
            raise ValueError('Unterminated Unreal name')
        names.append(raw[:-1].decode('utf-8'))
        at += count + 8

    def name(at):
        idx = number(at)
        if not 0 <= idx < len(names) or number(at + 4) != 0:
            raise ValueError('Invalid FName reference')
        return names[idx]

    imports = []
    for index in range(105):
        at = 5789 + 40 * index
        imports.append((name(at + 8), number(at + 16), name(at + 20)))

    def imported(idx, klass):
        if idx >= 0 or -idx > len(imports):
            raise ValueError('Invalid UObject reference')
        class_name, outer, object_name = imports[-idx - 1]
        if class_name != klass or outer >= 0 or -outer > len(imports):
            raise ValueError('UObject class or outer is not valid')
        parent_class, parent_outer, package = imports[-outer - 1]
        if parent_class != 'Package' or parent_outer != 0 or not package.startswith('/Game/'):
            raise ValueError('Unsupported UObject package layout')
        return package + '.' + object_name

    marker = struct.pack('<4i', 35, 0, 45, 0)  # ItemName / StrProperty
    rows, scan = [], 0
    while (at := data.find(marker, scan)) != -1:
        scan = at + len(marker)
        if at < 8 or name(at - 8) not in [str(x) for x in range(1, 25)] + ['test']:
            continue
        key, fields, cursor = name(at - 8), {}, at
        while name(cursor) != 'None':
            field, typ = name(cursor).split('_')[0], name(cursor + 8)
            size, payload = number(cursor + 20), cursor + 25
            if number(cursor + 16) != 0 or data[cursor + 24] != 0 or not 0 <= size <= 4096:
                raise ValueError('Unsupported FPropertyTag layout')
            raw = data[payload:payload + size]
            if len(raw) != size:
                raise ValueError('Truncated field')
            if typ == 'StrProperty':
                length = number(payload)
                if length < 1 or length + 4 != size or raw[-1] != 0:
                    raise ValueError('Invalid string field')
                value = raw[4:-1].decode('utf-8')
            elif typ == 'DoubleProperty' and size == 8:
                value = struct.unpack('<d', raw)[0]
            elif typ in ('IntProperty', 'ObjectProperty') and size == 4:
                value = number(payload)
            else:
                raise ValueError('Unexpected property type')
            if field in fields or len(fields) >= 7:
                raise ValueError('Duplicate/extra property')
            fields[field] = value
            cursor = payload + size
        if set(fields) != {'ItemName', 'Supplier', 'BuyPrice', 'SalePrice', 'Item', 'Image', 'ItemCategory'}:
            raise ValueError('Unexpected shop fields')
        rows.append({
            'unreal_row': key, 'source_name': fields['ItemName'],
            'supplier_id': fields['Supplier'], 'source_category': fields['ItemCategory'],
            'buy_price_source': fields['BuyPrice'], 'sale_price_source': fields['SalePrice'],
            'unreal_mesh_object': imported(fields['Item'], 'StaticMesh'),
            'unreal_icon_object': imported(fields['Image'], 'Texture2D'),
        })
    numbered = sorted((r for r in rows if r['unreal_row'] != 'test'),
                      key=lambda r: int(r['unreal_row']))
    if len(rows) != 25 or len(numbered) != 24 or len({r['unreal_row'] for r in rows}) != 25:
        raise ValueError('Missing/duplicate shop rows')
    for pos, product in enumerate(numbered):
        if product['unreal_row'] != str(pos + 1) or product['supplier_id'] != ((pos // 4 + 1) * 101):
            raise ValueError('Incorrect row or supplier order')
        if not 0 < product['buy_price_source'] < product['sale_price_source'] <= 100:
            raise ValueError('Invalid source buy/sale price')
    if any(len(set(row[key] for row in numbered)) != 24 for key in
           ('unreal_mesh_object', 'unreal_icon_object')):
        raise ValueError('Nonunique model or icon mapping')
    return {'source_sha256': SHA, 'source_rows': 25,
            'excluded_test_row': True, 'products': numbered}


if __name__ == '__main__':
    if len(sys.argv) != 3:
        raise SystemExit('Usage: recover_shop_table.py ORIGINAL_DT_ShopItems.uasset output.json')
    result = recover(Path(sys.argv[1]).read_bytes())
    Path(sys.argv[2]).write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(f"Verified {len(result['products'])} original shop records")
