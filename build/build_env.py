#!/usr/bin/env python
import sys
import urllib2
import subprocess
import os
import shutil
import platform
from pybuild_utils.base import system_info

VAAPI_ROOT = "https://www.freedesktop.org/software/vaapi/releases/"
SDL_SRC_ROOT = "https://www.libsdl.org/release/"
FFMPEG_SRC_ROOT = "http://ffmpeg.org/releases/"

ARCH_VAAPI_COMP = "bz2"
ARCH_VAAPI_EXT = "tar." + ARCH_VAAPI_COMP
ARCH_SDL_COMP = "gz"
ARCH_SDL_EXT = "tar." + ARCH_SDL_COMP
ARCH_FFMPEG_COMP = "bz2"
ARCH_FFMPEG_EXT = "tar." + ARCH_FFMPEG_COMP

def get_dist():
    """
    Return the running distribution group
    RHEL: RHEL, CENTOS, FEDORA
    DEBIAN: UBUNTU, DEBIAN
    """
    dist_name = platform.linux_distribution()[0]
    system_name = platform.system()

    if dist_name.upper() in ["RHEL", "CENTOS", "FEDORA"]:
        return "RHEL"
    elif dist_name.upper() in ["DEBIAN", "UBUNTU"]:
        return "DEBIAN"
    raise NotImplemented("Platform '%s' is not compatible with Propel" % dist_name)

def splitext(path):
    for ext in ['.tar.gz', '.tar.bz2', '.tar.xz']:
        if path.endswith(ext):
            return path[:-len(ext)]
    return os.path.splitext(path)[0]

def print_usage():
    print("Usage:\n"
        "[required] argv[1] vaapi version(1.7.3)\n"
        "[required] argv[2] sdl2 version(2.0.5)\n"
        "[required] argv[3] ffmpeg version(3.2.2)\n")

def print_message(progress, message):
    print message.message()
    sys.stdout.flush()

class BuildError(Exception):
    def __init__(self, value):
        self.value_ = value
    def __str__(self):
        return self.value_

def download_file(url):
    file_name = url.split('/')[-1]
    u = urllib2.urlopen(url)
    f = open(file_name, 'wb')
    meta = u.info()
    file_size = int(meta.getheaders("Content-Length")[0])
    print "Downloading: %s Bytes: %s" % (file_name, file_size)

    file_size_dl = 0
    block_sz = 8192
    while True:
        buffer = u.read(block_sz)
        if not buffer:
            break

        file_size_dl += len(buffer)
        f.write(buffer)
        status = r"%10d  [%3.2f%%]" % (file_size_dl, file_size_dl * 100. / file_size)
        status = status + chr(8) * (len(status) + 1)
        print status,

    f.close()
    return file_name

def extract_file(file):
    print "Extracting: {0}".format(file)
    subprocess.call(['tar', '-xvf', file])
    return splitext(file)

def build_ffmpeg(url):
    pwd = os.getcwd()
    file = download_file(url)
    folder = extract_file(file)
    os.chdir(folder)
    subprocess.call(['./configure', '--disable-xcb'])
    subprocess.call(['make', 'install'])
    os.chdir(pwd)
    shutil.rmtree(folder)

def build(dir_path):
    abs_dir_path = os.path.abspath(dir_path)
    if os.path.exists(abs_dir_path):
        shutil.rmtree(abs_dir_path)

    pwd = os.getcwd()
    os.mkdir(abs_dir_path)
    os.chdir(abs_dir_path)

    distr = get_dist()
    if distr  == 'DEBIAN':
        dep_libs = ['gcc', 'g++', 'pkg-config', 'gettext', 'libmount-dev', 'libpcre3-dev', 'yasm']
    elif distr == 'RHEL':
        dep_libs = ['gcc', 'gcc-c++', 'pkgconfig', 'gettext', 'libmount-devel', 'pcre-devel', 'yasm']

    for lib in dep_libs:
        if distr  == 'DEBIAN':
            subprocess.call(['apt-get', '-y', '--force-yes', 'install', lib])
        elif distr == 'RHEL':
            subprocess.call(['yum', '-y', 'install', lib])

    # build from sources
    source_urls = ['{0}libva/libva-{1}.{2}'.format(VAAPI_ROOT, vaapi_version, ARCH_VAAPI_EXT),
                   '{0}libva-intel-driver/libva-intel-driver-{1}.{2}'.format(VAAPI_ROOT, vaapi_version, ARCH_VAAPI_EXT),
                   '{0}SDL2-{1}.{2}'.format(SDL_SRC_ROOT, sdl_version, ARCH_SDL_EXT)
                   ]

    for url in source_urls:
        file = download_file(url)
        folder = extract_file(file)
        os.chdir(folder)
        subprocess.call(['./configure'])
        subprocess.call(['make', 'install'])
        os.chdir(abs_dir_path)
        shutil.rmtree(folder)

    build_ffmpeg('{0}ffmpeg-{1}.{2}'.format(FFMPEG_SRC_ROOT, ffmpeg_version, ARCH_FFMPEG_EXT))

if __name__ == "__main__":
    argc = len(sys.argv)

    if argc > 1:
        vaapi_version = sys.argv[1]
    else:
        print_usage()
        sys.exit(1)

    if argc > 2:
        sdl_version = sys.argv[2]
    else:
        print_usage()
        sys.exit(1)

    if argc > 3:
        ffmpeg_version = sys.argv[3]
    else:
        print_usage()
        sys.exit(1)

    platform_str = system_info.get_os()
    build('build_' + platform_str + '_env')