#!/bin/sh
if [ ! -d "build" ]; then
	mkdir build/
fi
cp CMakeLists_elevation_profiles.txt ./build/CMakeLists.txt
cd build/
cmake .
make
mv get_elevation_profiles.out ../
cd ..