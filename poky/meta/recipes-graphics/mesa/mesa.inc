SUMMARY = "A free implementation of the OpenGL API"
DESCRIPTION = "Mesa is an open-source implementation of the OpenGL specification - \
a system for rendering interactive 3D graphics.  \
A variety of device drivers allows Mesa to be used in many different environments \
ranging from software emulation to complete hardware acceleration for modern GPUs. \
Mesa is used as part of the overall Direct Rendering Infrastructure and X.org \
environment."

HOMEPAGE = "http://mesa3d.org"
BUGTRACKER = "https://bugs.freedesktop.org"
SECTION = "x11"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://docs/license.html;md5=42d77d95cba529a3637129be87d6555d"

INC_PR = "r9"
PE = "2"

DEPENDS = "expat makedepend-native flex-native bison-native"

PROVIDES = "virtual/libgl virtual/libgles1 virtual/libgles2 virtual/egl virtual/mesa"

inherit autotools pkgconfig pythonnative gettext distro_features_check

REQUIRED_DISTRO_FEATURES = "opengl"

EXTRA_OECONF = "--enable-shared-glapi"

PACKAGECONFIG ??= "egl gles dri \
		${@base_contains('DISTRO_FEATURES', 'x11', 'x11', '', d)}\
		${@base_contains('DISTRO_FEATURES', 'wayland', 'wayland', '', d)}\
		"

X11_DEPS = "xf86driproto glproto virtual/libx11 libxext libxxf86vm libxdamage libxfixes"
PACKAGECONFIG[x11] = "--enable-glx-tls,--disable-glx,${X11_DEPS}"
PACKAGECONFIG[xvmc] = "--enable-xvmc,--disable-xvmc,libxvmc"
PACKAGECONFIG[wayland] = ",,wayland"

DRIDRIVERS = "swrast"
DRIDRIVERS_append_x86 = ",radeon,r200,nouveau,i965,i915"
DRIDRIVERS_append_x86-64 = ",radeon,r200,nouveau,i965,i915"
PACKAGECONFIG[dri] = "--enable-dri --with-dri-drivers=${DRIDRIVERS}, --disable-dri, dri2proto libdrm udev"

EXTRA_OECONF += "--enable-gbm"

PACKAGECONFIG[gles] = "--enable-gles1 --enable-gles2, --disable-gles1 --disable-gles2"

EGL_PLATFORMS  = "drm"
EGL_PLATFORMS .="${@base_contains('PACKAGECONFIG', 'x11', ',x11', '', d)}"
EGL_PLATFORMS .="${@base_contains('PACKAGECONFIG', 'wayland', ',wayland', '', d)}"
PACKAGECONFIG[egl] = "--enable-egl --with-egl-platforms=${EGL_PLATFORMS}, --disable-egl"

PACKAGECONFIG[openvg] = "--enable-openvg, --disable-openvg"

GALLIUMDRIVERS = "swrast"
GALLIUMDRIVERS_LLVM33 = "${@base_contains('PACKAGECONFIG', 'r600', 'radeonsi,r600', '', d)}"
GALLIUMDRIVERS_LLVM33_ENABLED = "${@base_version_less_or_equal('MESA_LLVM_RELEASE', '3.2', False, len('${GALLIUMDRIVERS_LLVM33}') > 0, d)}"
GALLIUMDRIVERS_LLVM = "r300,svga,nouveau${@',${GALLIUMDRIVERS_LLVM33}' if ${GALLIUMDRIVERS_LLVM33_ENABLED} else ''}"
GALLIUMDRIVERS_append_x86 = "${@base_contains('PACKAGECONFIG', 'gallium-llvm', ',${GALLIUMDRIVERS_LLVM}', '', d)}"
GALLIUMDRIVERS_append_x86-64 = "${@base_contains('PACKAGECONFIG', 'gallium-llvm', ',${GALLIUMDRIVERS_LLVM}', '', d)}"
# keep --with-gallium-drivers separate, because when only one of gallium versions is enabled, other 2 were adding --without-gallium-drivers
PACKAGECONFIG[gallium]      = "--with-gallium-drivers=${GALLIUMDRIVERS}, --without-gallium-drivers"
PACKAGECONFIG[gallium-egl]  = "--enable-gallium-egl, --disable-gallium-egl"
PACKAGECONFIG[gallium-gbm]  = "--enable-gallium-gbm, --disable-gallium-gbm"
MESA_LLVM_RELEASE ?= "3.3"
PACKAGECONFIG[gallium-llvm] = "--enable-gallium-llvm --with-llvm-shared-libs, --disable-gallium-llvm, llvm${MESA_LLVM_RELEASE} \
                               ${@'elfutils' if ${GALLIUMDRIVERS_LLVM33_ENABLED} else ''}"
export WANT_LLVM_RELEASE = "${MESA_LLVM_RELEASE}"

# llvmpipe is slow if compiled with -fomit-frame-pointer (e.g. -O2)
FULL_OPTIMIZATION_append = " -fno-omit-frame-pointer"

# Multiple virtual/gl providers being built breaks staging
EXCLUDE_FROM_WORLD = "1"

# Remove the mesa dependency on mesa-dev, as mesa is empty
RDEPENDS_${PN}-dev = ""

PACKAGES =+ "libegl-mesa libegl-mesa-dev \
             libosmesa libosmesa-dev \
             libgl-mesa libgl-mesa-dev \
             libdricore libdricore-dev \
             libglapi libglapi-dev \
             libgbm libgbm-dev \
             libgles1-mesa libgles1-mesa-dev \
             libgles2-mesa libgles2-mesa-dev \
             libgles3-mesa libgles3-mesa-dev \
             libwayland-egl libwayland-egl-dev \
             libegl-gallium libgbm-gallium \
             libopenvg libopenvg-dev \
             libxvmcsoftpipe libxvmcsoftpipe-dev \
            "

do_install_append () {
    # Drivers never need libtool .la files
    rm -f ${D}${libdir}/dri/*.la
    rm -f ${D}${libdir}/egl/*.la
    rm -f ${D}${libdir}/gallium-pipe/*.la
    rm -f ${D}${libdir}/gbm/*.la
    
    # it was packaged in libdricore9.1.3-1 and preventing upgrades when debian.bbclass was used 
    rm -f ${D}${sysconfdir}/drirc
}

# For the packages that make up the OpenGL interfaces, inject variables so that
# they don't get Debian-renamed (which would remove the -mesa suffix), and
# RPROVIDEs/RCONFLICTs on the generic libgl name.
python __anonymous() {
    for p in (("libegl", "libegl1"), ("libgl", "libgl1"),
              ("libgles1", "libglesv1-cm1"), ("libgles2", "libglesv2-2"),
              ("libgles3",)):
        fullp = p[0] + "-mesa"
        pkgs = " ".join(p)
        d.setVar("DEBIAN_NOAUTONAME_" + fullp, "1")
        d.appendVar("RREPLACES_" + fullp, pkgs)
        d.appendVar("RPROVIDES_" + fullp, pkgs)
        d.appendVar("RCONFLICTS_" + fullp, pkgs)
        
        # For -dev, the first element is both the Debian and original name
        fullp += "-dev"
        pkgs = p[0] + "-dev"
        d.setVar("DEBIAN_NOAUTONAME_" + fullp, "1")
        d.appendVar("RREPLACES_" + fullp, pkgs)
        d.appendVar("RPROVIDES_" + fullp, pkgs)
        d.appendVar("RCONFLICTS_" + fullp, pkgs)
}

python mesa_populate_packages() {
    dri_drivers_root = os.path.join(d.getVar('libdir', True), "dri")

    pkgs = ['mesa', 'mesa-dev', 'mesa-dbg']
    pkgs += do_split_packages(d, dri_drivers_root, '^(.*)_dri\.so$', 'mesa-driver-%s', 'Mesa %s DRI driver', extra_depends='')
    for pkg in pkgs:
        d.setVar("RPROVIDES_%s" % pkg, pkg.replace("mesa-", "mesa-dri-"))
        d.setVar("RCONFLICTS_%s" % pkg, pkg.replace("mesa-", "mesa-dri-"))
        d.setVar("RREPLACES_%s" % pkg, pkg.replace("mesa-", "mesa-dri-"))
    
    pipe_drivers_root = os.path.join(d.getVar('libdir', True), "gallium-pipe")
    do_split_packages(d, pipe_drivers_root, '^pipe_(.*)\.so$', 'mesa-driver-pipe-%s', 'Mesa %s pipe driver', extra_depends='')
}

PACKAGESPLITFUNCS_prepend = "mesa_populate_packages "

PACKAGES_DYNAMIC += "^mesa-driver-.*"

FILES_${PN}-dbg += "${libdir}/dri/.debug/*"

FILES_libdricore = "${sysconfdir}/drirc ${libdir}/libdricore*.so.*"
FILES_libegl-mesa = "${libdir}/libEGL.so.*"
FILES_libgbm = "${libdir}/libgbm.so.*"
FILES_libgles1-mesa = "${libdir}/libGLESv1*.so.*"
FILES_libgles2-mesa = "${libdir}/libGLESv2.so.*"
FILES_libgl-mesa = "${libdir}/libGL.so.*"
FILES_libglapi = "${libdir}/libglapi.so.*"
FILES_libosmesa = "${libdir}/libOSMesa.so.*"
FILES_libwayland-egl = "${libdir}/libwayland-egl.so.*"
FILES_libegl-gallium = "${libdir}/egl/egl_gallium.so*"
FILES_libgbm-gallium = "${libdir}/gbm/gbm_gallium_drm.so*"
FILES_libopenvg = "${libdir}/libOpenVG.so.*"
FILES_libxvmcsoftpipe = "${libdir}/libXvMCsoftpipe.so.*"

FILES_${PN}-dev = "${libdir}/pkgconfig/dri.pc"
FILES_libdricore-dev = "${libdir}/libdricore*.*"
FILES_libegl-mesa-dev = "${libdir}/libEGL.* ${includedir}/EGL ${includedir}/KHR ${libdir}/pkgconfig/egl.pc"
FILES_libgbm-dev = "${libdir}/libgbm.* ${libdir}/pkgconfig/gbm.pc ${includedir}/gbm.h"
FILES_libgl-mesa-dev = "${libdir}/libGL.* ${includedir}/GL ${libdir}/pkgconfig/gl.pc"
FILES_libglapi-dev = "${libdir}/libglapi.*"
FILES_libgles1-mesa-dev = "${libdir}/libGLESv1*.* ${includedir}/GLES ${libdir}/pkgconfig/glesv1*.pc"
FILES_libgles2-mesa-dev = "${libdir}/libGLESv2.* ${includedir}/GLES2 ${libdir}/pkgconfig/glesv2.pc"
FILES_libgles3-mesa-dev = "${includedir}/GLES3"
FILES_libosmesa-dev = "${libdir}/libOSMesa.* ${includedir}/osmesa.h ${libdir}/pkgconfig/osmesa.pc"
FILES_libwayland-egl-dev = "${libdir}/pkgconfig/wayland-egl.pc ${libdir}/libwayland-egl.*"
FILES_libopenvg-dev = "${libdir}/libOpenVG.so ${libdir}/libOpenVG.la* \
                       ${includedir}/VG ${libdir}/pkgconfig/vg.pc"
FILES_libxvmcsoftpipe-dev = "${libdir}/libXvMCsoftpipe.so ${libdir}/libXvMCsoftpipe.la"

FILES_${PN}-dbg += "${libdir}/dri/.debug/* ${libdir}/egl/.debug/* ${libdir}/gbm/.debug/* ${libdir}/gallium-pipe/.debug"
