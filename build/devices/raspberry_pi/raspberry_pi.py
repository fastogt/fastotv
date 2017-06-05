#!/usr/bin/env python

from pybuild_utils.base import utils


def install_raspberry_pi():
    # ln -sf /opt/vc/lib/libEGL.so /usr/lib/arm-linux-gnueabihf/libEGL.so
    # ln -sf /opt/vc/lib/libEGL.so /usr/lib/arm-linux-gnueabihf/libEGL.so.1
    # ln -sf /opt/vc/lib/libGLESv2.so /usr/lib/arm-linux-gnueabihf/libGLESv2.so
    # ln -sf /opt/vc/lib/libGLESv2.so /usr/lib/arm-linux-gnueabihf/libGLESv2.so.2
    utils.symlink_force('/opt/vc/lib/libEGL.so', '/usr/lib/arm-linux-gnueabihf/libEGL.so')
    utils.symlink_force('/opt/vc/lib/libEGL.so', '/usr/lib/arm-linux-gnueabihf/libEGL.so.1')
    utils.symlink_force('/opt/vc/lib/libGLESv2.so', '/usr/lib/arm-linux-gnueabihf/libGLESv2.so')
    utils.symlink_force('/opt/vc/lib/libGLESv2.so', '/usr/lib/arm-linux-gnueabihf/libGLESv2.so.2')
