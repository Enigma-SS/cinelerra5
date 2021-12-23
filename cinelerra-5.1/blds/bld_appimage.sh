#!/bin/bash

# this should bascically be the same as a static build, but the 
# "make install" is followed by the appimage creation. After the
# install does its work, the bin directory and subdirectories 
# contain the whole application.

# Install linuxdeploy (CLI version). Download from
# https://github.com/linuxdeploy/linuxdeploy/releases/continuous . It is an
# AppImage, best to use a directory bin under your home directory for all
# AppImages, because at log-in time this is put in the path if it exists.


( ./autogen.sh
  ./configure --with-single-user --with-booby --enable-static-build
  make && make install ) 2>&1 | tee log
mv Makefile Makefile.cfg
cp Makefile.devel Makefile

mkdir AppDir                # create lowest level
mkdir AppDir/usr

cp -r bin AppDir/usr/    # copy whole of bin directory

# We need to specify all executables, so linuxdeploy can pick up dependencies.
# Any executable code in other places in not picked up (yet).
linuxdeploy-x86_64.AppImage --appdir=AppDir -o appimage -d image/cin.desktop -i image/cin.svg -e bin/cin -e bin/mpeg2enc -e bin/mplex -e bin/hveg2enc -e bin/lv2ui -e bin/bdwrite -e bin/zmpeg3toc -e bin/zmpeg3show -e bin/zmpeg3cat -e bin/zmpeg3ifochk -e bin/zmpeg3cc2txt -e bin/mplexlo 2>&1 | tee appimage.log

# There is now an appimage in the cinelerra-5.1 directory. 
