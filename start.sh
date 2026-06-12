#!/bin/bash

echo "[myos] Scanning library..."
python3 services/library_scanner.py \
    --music-dir assets/sounds \
    --rom-dir   assets/roms \
    --out       assets/library.json

echo "[myos] Building..."
cd core && make -f build/Makefile run