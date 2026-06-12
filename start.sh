#!/bin/bash
 
echo "[myos] Scanning library..."
python3 services/library_scanner.py
 
echo "[myos] Building..."
cd core && make -f build/Makefile run