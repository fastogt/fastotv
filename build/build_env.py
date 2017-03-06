#!/usr/bin/env python
import sys
from urllib.request import urlopen
import subprocess
import os
import shutil
import platform
from pybuild_utils.base import system_info, utils

# defines
CMAKE_SRC_PATH = "https://cmake.org/files/v3.7/cmake-3.7.2.tar.gz"
SDL_SRC_ROOT = "https://www.libsdl.org/release/"
FFMPEG_SRC_ROOT = "http://ffmpeg.org/releases/"

ARCH_SDL_COMP = "gz"
ARCH_SDL_EXT = "tar." + ARCH_SDL_COMP
ARCH_FFMPEG_COMP = "bz2"
ARCH_FFMPEG_EXT = "tar." + ARCH_FFMPEG_COMP


def linux_get_dist():
    """
    Return the running distribution group
    RHEL: RHEL, CENTOS, FEDORA
    DEBIAN: UBUNTU, DEBIAN
    """
    linux_tuple = platform.linux_distribution()
    dist_name = linux_tuple[0]
    dist_name_upper = dist_name.upper()

    if dist_name_upper in ["RHEL", "CENTOS", "FEDORA"]:
        return "RHEL"
    elif dist_name_upper in ["DEBIAN", "UBUNTU"]:
        return "DEBIAN"
    raise NotImplemented("Unknown platform '%s'" % dist_name)


def splitext(path):
    for ext in ['.tar.gz', '.tar.bz2', '.tar.xz']:
        if path.endswith(ext):
            return path[:-len(ext)]
    return os.path.splitext(path)[0]


def print_usage():
    print("Usage:\n"
          "[required] argv[1] sdl2 version(2.0.5)\n"
          "[required] argv[2] ffmpeg version(3.2.4)\n"
          "[optional] argv[3] platform\n"
          "[optional] argv[4] architecture\n"
          "[optional] argv[5] prefix path\n")


def print_message(progress, message):
    print(message.message())
    sys.stdout.flush()


def download_file(url):
    file_name = url.split('/')[-1]
    responce = urlopen(url)
    if responce.status != 200:
        raise utils.BuildError(
            "Can't fetch url: %s, status: %s, responce: %s" % (url, responce.status, responce.reason))

    f = open(file_name, 'wb')
    file_size = 0
    header = responce.getheader("Content-Length")
    if header:
        file_size = int(header)

    print("Downloading: %s Bytes: %s" % (file_name, file_size))

    file_size_dl = 0
    block_sz = 8192
    while True:
        buffer = responce.read(block_sz)
        if not buffer:
            break

        file_size_dl += len(buffer)
        f.write(buffer)
        percent = 0 if not file_size else file_size_dl * 100. / file_size
        status = r"%10d  [%3.2f%%]" % (file_size_dl, percent)
        status += chr(8) * (len(status) + 1)
        print(status, end='/r')

    f.close()
    return file_name


def extract_file(file):
    print("Extracting: {0}".format(file))
    subprocess.call(['tar', '-xvf', file])
    return splitext(file)


def build_ffmpeg(url, prefix_path):
    pwd = os.getcwd()
    file = download_file(url)
    folder = extract_file(file)
    os.chdir(folder)
    subprocess.call(['./configure', '--prefix={0}'.format(prefix_path), '--disable-libxcb'])
    subprocess.call(['make', 'install'])
    os.chdir(pwd)
    shutil.rmtree(folder)


def git_clone(url):
    pwd = os.getcwd()
    common_git_clone_line = ['git', 'clone', url]
    cloned_dir = os.path.splitext(url.rsplit('/', 1)[-1])[0]
    common_git_clone_line.append(cloned_dir)
    subprocess.call(common_git_clone_line)
    os.chdir(cloned_dir)

    common_git_clone_init_line = ['git', 'submodule', 'update', '--init', '--recursive']
    subprocess.call(common_git_clone_init_line)
    os.chdir(pwd)
    return os.path.join(pwd, cloned_dir)


def build_from_sources(url, prefix_path):
    pwd = os.getcwd()
    file = download_file(url)
    folder = extract_file(file)
    os.chdir(folder)
    subprocess.call(['./configure', '--prefix={0}'.format(prefix_path)])
    subprocess.call(['make', 'install'])
    os.chdir(pwd)
    shutil.rmtree(folder)


class BuildRequest(object):
    def __init__(self, platform, arch_bit):
        platform_or_none = system_info.get_supported_platform_by_name(platform)

        if not platform_or_none:
            raise utils.BuildError('invalid platform')

        arch = platform_or_none.architecture_by_bit(arch_bit)
        if not arch:
            raise utils.BuildError('invalid arch')

        self.platform_ = system_info.Platform(platform_or_none.name(), arch, platform_or_none.package_types())
        print("Build request for platform: {0}, arch: {1} created".format(platform, arch.name()))

    def build(self, dir_path, prefix_path):
        cmake_project_root_abs_path = '..'
        if not os.path.exists(cmake_project_root_abs_path):
            raise utils.BuildError('invalid cmake_project_root_path: %s' % cmake_project_root_abs_path)

        if not prefix_path:
            prefix_path = self.platform_.arch().default_install_prefix_path()

        abs_dir_path = os.path.abspath(dir_path)
        if os.path.exists(abs_dir_path):
            shutil.rmtree(abs_dir_path)

        os.mkdir(abs_dir_path)
        os.chdir(abs_dir_path)

        platform_name = self.platform_.name()
        arch = self.platform_.arch()
        dep_libs = []

        if platform_name == 'linux':
            distribution = linux_get_dist()
            if distribution == 'DEBIAN':
                dep_libs = ['gcc', 'g++', 'yasm', 'pkg-config', 'libtool',
                            'libz-dev', 'libbz2-dev', 'libpcre3-dev',
                            'libasound2-dev',
                            'libx11-dev',
                            'libdrm-dev', 'libdri2-dev', 'libump-dev',
                            'xorg-dev', 'xutils-dev', 'xserver-xorg', 'xinit']
            elif distribution == 'RHEL':
                dep_libs = ['gcc', 'gcc-c++', 'yasm', 'pkgconfig', 'libtoolize',
                            'libz-devel', 'libbz2-devel', 'pcre-devel',
                            'libasound2-dev',
                            'libx11-devel',
                            'libdrm-devel', 'libdri2-devel', 'libump-devel',
                            'xorg-x11-server-devel', 'xserver-xorg', 'xinit']

            for lib in dep_libs:
                if distribution == 'DEBIAN':
                    subprocess.call(['apt-get', '-y', '--force-yes', 'install', lib])
                elif distribution == 'RHEL':
                    subprocess.call(['yum', '-y', 'install', lib])
            build_from_sources(CMAKE_SRC_PATH, prefix_path)
        elif platform_name == 'windows':
            if arch.bit() == 64:
                dep_libs = ['mingw-w64-x86_64-toolchain', 'mingw-w64-x86_64-yasm']
            elif arch.bit() == 32:
                dep_libs = ['mingw-w64-i686-toolchain', 'mingw-w64-i686-yasm']

            for lib in dep_libs:
                subprocess.call(['pacman', '-S', lib])

        # project static options
        prefix_args = '-DCMAKE_INSTALL_PREFIX={0}'.format(prefix_path)

        cmake_line = ['cmake', cmake_project_root_abs_path, '-GUnix Makefiles', '-DCMAKE_BUILD_TYPE=RELEASE',
                      prefix_args]
        try:
            cloned_dir = git_clone('https://github.com/fastogt/common.git')
            os.chdir(cloned_dir)

            os.mkdir('build_cmake_release')
            os.chdir('build_cmake_release')
            common_cmake_line = list(cmake_line)
            common_cmake_line.append('-DQT_ENABLED=OFF')
            subprocess.call(common_cmake_line)
            subprocess.call(['make', 'install'])
            os.chdir(abs_dir_path)
            shutil.rmtree(cloned_dir)
        except Exception as ex:
            os.chdir(abs_dir_path)
            raise ex

        # build from sources
        source_urls = ['{0}SDL2-{1}.{2}'.format(SDL_SRC_ROOT, sdl_version, ARCH_SDL_EXT)]
        for url in source_urls:
            build_from_sources(url, abs_dir_path, prefix_path)

        build_ffmpeg('{0}ffmpeg-{1}.{2}'.format(FFMPEG_SRC_ROOT, ffmpeg_version, ARCH_FFMPEG_EXT), prefix_path)


if __name__ == "__main__":
    argc = len(sys.argv)

    if argc > 1:
        sdl_version = sys.argv[1]
    else:
        print_usage()
        sys.exit(1)

    if argc > 2:
        ffmpeg_version = sys.argv[2]
    else:
        print_usage()
        sys.exit(1)

    if argc > 3:
        platform_str = sys.argv[3]
    else:
        platform_str = system_info.get_os()

    if argc > 4:
        arch_bit_str = sys.argv[4]
    else:
        arch_bit_str = system_info.get_arch_bit()

    if argc > 5:
        prefix_path = sys.argv[5]
    else:
        prefix_path = None

    request = BuildRequest(platform_str, int(arch_bit_str))
    request.build('build_' + platform_str + '_env', prefix_path)
