#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-
#
#  Codezero -- a microkernel for embedded systems.
#
#  Copyright © 2009  B Labs Ltd
#
import os, sys, shelve
from os.path import join

PROJRELROOT = '../../'

SCRIPTROOT = os.path.abspath(os.path.dirname("."))
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), PROJRELROOT)))

from config.projpaths import *
from config.configuration import *
from config.lib import *

LINUX_KERNEL_BUILDDIR = join(BUILDDIR, os.path.relpath(LINUX_KERNELDIR, PROJROOT))

# Create linux kernel build directory path as:
# conts/linux -> build/cont[0-9]/linux
def source_to_builddir(srcdir, id):
    cont_builddir = \
        os.path.relpath(srcdir, \
                        PROJROOT).replace("conts", \
                                          "cont" + str(id))
    return join(BUILDDIR, cont_builddir)

class LinuxUpdateKernel:

    def __init__(self, container):
        self.list = (['MAGIC_SYSRQ', 'SET'],['DEBUG_KERNEL', 'SET'])
        self.modify_kernel_config()
        self.update_kernel_params(container)

    # Replace line(having input_pattern) in filename with new_data
    def replace_line(self, filename, input_pattern, new_data, prev_line):
        with open(filename, 'r+') as f:
            flag = 0
            temp = 0
            x = re.compile(input_pattern)
            for line in f:
                if '' != prev_line:
                    if temp == prev_line and re.match(x, line):
                        flag = 1
                        break
                    temp = line
                else:
                    if re.match(x, line):
                        flag = 1
                        break

            if flag == 0:
                print 'Warning: No match found for the parameter'
                return
            else:
                # Prevent recompilation in case kernel parameter is same
                if new_data != line:
                    f.seek(0)
                    l = f.read()

                    # Need to truncate file because, size of contents to be
                    # written may be less than the size of original file.
                    f.seek(0)
                    f.truncate(0)

                    # Write back to file
                    f.write(l.replace(line, new_data))

    def update_kernel_params(self, container):
        # Update TEXT_START
        file = join(LINUX_KERNELDIR, 'arch/arm/boot/compressed/Makefile')
        param = str(conv_hex(container.linux_phys_offset))
        new_data = ('ZTEXTADDR' + '\t' + ':= ' + param + '\n')
        data_to_replace = "(ZTEXTADDR)(\t)(:= 0)"
        prev_line = ''
        self.replace_line(file, data_to_replace, new_data, prev_line)

        # Update PHYS_OFFSET
        file = join(LINUX_KERNELDIR, 'arch/arm/mach-versatile/include/mach/memory.h')
        param = str(conv_hex(container.linux_phys_offset))
        new_data = ('#define PHYS_OFFSET     UL(' + param + ')\n')
        data_to_replace = "(#define PHYS_OFFSET)"
        prev_line = ''
        self.replace_line(file, data_to_replace, new_data, prev_line)

        # Update PAGE_OFFSET
        file = join(LINUX_KERNELDIR, 'arch/arm/Kconfig')
        param = str(conv_hex(container.linux_page_offset))
        new_data = ('\t' + 'default ' + param + '\n')
        data_to_replace = "(\t)(default )"
        prev_line = ('\t'+'default 0x80000000 if VMSPLIT_2G' + '\n')
        self.replace_line(file, data_to_replace, new_data, prev_line)

        # Update ZRELADDR
        file = join(LINUX_KERNELDIR, 'arch/arm/mach-versatile/Makefile.boot')
        param = str(conv_hex(container.linux_zreladdr))
        new_data = ('   zreladdr-y' + '\t' + ':= ' + param + '\n')
        data_to_replace = "(\s){3}(zreladdr-y)(\t)(:= )"
        prev_line = ''
        self.replace_line(file, data_to_replace, new_data, prev_line)

        # Update ARCHID, CPUID and ATAGS ADDRESS
        # Atags will be present at PHYS_OFFSET + 0x100
        file = join(LINUX_KERNELDIR, 'arch/arm/kernel/head.S')
        prev_line = ''
        # CPUID for versatile: 0x41069265
        new_data = ('cpuid:  .word   ' + '0x41069265' + '\n')
        data_to_replace = "(cpuid:)"
        self.replace_line(file, data_to_replace, new_data, prev_line)
        # ARCHID for versatile: 0x183
        new_data = ('archid: .word   ' + '0x183' + '\n')
        data_to_replace = "(archid:)"
        self.replace_line(file, data_to_replace, new_data, prev_line)
        # Atags will be present at PHYS_OFFSET + 0x100(=256)
        new_data = ('atags:  .word   ' + \
                    str(conv_hex(container.linux_phys_offset + 0x100)) + '\n')
        data_to_replace = "(atags:)"
        self.replace_line(file, data_to_replace, new_data, prev_line)

    def modify_kernel_config(self):
        file = join(LINUX_KERNELDIR, 'arch/arm/configs/versatile_defconfig')
        for i in self.list:
            param = 'CONFIG_' + i[0]
            prev_line = ''
            if i[1] == 'SET':
                data_to_replace = ('# ' + param)
                new_data = (param + '=y' + '\n')
            else:
                data_to_replace = param
                new_data = ('# ' + param + ' is not set' + '\n')

            self.replace_line(file, data_to_replace, new_data, prev_line)


class LinuxBuilder:

    def __init__(self, pathdict, container):
        self.LINUX_KERNELDIR = pathdict["LINUX_KERNELDIR"]

        # Calculate linux kernel build directory
        self.LINUX_KERNEL_BUILDDIR = \
            source_to_builddir(LINUX_KERNELDIR, container.id)

        self.linux_lds_in = join(self.LINUX_KERNELDIR, "linux.lds.in")
        self.linux_lds_out = join(self.LINUX_KERNEL_BUILDDIR, "linux.lds")
        self.linux_S_in = join(self.LINUX_KERNELDIR, "linux.S.in")
        self.linux_S_out = join(self.LINUX_KERNEL_BUILDDIR, "linux.S")
        self.linux_elf_out = join(self.LINUX_KERNEL_BUILDDIR, "linux.elf")

        self.container = container
        self.kernel_binary_image = \
            join(os.path.relpath(self.LINUX_KERNEL_BUILDDIR, LINUX_KERNELDIR), \
                 "arch/arm/boot/Image")
        self.kernel_image = None
        self.kernel_updater = LinuxUpdateKernel(self.container)

    def build_linux(self):
        print '\nBuilding the linux kernel...'
        os.chdir(self.LINUX_KERNELDIR)
        if not os.path.exists(self.LINUX_KERNEL_BUILDDIR):
            os.makedirs(self.LINUX_KERNEL_BUILDDIR)
        os.system("make defconfig ARCH=arm O=" + self.LINUX_KERNEL_BUILDDIR)
        os.system("make ARCH=arm " + \
                  "CROSS_COMPILE=arm-none-linux-gnueabi- O=" + \
                  self.LINUX_KERNEL_BUILDDIR)

        with open(self.linux_S_in, 'r') as input:
            with open(self.linux_S_out, 'w+') as output:
                content = input.read() % self.kernel_binary_image
                output.write(content)

        os.system("arm-none-linux-gnueabi-cpp -P " + \
                  "%s > %s" % (self.linux_lds_in, self.linux_lds_out))
        os.system("arm-none-linux-gnueabi-gcc -nostdlib -o %s -T%s %s" % \
                  (self.linux_elf_out, self.linux_lds_out, self.linux_S_out))

        # Get the kernel image path
        self.kernel_image = self.linux_elf_out

        print 'Done...'

    def clean(self):
        print 'Cleaning linux kernel build...'
        if os.path.exists(self.LINUX_KERNEL_BUILDDIR):
            shutil.rmtree(self.LINUX_KERNEL_BUILDDIR)
        print 'Done...'

if __name__ == "__main__":
    # This is only a default test case
    container = Container()
    container.id = 0
    linux_builder = LinuxBuilder(projpaths, container)

    if len(sys.argv) == 1:
        linux_builder.build_linux()
    elif "clean" == sys.argv[1]:
        linux_builder.clean()
    else:
        print " Usage: %s [clean]" % (sys.argv[0])
