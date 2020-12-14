#!/bin/sh

# Note that this will rebuild and re-link every time. You should use a build system to save time!

# Compile
clang++ -c main.cpp -g -IDependencies/ogre-next/OgreMain/include \
		-IDependencies/ogre-next/Components/Hlms/Common/include \
		-IDependencies/ogre-next/Components/Hlms/Pbs/include \
		-IDependencies/ogre-next/Components/Hlms/Unlit/include \
		-IDependencies/ogre-next/build/Debug/include \
		-IDependencies/ogre-next/Components/Overlay/include

# Link
clang++ -o ogreApp main.o -g -LDependencies/ogre-next/build/Debug/lib \
		-lOgreHlmsPbs_d -lOgreHlmsUnlit_d -lOgreMain_d -lOgreOverlay_d \
		-Wl,-rpath,.:Dependencies/ogre-next/build/Debug/lib
