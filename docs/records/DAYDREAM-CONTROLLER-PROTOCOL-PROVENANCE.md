# Daydream Controller Protocol Provenance

**Status:** implementation reference  
**Recorded:** 2026-10-04

## Purpose

ReverieVR implements its Daydream controller transport independently in Java using Android Bluetooth LE APIs.

No Google VR Services binaries or proprietary Daydream runtime components are redistributed.

## Public technical reference

The implementation was developed from observable BLE behavior and the published reverse-engineering reference in:

- repository: `nullstalgia/daydream-catcher`
- reference commit: `69a324900a44295fb881574ad2a8cd5f920d3df1`
- reference document: `Writeups/Reference - Daydream (Paprika).md`

The published reference documents:

- Daydream GATT service `0000fe55-0000-1000-8000-00805f9b34fb`;
- pose characteristic `00000001-1000-1000-8000-00805f9b34fb`;
- control characteristic `00000002-1000-1000-8000-00805f9b34fb`;
- battery-voltage characteristic `00000003-1000-1000-8000-00805f9b34fb`;
- standard Battery Service / Battery Level;
- bonding requirement on later firmware;
- the complete 160-bit pose/input packet layout;
- controller recenter command value `0x00`.

## ReverieVR implementation boundary

ReverieVR does **not** copy the reference project's Rust parser or BLE implementation.

The project contains its own:

- MSB-first bit reader;
- 13-bit signed-field decoder;
- axis-angle to quaternion conversion;
- Android BLE scan/bond/GATT implementation;
- asynchronous GATT operation serialization;
- application-level controller snapshot model.

This record exists so future maintainers preserve provenance and do not accidentally replace the independent implementation with copied source whose licensing/provenance has not been reviewed.
