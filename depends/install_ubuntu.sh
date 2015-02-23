#/bin/sh

set -e

cd `dirname $0`
DEPENDS_DIR=`pwd`

# Ubuntu debian dependencies
sudo apt-get install libusb-dev libturbojpeg opencl-headers libudev-dev libtool autoconf automake libglew-dev

sh ./install_deps.sh
