#!/usr/bin/env bash
#
# Script to clone external packages required by the VC-5 software

# Switch to enable cleaning instead of cloning passed on the command line?
if [ "$1" == "--clean" ]; then
	packages=("argparse" "libexpat" "mxml" "iotivity" "uthash" "ya_getopt")
	#echo "Cleaning the external packages ..."

	for item in ${packages[*]}; do
		echo "Removing $item"
		rm -rf $item
	done

	#echo "Done"

	exit 0
fi

argparse=https://github.com/cofyc/argparse
expat=https://github.com/libexpat/libexpat
mxml=https://github.com/michaelrsweet/mxml
#iotivity=https://github.com/iotivity/iotivity
ya_getopt=https://github.com/kubo/ya_getopt.git
uthash=https://github.com/troydhanson/uthash

for url in ${argparse} ${expat} ${mxml} ${ya_getopt} ${uthash}; do
	# Clone the repository if it does not exist in the current directory
	#echo "git clone ${url}"
	[ -d $(basename ${url}) ] || git clone ${url}
done

# Build the packages that use CMake
for dir in argparse; do
	if [ -d ${dir} ]; then
		echo "Building ${dir}"
		pushd ${dir}
		[ -d build ] || mkdir build
		cd build
		cmake ..
		make
		popd
	fi
done

# Expat build scripts are in a subdirectory of the cloned repository
for dir in libexpat; do
	if [ -d ${dir} ]; then
		echo "Building ${dir}"
		pushd ${dir}/expat
		[ -d build ] || mkdir build
		cd build
		cmake ..
		make
		popd
	fi
done

# Build the packages that use Autoconf
for dir in mxml; do
	if [ -d ${dir} ]; then
		echo "Building ${dir}"
		pushd ${dir}
		./configure
		make
		popd
	fi
done

# Define the locations of the dynamic libraries
#EXTERNAL=`pwd`
#export DYLD_LIBRARY_PATH=${EXTERNAL}/argparse/build:${EXTERNAL}/libexpat/expat/build:${EXTERNAL}/mxml:${DYLD_LIBRARY_PATH}

