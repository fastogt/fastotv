#!/usr/bin/env python3

import os

from pyfastogt import utils


def install_raspberry_pi(gpu_mem_size: int):
    # ln -sf /opt/vc/lib/libEGL.so /usr/lib/arm-linux-gnueabihf/libEGL.so
    # ln -sf /opt/vc/lib/libEGL.so /usr/lib/arm-linux-gnueabihf/libEGL.so.1
    # ln -sf /opt/vc/lib/libGLESv2.so /usr/lib/arm-linux-gnueabihf/libGLESv2.so
    # ln -sf /opt/vc/lib/libGLESv2.so /usr/lib/arm-linux-gnueabihf/libGLESv2.so.2
    utils.symlink_force('/opt/vc/lib/libEGL.so', '/usr/lib/arm-linux-gnueabihf/libEGL.so')
    utils.symlink_force('/opt/vc/lib/libEGL.so', '/usr/lib/arm-linux-gnueabihf/libEGL.so.1')
    utils.symlink_force('/opt/vc/lib/libGLESv2.so', '/usr/lib/arm-linux-gnueabihf/libGLESv2.so')
    utils.symlink_force('/opt/vc/lib/libGLESv2.so', '/usr/lib/arm-linux-gnueabihf/libGLESv2.so.2')

    with open("/boot/config.txt", "r+") as f:
        line_found = any("gpu_mem" in line for line in f)
        if not line_found:
            f.seek(0, os.SEEK_END)
            f.write("gpu_mem={0}\n".format(gpu_mem_size))
