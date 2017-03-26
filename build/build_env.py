#!/usr/bin/env python
import sys
from urllib.request import urlopen
import subprocess
import os
import shutil
import platform
import argparse
from pybuild_utils.base import system_info, utils

# Script for building enviroment on clean machine

# Known issues
# For windows 32 please specify architecture 32

# defines
CMAKE_SRC_ROOT = "https://cmake.org/files/"
SDL_SRC_ROOT = "https://www.libsdl.org/release/"
FFMPEG_SRC_ROOT = "http://ffmpeg.org/releases/"
PNG_SRC_ROOT = "https://downloads.sourceforge.net/project/libpng/libpng16/"

ARCH_CMAKE_COMP = "gz"
ARCH_CMAKE_EXT = "tar." + ARCH_CMAKE_COMP
ARCH_PNG_COMP = "xz"
ARCH_PNG_EXT = "tar." + ARCH_PNG_COMP
ARCH_SDL_COMP = "gz"
ARCH_SDL_EXT = "tar." + ARCH_SDL_COMP
ARCH_FFMPEG_COMP = "bz2"
ARCH_FFMPEG_EXT = "tar." + ARCH_FFMPEG_COMP


def splitext(path):
    for ext in ['.tar.gz', '.tar.bz2', '.tar.xz']:
        if path.endswith(ext):
            return path[:-len(ext)]
    return os.path.splitext(path)[0]


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
        print(status, end='\r')

        f.close()
        return file_name


def extract_file(file):
    print("Extracting: {0}".format(file))
    subprocess.call(['tar', '-xvf', file])
    return splitext(file)


def build_ffmpeg(url, prefix_path, other_args):
    pwd = os.getcwd()
    file = download_file(url)
    folder = extract_file(file)
    os.chdir(folder)
    ffmpeg_cmd = ['./configure', '--prefix={0}'.format(prefix_path)]
    ffmpeg_cmd.extend(other_args)
    subprocess.call(ffmpeg_cmd)
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


def build_from_sources(url, prefix_path, other_args):
    pwd = os.getcwd()
    file = download_file(url)
    folder = extract_file(file)
    os.chdir(folder)
    source_build_cmd = ['./configure', '--prefix={0}'.format(prefix_path)]
    source_build_cmd.extend(other_args)
    subprocess.call(source_build_cmd)
    subprocess.call(['make', 'install'])
    os.chdir(pwd)
    shutil.rmtree(folder)


def install_orange_pi():
    subprocess.call(['modprobe', 'mali'])
    pwd = os.getcwd()
    try:
        cloned_dir = git_clone('https://github.com/linux-sunxi/sunxi-mali.git')
        os.chdir(cloned_dir)
        subprocess.call(['make', 'config'])
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
        f.write('pcm.!default {'
                'type hw'
                'card 1'
                '}\n'
                'ctl.!default {'
                'type hw'
                'card 1'
                '}')

    shutil.move('/usr/lib/arm-linux-gnueabihf/mesa-egl/', '/usr/lib/arm-linux-gnueabihf/.mesa-egl/')
    #os.symlink('/usr/lib/libMali.so', '/usr/lib/libGLESv2.so')


class SupportedDevice(object):
    def __init__(self, name, system_libs, sdl2_flags, install_specific=None):
        self.name_ = name
        self.system_libs_ = system_libs
        self.sdl2_flags_ = sdl2_flags
        self.install_specific_ = install_specific

    def name(self):
        return self.name_

    def sdl2_flags(self):
        return self.sdl2_flags_

    def system_libs(self, platform):
        return self.system_libs_

    def install_specific(self):
        if self.install_specific_:
            return self.install_specific_()


SUPPORTED_DEVICES = [SupportedDevice('pc', [], []),
                     SupportedDevice('orange-pi', ['libgles2-mesa-dev', 'xserver-xorg-video-fbturbo'],
                                     ['--disable-video-opengl', '--disable-video-opengles1',
                                      '--enable-video-opengles2'], install_orange_pi)]


def get_device():
    return SUPPORTED_DEVICES[0]


def get_supported_device_by_name(name):
    return next((x for x in SUPPORTED_DEVICES if x.name() == name), None)


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


class BuildRequest(object):
    def __init__(self, device, platform, arch_name, dir_path, prefix_path):
        platform_or_none = system_info.get_supported_platform_by_name(platform)

        if not platform_or_none:
            raise utils.BuildError('invalid platform')

        build_arch = platform_or_none.architecture_by_arch_name(arch_name)
        if not build_arch:
            raise utils.BuildError('invalid arch')

        if not prefix_path:
            prefix_path = build_arch.default_install_prefix_path()

        build_platform = system_info.Platform(platform_or_none.name(), build_arch, platform_or_none.package_types())

        self.device_ = device
        self.platform_ = build_platform
        build_dir_path = os.path.abspath(dir_path)
        if os.path.exists(build_dir_path):
            shutil.rmtree(build_dir_path)

        os.mkdir(build_dir_path)
        os.chdir(build_dir_path)

        self.build_dir_path_ = build_dir_path
        self.prefix_path_ = prefix_path
        print("Build request for device: {0}, platform: {1}({2}) created".format(device.name(), build_platform.name(),
                                                                                 build_arch.name()))

    def install_device_specific(self):
        device.install_specific()

    def install_system(self):
        platform_name = self.platform_.name()
        arch = self.platform_.arch()
        device = self.device_

        if platform_name == 'linux':
            distribution = linux_get_dist()
            if distribution == 'DEBIAN':
                dep_libs = ['git', 'gcc', 'g++', 'yasm', 'ninja-build', 'pkg-config', 'libtool', 'rpm', 'make',
                            'libz-dev', 'libbz2-dev', 'libpcre3-dev',
                            'libasound2-dev',
                            'libx11-dev',
                            'libdrm-dev', 'libdri2-dev', 'libump-dev',
                            'xorg-dev', 'xutils-dev', 'xserver-xorg', 'xinit']
            elif distribution == 'RHEL':
                dep_libs = ['git', 'gcc', 'gcc-c++', 'yasm', 'ninja-build', 'pkgconfig', 'libtoolize', 'rpm-build',
                            'make',
                            'zlib-dev', 'bzip2-devel', 'pcre-devel',
                            'alsa-lib-devel',
                            'libx11-devel',
                            'libdrm-devel', 'libdri2-devel', 'libump-devel',
                            'xorg-x11-server-devel', 'xserver-xorg', 'xinit']

            dep_libs.extend(device.system_libs(platform_name))

            for lib in dep_libs:
                if distribution == 'DEBIAN':
                    subprocess.call(['apt-get', '-y', '--force-yes', 'install', lib])
                elif distribution == 'RHEL':
                    subprocess.call(['yum', '-y', 'install', lib])

            if distribution == 'RHEL':
                subprocess.call(['ln', '-sf', '/usr/bin/ninja-build', '/usr/bin/ninja'])
        elif platform_name == 'windows':
            if arch.name() == 'x86_64':
                dep_libs = ['git', 'mingw-w64-x86_64-gcc', 'mingw-w64-x86_64-yasm',
                            'mingw-w64-x86_64-make', 'mingw-w64-x86_64-ninja']
            elif arch.name() == 'i386':
                dep_libs = ['git', 'mingw-w64-i686-gcc', 'mingw-w64-i686-yasm',
                            'mingw-w64-i686-make', 'mingw-w64-i686-ninja']

            for lib in dep_libs:
                subprocess.call(['pacman', '-SYq', lib])
        elif platform_name == 'macosx':
            dep_libs = ['git', 'yasm', 'make', 'ninja']

            for lib in dep_libs:
                subprocess.call(['port', 'install', lib])

    def build_ffmpeg(self, ffmpeg_version):
        ffmpeg_platform_args = ['--disable-doc',
                                '--disable-opencl',
                                '--disable-lzma', '--disable-iconv',
                                '--disable-shared', '--enable-static',
                                '--disable-debug', '--disable-ffserver',
                                '--extra-version=static']  # '--extra-cflags=--static'
        platform_name = self.platform_.name()
        if platform_name == 'linux':
            ffmpeg_platform_args.extend(['--disable-libxcb'])
        elif platform_name == 'windows':
            ffmpeg_platform_args = ffmpeg_platform_args
        elif platform_name == 'macosx':
            ffmpeg_platform_args.extend(['--cc=clang', '--cxx=clang++'])

        build_ffmpeg('{0}ffmpeg-{1}.{2}'.format(FFMPEG_SRC_ROOT, ffmpeg_version, ARCH_FFMPEG_EXT),
                     self.prefix_path_, ffmpeg_platform_args)

    def build_sdl2(self, version):
        device = self.device_
        build_from_sources('{0}SDL2-{1}.{2}'.format(SDL_SRC_ROOT, version, ARCH_SDL_EXT), self.prefix_path_,
                           device.sdl2_flags())

    def build_libpng(self, version):
        build_from_sources('{0}{1}/libpng-{1}.{2}'.format(PNG_SRC_ROOT, version, ARCH_PNG_EXT), self.prefix_path_,
                           [])

    def build_cmake(self, version):
        stabled_version_array = version.split(".")
        stabled_version = 'v{0}.{1}'.format(stabled_version_array[0], stabled_version_array[1])
        build_from_sources(
            '{0}{1}/cmake-{2}.{3}'.format(CMAKE_SRC_ROOT, stabled_version, version, ARCH_CMAKE_EXT, []),
            self.prefix_path_)

    def build_common(self):
        cmake_project_root_abs_path = '..'
        if not os.path.exists(cmake_project_root_abs_path):
            raise utils.BuildError('invalid cmake_project_root_path: %s' % cmake_project_root_abs_path)

        # project static options
        prefix_args = '-DCMAKE_INSTALL_PREFIX={0}'.format(self.prefix_path_)

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
            os.chdir(self.build_dir_path_)
            shutil.rmtree(cloned_dir)
        except Exception as ex:
            os.chdir(self.build_dir_path_)
            raise ex


if __name__ == "__main__":
    libpng_default_version = '1.6.29'
    sdl2_default_version = '2.0.5'
    ffmpeg_default_version = '3.2.4'
    cmake_default_version = '3.7.2'

    host_os = system_info.get_os()
    arch_host_os = system_info.get_arch_name()
    default_device = get_device().name()

    parser = argparse.ArgumentParser(prog='build_env', usage='%(prog)s [options]')
    parser.add_argument('--with-device',
                        help='build dependencies for device (default, device:{0})'.format(default_device),
                        dest='with_device', action='store_true')
    parser.add_argument('--without-device', help='build without device dependencies', dest='with_device',
                        action='store_false')
    parser.add_argument('--device', help='device (default: {0})'.format(default_device), default=default_device)
    parser.set_defaults(with_device=True)

    parser.add_argument('--with-system', help='build with system dependencies (default)', dest='with_system',
                        action='store_true')
    parser.add_argument('--without-system', help='build without system dependencies', dest='with_system',
                        action='store_false')
    parser.set_defaults(with_system=True)

    parser.add_argument('--with-libpng', help='build libpng (default, version:{0})'.format(libpng_default_version),
                        dest='with_libpng', action='store_true')
    parser.add_argument('--without-libpng', help='build without libpng', dest='with_libpng', action='store_false')
    parser.add_argument('--libpng-version', help='libpng version (default: {0})'.format(libpng_default_version),
                        default=libpng_default_version)
    parser.set_defaults(with_libpng=True)

    parser.add_argument('--with-sdl2', help='build sdl2 (default, version:{0})'.format(sdl2_default_version),
                        dest='with_sdl2', action='store_true')
    parser.add_argument('--without-sdl2', help='build without sdl2', dest='with_sdl2', action='store_false')
    parser.add_argument('--sdl2-version', help='sdl2 version (default: {0})'.format(sdl2_default_version),
                        default=sdl2_default_version)
    parser.set_defaults(with_sdl2=True)

    parser.add_argument('--with-ffmpeg', help='build ffmpeg (default, version:{0})'.format(ffmpeg_default_version),
                        dest='with_ffmpeg', action='store_true')
    parser.add_argument('--without-ffmpeg', help='build without ffmpeg', dest='with_ffmpeg', action='store_false')
    parser.add_argument('--ffmpeg-version', help='ffmpeg version (default: {0})'.format(ffmpeg_default_version),
                        default=ffmpeg_default_version)
    parser.set_defaults(with_ffmpeg=True)

    parser.add_argument('--with-cmake', help='build cmake (default, version:{0})'.format(cmake_default_version),
                        dest='with_cmake', action='store_true')
    parser.add_argument('--without-cmake', help='build without cmake', dest='with_cmake', action='store_false')
    parser.add_argument('--cmake-version', help='cmake version (default: {0})'.format(cmake_default_version),
                        default=cmake_default_version)
    parser.set_defaults(with_cmake=True)

    parser.add_argument('--with-common', help='build common (default, version: git master)', dest='with_common',
                        action='store_true')
    parser.add_argument('--without-common', help='build without common', dest='with_common', action='store_false')
    parser.set_defaults(with_common=True)

    parser.add_argument('--platform', help='build for platform (default: {0})'.format(host_os), default=host_os)
    parser.add_argument('--architecture', help='architecture (default: {0})'.format(arch_host_os),
                        default=arch_host_os)
    parser.add_argument('--prefix_path', help='prefix_path (default: None)', default=None)

    argv = parser.parse_args()

    platform_str = argv.platform
    prefix_path = argv.prefix_path
    architecture = argv.architecture
    device = get_supported_device_by_name(argv.device)
    if not device:
        raise utils.BuildError('invalid device')

    request = BuildRequest(device, platform_str, architecture, 'build_' + platform_str + '_env', prefix_path)
    if argv.with_system:
        request.install_system()

    if argv.with_device:
        request.install_device_specific()

    if argv.with_libpng:
        request.build_libpng(argv.libpng_version)
    if argv.with_sdl2:
        request.build_sdl2(argv.sdl2_version)
    if argv.with_ffmpeg:
        request.build_ffmpeg(argv.ffmpeg_version)

    if argv.with_cmake:
        request.build_cmake(argv.cmake_version)
    if argv.with_common:
        request.build_common()
