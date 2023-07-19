#!/bin/bash
cp -f /sdcard/code/MEC/build/release/lib/* /sdcard/code/Orac/Organelle/orac/externals/
cd /sdcard/code/Orac
./scripts/create_organelle.sh
cp -f ./pkg/organelle/orac.zop /sdcard/Patches/
