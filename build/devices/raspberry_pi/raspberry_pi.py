#!/usr/bin/env python

import os


def install_raspberry_pi():
    # ln -sf /opt/vc/lib/libEGL.so /usr/lib/arm-linux-gnueabihf/libEGL.so
    # ln -sf /opt/vc/lib/libEGL.so /usr/lib/arm-linux-gnueabihf/libEGL.so.1
    # ln -sf /opt/vc/lib/libGLESv2.so /usr/lib/arm-linux-gnueabihf/libGLESv2.so
    # ln -sf /opt/vc/lib/libGLESv2.so /usr/lib/arm-linux-gnueabihf/libGLESv2.so.2
    os.symlink('/opt/vc/lib/libEGL.so', '/usr/lib/arm-linux-gnueabihf/libEGL.so')
    os.symlink('/opt/vc/lib/libEGL.so', '/usr/lib/arm-linux-gnueabihf/libEGL.so.1')
    os.symlink('/opt/vc/lib/libGLESv2.so', '/usr/lib/arm-linux-gnueabihf/libGLESv2.so')
    os.symlink('/opt/vc/lib/libGLESv2.so', '/usr/lib/arm-linux-gnueabihf/libGLESv2.so.2')
