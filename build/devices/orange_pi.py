#!/usr/bin/env python

import os
import shutil
import subprocess

from pybuild_utils.base import utils


def install_orange_pi():
    subprocess.call(['modprobe', 'mali'])
    pwd = os.getcwd()
    try:
        cloned_dir = utils.git_clone('https://github.com/linux-sunxi/sunxi-mali.git', pwd)
        os.chdir(cloned_dir)
        subprocess.call(['make', 'config'])
        subprocess.call(['make', 'install'])
        os.chdir(pwd)
        shutil.rmtree(cloned_dir)
    except Exception as ex:
        os.chdir(pwd)
        raise ex

    try:
        cloned_dir = utils.git_clone('https://github.com/fastogt/libvdpau-sunxi.git', pwd)
        os.chdir(cloned_dir)
        subprocess.call(['make', 'install'])
        os.chdir(pwd)
        shutil.rmtree(cloned_dir)
    except Exception as ex:
        os.chdir(pwd)
        raise ex

    with open('/etc/udev/rules.d/50-mali.rules', 'w') as f:
        f.write('KERNEL=="mali", MODE="0660", GROUP="video"\n'
                'KERNEL=="ump", MODE="0660", GROUP="video"')

    with open('/etc/asound.conf', 'w') as f:
        f.write('pcm.!default {\n'
                'type hw\n'
                'card 1\n'
                '}\n'
                'ctl.!default {\n'
                'type hw\n'
                'card 1\n'
                '}')

    standart_egl_path = '/usr/lib/arm-linux-gnueabihf/mesa-egl/'
    if os.path.exists(standart_egl_path):
        shutil.move(standart_egl_path, '/usr/lib/arm-linux-gnueabihf/.mesa-egl/')
