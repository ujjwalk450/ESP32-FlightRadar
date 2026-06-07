#!/usr/bin/env python3
"""
Generate a USER_AIRPORTS[] snippet from a CSV airport database.

Expected CSV columns:
code,name,lat,lon,type

Example:
python tools/nearest_airports_generator.py --lat 28.494106 --lon 77.137982 --csv airports.csv --limit 5 --radius 250
"""
import argparse
import csv
import math


def haversine_km(lat1, lon1, lat2, lon2):
    r = 6371.0
    p1 = math.radians(lat1)
    p2 = math.radians(lat2)
    dlat = math.radians(lat2 - lat1)
    dlon = math.radians(lon2 - lon1)
    a = math.sin(dlat / 2) ** 2 + math.cos(p1) * math.cos(p2) * math.sin(dlon / 2) ** 2
    return r * 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--lat", type=float, required=True)
    parser.add_argument("--lon", type=float, required=True)
    parser.add_argument("--csv", required=True)
    parser.add_argument("--limit", type=int, default=5)
    parser.add_argument("--radius", type=float, default=250)
    args = parser.parse_args()

    rows = []
    with open(args.csv, newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                lat = float(row["lat"])
                lon = float(row["lon"])
            except Exception:
                continue
            d = haversine_km(args.lat, args.lon, lat, lon)
            if d <= args.radius:
                rows.append((d, row, lat, lon))

    rows.sort(key=lambda x: x[0])
    rows = rows[: args.limit]

    print("static const AirportMarker USER_AIRPORTS[] = {")
    for i, (d, row, lat, lon) in enumerate(rows):
        code = row.get("code", "AP")[:4].upper()
        primary = "true" if i == 0 else "false"
        print(f'  {{ "{code}", {lat:.6f}f, {lon:.6f}f, {primary}, 0, 0 }}, // {d:.1f} km')
    print("};")
    print("static const int USER_AIRPORT_COUNT = sizeof(USER_AIRPORTS) / sizeof(USER_AIRPORTS[0]);")


if __name__ == "__main__":
    main()
