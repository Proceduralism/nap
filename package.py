#!/usr/bin/env python
import argparse
import datetime
import json
import os
from multiprocessing import cpu_count
import shutil
import subprocess
import sys
from sys import platform

WORKING_DIR = '.'
BUILD_DIR = 'packagingBuild'
PACKAGING_DIR = 'packagingStaging'
ARCHIVING_DIR = 'archiving'
BUILDINFO_FILE = 'dist/cmake/buildInfo.json'

def call(cwd, cmd, capture_output=False, exception_on_nonzero=True):
    # print('dir: %s' % cwd)
    # print('cmd: %s' % cmd)
    if capture_output:
        proc = subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    else:
        proc = subprocess.Popen(cmd, cwd=cwd)
    (out, err) = proc.communicate()
    if exception_on_nonzero and proc.returncode != 0:
        print("Bailing for non zero returncode")
        raise Exception(proc.returncode)
    return (out, err)

# Install dependencies for Linux, checking if we have them first (avoiding an sudo if they're already installed)
def install_dependencies_linux():
    dependencies = [
        'cmake',
        'build-essential',
        'patchelf'
    ]

    # Create a list of packages we need to install
    packages_to_install = []
    for d in dependencies:
        if not is_linux_apt_package_installed(d):
            packages_to_install.append(d)

    # Return if all already installed
    if len(packages_to_install) == 0:
        print("All dependencies already installed")
        return

    # Build cmd and install with apt
    print("Installing packages via apt: %s" % ' '.join(packages_to_install))
    apt_cmd = ['sudo', 'apt-get', '--assume-yes', 'install']
    apt_cmd.extend(packages_to_install)
    call(WORKING_DIR, apt_cmd)

# Check if we have a package installed with apt
def is_linux_apt_package_installed(package_name):
    (out, err) = call(WORKING_DIR, ['dpkg', '--get-selections', package_name], True)
    if type(err) is bytes:
        err = err.decode("utf-8")    
    installed = not 'no packages' in err
    print("Package '%s' installed? %s" % (package_name, installed))
    return installed

# Check if brew is installed
def is_osx_brew_installed():
    try:
        brew_path = call(WORKING_DIR, ['which', 'brew'], True)[0].strip()
        return os.path.exists(brew_path)
    except:
        return False

# Check if we have a package installed with homebrew
def is_osx_brew_package_installed(package_name):
    # TODO Running brew list once without the packagename and searching the results would be faster
    (out, err) = call(WORKING_DIR, ['brew', 'list', package_name], True, False)
    if type(err) is bytes:
        err = err.decode("utf-8")
    installed = not 'Error: No such keg' in err
    print("Package '%s' installed? %s" % (package_name, installed))
    return installed

# Install dependencies for macOS via homebrew, checking if we have them first
def install_dependencies_osx():
    dependencies = [
        'cmake'
    ]

    if not is_osx_brew_installed():
        print("Not installing macOS dependencies as homebrew was not found")
        # TODO potentially fail here in future if we have a hard homebrew dependency
        return

    # Create a list of packages we need to install
    packages_to_install = []
    for d in dependencies:
        if not is_osx_brew_package_installed(d):
            packages_to_install.append(d)

    # Return if all already installed
    if len(packages_to_install) == 0:
        print("All dependencies already installed")
        return

    for pack in packages_to_install:
        try:
            call(WORKING_DIR, ['brew', 'install', pack])
        except:
            print("Failed installing %s via homebrew" % pack)

def install_dependencies():
    print("Installing dependencies")
    if platform in ['linux', 'linux2']:
        install_dependencies_linux()
    elif platform == 'darwin':
        install_dependencies_osx()
    elif platform == 'win32':
        # Windows...
        pass

def package(zip_release, include_docs, include_apps):
    print("Packaging..")

    install_dependencies()

    # Note: Packaging directly from Python for now.  CPack was investigated but it was looking difficult to make it work when
    # wanting to build multiple configurations at the same time.  If there was a reasonable CPack solution it feels like that 
    # would be cleaner than this.

    # Remove old packaging path if it exists
    if os.path.exists(PACKAGING_DIR):
        shutil.rmtree(PACKAGING_DIR, True)
    os.makedirs(PACKAGING_DIR)

    # Add timestamp and git revision for build info
    timestamp = datetime.datetime.now().strftime('%Y.%m.%dT%H.%M')
    (git_revision, _) = call(WORKING_DIR, ['git', 'rev-parse', 'HEAD'], True)
    git_revision = git_revision.decode('ascii', 'ignore').strip()

    if platform in ["linux", "linux2"]:
        for build_type in ['Release', 'Debug']:
            build_dir_for_type = BUILD_DIR + build_type
            call(WORKING_DIR, ['cmake', 
                               '-H.', 
                               '-B%s' % build_dir_for_type, 
                               '-DNAP_PACKAGED_BUILD=1',
                               '-DCMAKE_BUILD_TYPE=%s' % build_type,
                               '-DINCLUDE_DOCS=%s' % int(include_docs),
                               '-DPACKAGE_NAIVI_APPS=%s' % int(include_apps),
                               '-DBUILD_TIMESTAMP=%s' % timestamp,
                               '-DBUILD_GIT_REVISION=%s' % git_revision
                               ])

            d = '%s/%s' % (WORKING_DIR, build_dir_for_type)
            call(d, ['make', 'all', 'install', '-j%s' % cpu_count()])

        # Create archive
        if zip_release:
            archive_to_linux_tar_xz(timestamp)
        else:
            archive_to_timestamped_dir('Linux', timestamp)
    elif platform == 'darwin':
        # Generate project
        call(WORKING_DIR, ['cmake', 
                           '-H.', 
                           '-B%s' % BUILD_DIR, 
                           '-G', 'Xcode',
                           '-DNAP_PACKAGED_BUILD=1',                 
                           '-DINCLUDE_DOCS=%s' % int(include_docs),
                           '-DPACKAGE_NAIVI_APPS=%s' % int(include_apps),
                           '-DBUILD_TIMESTAMP=%s' % timestamp,
                           '-DBUILD_GIT_REVISION=%s' % git_revision
                           ])

        # Build & install to packaging dir
        d = '%s/%s' % (WORKING_DIR, BUILD_DIR)
        for build_type in ['Release', 'Debug']:
            call(d, ['xcodebuild', '-configuration', build_type, '-target', 'install', '-jobs', str(cpu_count())])

        # Remove unwanted files (eg. .DS_Store)
        call(PACKAGING_DIR, ['find', '.', '-name', '.DS_Store', '-type', 'f', '-delete'])

        # Create archive
        if zip_release:
            archive_to_macos_zip(timestamp)
        else:
            archive_to_timestamped_dir('macOS', timestamp)
    else:
        # Create build dir if it doesn't exist
        if not os.path.exists(BUILD_DIR):
            os.makedirs(BUILD_DIR)

        # Generate project
        call(WORKING_DIR, ['cmake', 
                           '-H.', 
                           '-B%s' % BUILD_DIR, 
                           '-G', 'Visual Studio 14 2015 Win64',
                           '-DNAP_PACKAGED_BUILD=1',
                           '-DPYBIND11_PYTHON_VERSION=3.5',
                           '-DINCLUDE_DOCS=%s' % int(include_docs),
                           '-DPACKAGE_NAIVI_APPS=%s' % int(include_apps),
                           '-DBUILD_TIMESTAMP=%s' % timestamp,
                           '-DBUILD_GIT_REVISION=%s' % git_revision
                           ])

        # Build & install to packaging dir
        for build_type in ['Release', 'Debug']:
            call(WORKING_DIR, ['cmake', '--build', BUILD_DIR, '--target', 'install', '--config', build_type])

        # Create archive
        if zip_release:
            archive_to_win64_zip(timestamp)
        else:
            archive_to_timestamped_dir('Win64', timestamp)

    return True

# Create build archive to xz tarball on Linux
def archive_to_linux_tar_xz(timestamp):
    package_filename = build_package_basename('Linux', timestamp)
    shutil.move(PACKAGING_DIR, package_filename)

    package_filename_with_ext = '%s.%s' % (package_filename, 'tar.xz')
    print("Archiving to %s.." % package_filename_with_ext)
    call(WORKING_DIR, ['tar', '-cJvf', package_filename_with_ext, package_filename])

    # Cleanup
    shutil.move(package_filename, PACKAGING_DIR)
    print("Packaged to %s" % package_filename_with_ext)  

# Create build archive to zip on macOS
def archive_to_macos_zip(timestamp):
    package_filename = build_package_basename('macOS', timestamp)
    shutil.move(PACKAGING_DIR, package_filename)

    # Archive
    package_filename_with_ext = '%s.%s' % (package_filename, 'zip')
    print("Archiving to %s.." % package_filename_with_ext)
    call(WORKING_DIR, ['zip', '-yr', package_filename_with_ext, package_filename])

    # Cleanup
    shutil.move(package_filename, PACKAGING_DIR)
    print("Packaged to %s" % package_filename_with_ext)  

# Create build archive to zip on Win64
def archive_to_win64_zip(timestamp):
    package_filename = build_package_basename('Win64', timestamp)
    package_filename_with_ext = '%s.%s' % (package_filename, 'zip')

    # Rename our packaging dir to match the release
    shutil.move(PACKAGING_DIR, package_filename)

    # Create our archive dir, used to create a copy level folder within the archive
    if os.path.exists(ARCHIVING_DIR):
        shutil.rmtree(ARCHIVING_DIR, True)
    os.makedirs(ARCHIVING_DIR)
    archive_path = os.path.join(ARCHIVING_DIR, package_filename)
    shutil.move(package_filename, archive_path)

    # Create archive
    print("Archiving to %s.." % package_filename_with_ext)
    shutil.make_archive(package_filename, 'zip', ARCHIVING_DIR)

    # Cleanup
    shutil.move(archive_path, PACKAGING_DIR)
    shutil.rmtree(ARCHIVING_DIR)

    print("Packaged to %s" % package_filename_with_ext)  

# Copy our packaged dir to a timestamped dir
def archive_to_timestamped_dir(platform, timestamp):
    package_filename = build_package_basename(platform, timestamp)
    shutil.move(PACKAGING_DIR, package_filename)

    print("Packaged to directory %s" % package_filename)

# Build the name of our package and populate our JSON build info file
def build_package_basename(platform, timestamp):
    # Fetch version from build info
    # TODO hardening.  deal with missing build info, exception loading build info file and no version entry.
    with open(BUILDINFO_FILE) as json_file:
        build_info = json.load(json_file)

    package_filename = "NAP-%s-%s-%s" % (build_info['version'], platform, timestamp)
    return package_filename
    
# Main
if __name__ == '__main__':
    # TODO add options for
    # - managing clean build behaviour
    # - not populating git revision into buildInfo
    # - external build number management?

    parser = argparse.ArgumentParser()
    parser.add_argument("-nz", "--no-zip", action="store_true",
                        help="Don't zip the release, package to a directory")
    parser.add_argument("-a", "--include-apps", action="store_true",
                        help="Include Naivi apps, packaging them as projects")
    parser.add_argument("--include-docs", action="store_true",
                        help="Include documentation")      
    args = parser.parse_args()

    # Package our build
    packaging_success = package(not args.no_zip, args.include_docs, args.include_apps)

    # TODO improve error propogation behaviour
    sys.exit(0 if packaging_success else 1)
