#!/bin/sh
cd Dependencies/

# See the official script at
#  https://raw.githubusercontent.com/OGRECave/ogre-next/master/Scripts/BuildScripts/output/build_ogre_linux_c%2B%2Blatest.sh
echo "Building Ogre dependencies..."
cd ogre-next-deps && mkdir -p build && cd build && cmake  -G Ninja .. || exit $?
ninja || exit $?
ninja install || exit $?

echo "Building Ogre..."
cd ../../ogre-next
if test ! -f Dependencies; then
	ln -s ../ogre-next-deps/build/ogredeps Dependencies
fi
mkdir -p build/Debug
mkdir -p build/Release
cd build/Debug
echo "--- Building Ogre (Debug) ---"
cmake -D OGRE_USE_BOOST=0 -D OGRE_CONFIG_THREAD_PROVIDER=0 -D OGRE_CONFIG_THREADS=0 -D OGRE_BUILD_COMPONENT_SCENE_FORMAT=1 -D OGRE_BUILD_SAMPLES2=1 -D OGRE_BUILD_TESTS=1 -D CMAKE_BUILD_TYPE="Debug"  -G Ninja ../.. || exit $?
ninja || exit $?
cd ../Release
echo "--- Building Ogre (Release) ---"
cmake -D OGRE_USE_BOOST=0 -D OGRE_CONFIG_THREAD_PROVIDER=0 -D OGRE_CONFIG_THREADS=0 -D OGRE_BUILD_COMPONENT_SCENE_FORMAT=1 -D OGRE_BUILD_SAMPLES2=1 -D OGRE_BUILD_TESTS=1 -D CMAKE_BUILD_TYPE="Release"  -G Ninja ../.. || exit $?
ninja || exit $?
