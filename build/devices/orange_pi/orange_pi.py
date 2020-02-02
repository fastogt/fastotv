#!/usr/bin/env python

import os
import shutil
import subprocess

from pyfastogt import utils

LIRC_FOLDER = '/etc/lirc/'


def install_orange_pi_h3():
    subprocess.call(['modprobe', 'mali'])
    pwd = os.getcwd()
    script_dir = os.path.dirname(os.path.realpath(__file__))
    try:
        cloned_dir = utils.git_clone('https://github.com/linux-sunxi/sunxi-mali.git')
        os.chdir(cloned_dir)
        subprocess.call(['make', 'config'])
        subprocess.call(['make', 'install'])
        os.chdir(pwd)
        shutil.rmtree(cloned_dir)
    except Exception as ex:
        os.chdir(pwd)
        raise ex

    try:
        cloned_dir = utils.git_clone('https://github.com/fastogt/libvdpau-sunxi.git')
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

    lirc_conf_path = os.path.join(script_dir, 'hardware/lirc/hardware.conf')
    if os.path.exists(LIRC_FOLDER):
        dst = os.path.join(LIRC_FOLDER, 'hardware.conf')
        shutil.copy(lirc_conf_path, dst)


def install_orange_pi_h5():
    script_dir = os.path.dirname(os.path.realpath(__file__))

    with open('/etc/asound.conf', 'w') as f:
        f.write('pcm.!default {\n'
                'type hw\n'
                'card 1\n'
                '}\n'
                'ctl.!default {\n'
                'type hw\n'
                'card 1\n'
                '}')

    lirc_conf_path = os.path.join(script_dir, 'hardware/lirc/hardware.conf')
    if os.path.exists(LIRC_FOLDER):
        dst = os.path.join(LIRC_FOLDER, 'hardware.conf')
        shutil.copy(lirc_conf_path, dst)
