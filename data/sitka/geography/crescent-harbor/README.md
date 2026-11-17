# Crescent Harbor Float 2

From shore, hold Volume Up and touch the left side of the pad to travel to the Crescent Harbor gangway. Pad up walks toward the finger floats. Pad down walks back to shore. Select takes the net bundle, signs it over alongside berth 2-4, delivers tags alongside berth 2-10 and files the receipt at the gangway. Filing pays 60 credits once, up to the existing purse cap. The same travel chord returns to the practice bay. Volume Up and pad right still opens the NOAA/USGS survey from the bay.

The centerline is 124.498 m each way. Its points are in metres relative to UTM 480000 E, 6322900 N (EPSG:32608). The gangway is municipal Docks feature 31. The connected main float and fingers are feature 38. The walker's handoffs are on the deck alongside the two moorings. The moorings themselves are the city's Slip Points features 2534 (2-4) and 2540 (2-10). All segments and their 0.30 m buffer fit inside the union of the municipal footprint polygons. The other floats appear in the harbor view. The current walking route serves Float 2.

## Geometry and provenance

The City and Borough of Sitka Harbor Department publishes the Crescent Harbor Map. Its Docks and Slip Points layers supplied the plan positions. The archived JSON holds seven exterior dock rings rounded to the nearest millimetre, two slip points, the source queries and SHA-256 digests of their full ArcGIS responses. The source service supplied EPSG:32608 through its `outSR` parameter. The city data item was last modified in September 2024. [Municipal map](https://www.arcgis.com/home/item.html?id=0498ca4122b04d0bb9907566cbc0a1c7) · [City GIS data service](https://services7.arcgis.com/EozEvrS4g3SEhtG3/arcgis/rest/services/Harbor_Data/FeatureServer) · [Acquisition record](municipal-docks.json)

Run `python3 scripts/sitka/build_crescent_deck.py` with Shapely 2.1 to reproduce the checked-in 94,214-byte mesh from the archived footprint. The standard APK build uses the checked-in mesh and verifies its SHA-256. The native module also checks its CRC, and the existing NOAA/USGS chart grid identifies the local coordinate frame. The seven features yield 2,536 triangles including float skirts. The deck height, trim, water surface, two boats and delivery are authored for play. Geographic boating, additional casts and tide-dependent depths remain open.

The V7 journal stores progress, position along the deck and the current location. V5 and V6 journals import into the same slot. The native geographic EGL test walks both directions, renders both eyes, reopens midway, and retries a failed settlement save.

## Fishing from Float 2

A blue mark between the two delivery stops identifies the mapped cast. Select there to cast, select again on the bite and hold to reel. Releasing the button eases tension. Volume Up cancels. Landed catches use the normal journal, inventory and sale price at the practice harbor office. A paid delivery points the walker back to the mark. The cast is also available during the delivery.

The float lands on one NOAA sounding at 57.0501578° N, 135.3286259° W, 3.9 m below mean lower low water. Its projected position is 480063.584 E, 6323017.253 N. The adjoining NOAA depth area spans 3.6–5.4 m. The game uses the sounding for this specific cast. It never extends the measurement to neighboring cells or adds it to the independent USGS terrain elevations. The chart cell credits a January 2011 source. The fish mix and bites are the game's simulation. Neither dock access rights nor real-world fishing permission or safety are established by these map features.

The host controller replay cancels one cast, casts again, waits for a fresh press on the bite, reels, recovers a failed write and reopens the caught fish in the shared creel. It then leaves the float, walks to the practice office and sells the fish through the harbor menu. [NOAA sounding and chart-cell receipt](float2-sounding.json)
