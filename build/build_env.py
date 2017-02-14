#!/usr/bin/env python
import sys
import urllib2
import subprocess
import os
import shutil
import platform
from pybuild_utils.base import system_info

SDL_SRC_ROOT = "https://www.libsdl.org/release/"
FFMPEG_SRC_ROOT = "http://ffmpeg.org/releases/"

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
        "[required] argv[1] sdl2 version(2.0.5)\n"
        "[required] argv[2] ffmpeg version(3.2.2)\n"
        "[optional] argv[3] platform\n"
        "[optional] argv[4] architecture\n"
        "[optional] argv[5] prefix path\n")

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

def build_ffmpeg(url, prefix_path):
    pwd = os.getcwd()
    file = download_file(url)
    folder = extract_file(file)
    os.chdir(folder)
    subprocess.call(['./configure', '--prefix={0}'.format(prefix_path), '--disable-libxcb'])
    subprocess.call(['make', 'install'])
    os.chdir(pwd)
    shutil.rmtree(folder)
    
def git_clone(self, url):
    pwd = os.getcwd()
    common_git_clone_line = ['git', 'clone']
    common_git_clone_line.append(url)
    cloned_dir = os.path.splitext(url.rsplit('/', 1)[-1])[0]
    common_git_clone_line.append(cloned_dir)
    run_command.run_command_cb(common_git_clone_line, git_policy)
    os.chdir(cloned_dir)

    common_git_clone_init_line = ['git', 'submodule', 'update', '--init', '--recursive']
    run_command.run_command_cb(common_git_clone_init_line, git_policy)
    os.chdir(pwd)
    return os.path.join(pwd, cloned_dir)

class BuildRequest(object):
    def __init__(self, platform, arch_bit):
        platform_or_none = system_info.get_supported_platform_by_name(platform)

        if platform_or_none == None:
            raise utils.BuildError('invalid platform')

        arch = platform_or_none.architecture_by_bit(arch_bit)
        if arch == None:
            raise utils.BuildError('invalid arch')

        self.platform_ = system_info.Platform(platform_or_none.name(), arch, platform_or_none.package_types())
        print("Build request for platform: {0}, arch: {1} created".format(platform, arch.name()))

    def git_clone(self, url):
        pwd = os.getcwd()
        common_git_clone_line = ['git', 'clone']
        common_git_clone_line.append(url)
        cloned_dir = os.path.splitext(url.rsplit('/', 1)[-1])[0]
        common_git_clone_line.append(cloned_dir)
        subprocess.call(common_git_clone_line)
        os.chdir(cloned_dir)

        common_git_clone_init_line = ['git', 'submodule', 'update', '--init', '--recursive']
        subprocess.call(common_git_clone_init_line)
        os.chdir(pwd)
        return os.path.join(pwd, cloned_dir)

    def build(self, dir_path, prefix_path):
        cmake_project_root_abs_path = '..'
        if not os.path.exists(cmake_project_root_abs_path):
            raise utils.BuildError('invalid cmake_project_root_path: %s' % cmake_project_root_abs_path)

        if prefix_path == None:
            prefix_path = self.platform_.arch().default_install_prefix_path()

        abs_dir_path = os.path.abspath(dir_path)
        if os.path.exists(abs_dir_path):
            shutil.rmtree(abs_dir_path)

        pwd = os.getcwd()
        os.mkdir(abs_dir_path)
        os.chdir(abs_dir_path)

        platform_name = self.platform_.name()
        arch = self.platform_.arch()

        if platform_name == 'linux':
            distr = get_dist()
            if distr  == 'DEBIAN':
                dep_libs = ['gcc', 'g++', 'yasm', 'cmake', 'pkg-config', 'libz-dev', 'libbz2-dev', 'libasound2-dev', 'libpcre3-dev', 'libx11-dev', 'libdri2-dev', 'libxext-dev', 'libdrm-dev', 'xorg-dev', 'xutils-dev']
            elif distr == 'RHEL':
                dep_libs = ['gcc', 'gcc-c++', 'yasm', 'cmake', 'pkgconfig', 'libz-devel', 'libbz2-devel', 'libasound2-dev', 'pcre-devel', 'libx11-devel', 'libdri2-devel', 'libxext-devel', 'libdrm-devel', 'xorg-devel', 'xorg-x11-server-devel']

            for lib in dep_libs:
                if distr  == 'DEBIAN':
                    subprocess.call(['apt-get', '-y', '--force-yes', 'install', lib])
                elif distr == 'RHEL':
                    subprocess.call(['yum', '-y', 'install', lib])
        elif platform_name == 'windows':
            if  arch.bit() == 64:
                dep_libs = ['mingw-w64-x86_64-toolchain', 'mingw-w64-x86_64-yasm']
            elif arch.bit() == 32:
                dep_libs = ['mingw-w64-i686-toolchain', 'mingw-w64-i686-yasm']

            for lib in dep_libs:
                subprocess.call(['pacman', '-S', lib])

        # project static options
        prefix_args = '-DCMAKE_INSTALL_PREFIX={0}'.format(prefix_path)

        cmake_line = ['cmake', cmake_project_root_abs_path, '-GUnix Makefiles', '-DCMAKE_BUILD_TYPE=RELEASE', prefix_args]
        try:
            cloned_dir = self.git_clone('https://github.com/fastogt/common.git')
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
            file = download_file(url)
            folder = extract_file(file)
            os.chdir(folder)
            subprocess.call(['./configure','--prefix={0}'.format(prefix_path)])
            subprocess.call(['make', 'install'])
            os.chdir(abs_dir_path)
            shutil.rmtree(folder)

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