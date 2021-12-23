# to see the make rules:
#   make rules
#
# command uses:
#   make <name>-clean
#   make <name>
#   make rules
#   make val-<var>
#
# to add a new library:
#   vi ../configure.ac
#     add PKG_3RD(pkg,<yes/no/auto>, <ver_basename>, <lib_paths>, <inc_paths>)
#     add CHECK_LIB(pkg, <syslib>, <entry_pt>, [opt libs]) if may be in system
#     add CHECK_HEADERS(pkg, <title>, <entry_pt>) to check for devel headers
#     add CHECK_WANT(<want>, <title>, [ checks ]) around tests if conditional
#     see openexr check for example if c++ lib
#     add PKG_PROVIDE(pkg,<cond>) to instance pkg if probe succeeds
#     add to exec > global_config if additional environment needed
#   vi Makefile
#     add pkg.<data> (pkg.x = .flags, .vars, .params)
#     add $(call rules,$(call std-build,<name>,<dep>...))
#     add deps to other call rules if they require the new library
#     add config enables if other libraries use the new library
#     add pc_<pkg>=<pkgcfg names> if needed for system build
#

TOPDIR ?= $(CURDIR)/..
include $(TOPDIR)/global_config
-include $(TOPDIR)/cin_config
BLD := $(THIRDPARTY)/build

ifneq ($(WANT_CIN_3RDPARTY),no)

# package build flags, if needed
# need frame pointer for debugs and helps with profiling
# NOTE: the trailing blank is MANDITORY or the builds fail
# due to bugs in the build scripts from auto*
CFLAGS_ ?= -g -O2 -fno-omit-frame-pointer
CFLAGS ?= $(CFLAGS_) 
LDFLAGS += $(LDFLAGS_) 
CXXFLAGS += $(CXXFLAGS_) 

pkg-source=$(BLD)/$(1).source
pkg-config=$(BLD)/$(1).configure
pkg-built=$(BLD)/$(1).built

ext=$(lastword $(subst ., ,$(1)))
tarball=$(if $($(1).git),$(1).git,$(lastword $(sort $(wildcard src/$(1)*.tar.*))))
unpack=$(call unpack_$(call ext,$(1)),$(1))
unpack_git=git clone $($(1)) $(1)
unpack_gz=tar -xzf $(1)
unpack_bz2=tar -xjf $(1)
unpack_xz=tar -xJf $(1)
bld_depends=$(if $(ver_$(1)),$(call pkg-built,$(1)))
bld_path=$(ver_$(1))/$(2)
if_pkg=$(if $(ver_$(1)),$(2),$(3))
if_npkg=$(if $(ver_$(1)),,$(2))
if_shr=$(if $(filter $(1),$(shared_pkgs)),$(2),$(3))
pkg_conf=$(call if_shr,$(1),$(foreach p,$(pc_$(1)), $(shell pkg-config 2> /dev/null $(2) $(p))))
inc_path=$(call if_pkg,$(1),$(inc_$(1)), $(call pkg_conf,$(1),--cflags))
ld_path=$(call if_pkg,$(1),-L$(call bld_path,$(1),$(2)) $(lib_$(1)))
if_ena=$(if $(or $(ver_$(1)),$(filter $(1),$(shared_pkgs))),$(2))
if_want=$(if $(filter x$(WANT_$(1)),xyes),$(2),$(3))
sysincs:=usr/include usr/local/include
syslibs:=usr/lib usr/lib32 usr/lib64 lib lib32 lib64 usr/local/lib usr/local/lib32 usr/local/lib64
pkg_incl=$(lastword $(wildcard $(foreach f,$(sysincs), $(call bld_path,$(1),$(f)))))
pkg_libs=$(lastword $(wildcard $(foreach f,$(syslibs), $(call bld_path,$(1),$(f)))))

#$(eval $(call std-build,pkg,deps...))
#$(pkg.cflags) added as CFLAGS+=$(cflags) to pkg.vars
#$(pkg.cppflags) added as CPPFLAGS+=$(cppflags) to pkg.vars
#$(pkg.cfg_vars) prepended to configure
#$(pkg.cfg_params) appended to configure
#$(pkg.mak_vars) prepended to make
#$(pkg.mak_params) appended to make

define std-build
# $(1)
.PHONY: $(1) $(1)-configure $(1)-src $(1)-clean
$(1): $(call pkg-built,$(1))
$(1)-configure: $(call pkg-config,$(1))
$(1)-src: $(call pkg-source,$(1))
$(1)-clean:
	rm -rf $(1)* $(BLD)/$(1).*

$(call pkg-source,$(1)):
	$(call unpack,$(call tarball,$(1))) \
	$(foreach pch,$(sort $(wildcard src/$(notdir $(ver_$(1))).patch*)),
		patch -d $(1)* -p1 < $(pch))
	touch $$@

$(call pkg-config,$(1)): $(call pkg-source,$(1)) $(foreach dep,$(2),$(call bld_depends,$(dep)))
	@echo "CONFIGURING $(1)"
	cd $(1)* && \
		$($(1).cfg_vars) $(CFG_VARS) ./configure $($(1).cfg_params) $(CFG_PARAMS)
	touch $$@

$(call pkg-built,$(1)): $(call pkg-config,$(1))
	+$($(1).mak_vars) $(MAK_VARS) $(MAKE) -C $(1)* $($(1).mak_params) $(MAK_PARAMS)
	touch $$@

endef

TARGETS := $(static_blds)

all:	$(TARGETS)

$(TARGETS):	$(BLD)

$(BLD):
	mkdir $(BLD)

# pkg-config names
pc_libaom=aom
pc_dav1d=dav1d
pc_libwebp=libwebp libwebpmux
pc_opus=opus
pc_openjpeg=libopenjp2
pc_libvorbis=vorbis vorbisenc
pc_libvpx=vpx
pc_x264=x264
pc_x265=x265

# vars first
encore.cfg_vars= true ||
esound.cfg_vars:= AUDIOFILE_CFLAGS="$(call inc_path,audiofile) -laudiofile"
esound.cfg_vars+= AUDIOFILE_LIBS="$(call ld_path,audiofile,libaudiofile/.libs)"
esound.cfg_params= --enable-shared=no --with-pic
esound.mak_vars+= CFLAGS="" 
esound.ldflags=" -lm -lstdc++"
fftw.cfg_params= --disable-fortran --enable-shared=no
ffmpeg.cfg_params= \
	--enable-pthreads --enable-gpl --disable-ffplay \
	$(call if_want,VAAPI,--enable-vaapi,--disable-vaapi) \
	$(call if_want,VDPAU,--enable-vdpau,--disable-vdpau) \
	$(call if_want,NV, --enable-nvenc --enable-nvdec --enable-ffnvcodec) \
	$(call if_ena,twolame,--enable-libtwolame) \
	$(call if_ena,openjpeg,--enable-libopenjpeg) \
	$(call if_ena,lame,--enable-libmp3lame) \
	$(call if_ena,libaom,--enable-libaom) \
	$(call if_ena,dav1d,--enable-libdav1d) \
	$(call if_ena,libwebp,--enable-libwebp) \
	$(call if_ena,opus,--enable-libopus) \
	$(call if_ena,libvorbis,--enable-libvorbis) \
	$(call if_ena,libtheora,--enable-libtheora) \
	$(call if_ena,libvpx,--enable-libvpx) \
	$(call if_ena,x264,--enable-libx264) \
	$(call if_ena,x265,--enable-libx265) \
	--extra-cflags="-Wno-attributes \
		$(call if_want,NV,$(inc_ffnvcodec)) \
		$(call inc_path,twolame) \
		$(call inc_path,lame) \
		$(call inc_path,libaom) \
		$(call inc_path,dav1d) \
		$(call inc_path,libwebp) \
		$(call inc_path,openjpeg) \
		$(call inc_path,libogg) \
		$(call inc_path,opus) \
		$(call inc_path,libvorbis) \
		$(call inc_path,libtheora) \
		$(call inc_path,libvpx) \
		$(call inc_path,x264) \
		$(call inc_path,x265)" \
	--extra-cxxflags="-D__STDC_CONSTANT_MACROS" \
	--pkg-config=true \
	--extra-libs="-Wl,--start-group \
		$(call ld_path,twolame,libtwolame/.libs) \
		$(call ld_path,lame,libmp3lame/.libs) \
		$(call ld_path,libaom,usr/local/lib) \
		$(call ld_path,dav1d,usr/local/lib) \
		$(call ld_path,libwebp,usr/local/lib) \
		$(call ld_path,openjpeg,bin) \
		$(call ld_path,opus,.libs) \
		$(call ld_path,libogg,lib/.libs) \
		$(call ld_path,libvorbis,lib/.libs) \
		$(call ld_path,libtheora,lib/.libs) \
		$(call ld_path,libvpx) \
		$(call ld_path,x264) \
		$(call ld_path,x265) \
		$(shared_libs) \
		-Wl,--end-group -lm -lstdc++ -pthread \
		$(EXTRA_LIBS)" $(FFMPEG_EXTRA_CFG) \

#cmake_config=echo "exec cmake \$$$$@ $(1)" > ./configure; chmod +x ./configure;
cmake_config=echo 'cmake "$$$$@" "$(1)"' > ./configure; chmod +x ./configure;
libbthread.cfg_vars?= echo "exec true" > ./configure; chmod +x ./configure;
a52dec.mak_params?= ; cd $(call bld_path,a52dec,include); ln -sf . a52dec
a52dec.cfg_vars?= CFLAGS+=" -U__FreeBSD__ $(call inc_path,djbfft)" LIBS+=" $(call ld_path,djbfft)"
a52dec.cfg_params?=--enable-djbfft
djbfft.cfg_vars?=echo "$(call bld_path,djbfft)" > conf-home; \
 (CFLAGS="$(CFLAGS)"; $(CFG_VARS)$(if $(CFG_VARS),; )echo "$(CC) $$$$CFLAGS") > conf-cc; \
 echo > ./configure; chmod +x ./configure;
djbfft.mak_params?=; cd $(call bld_path,djbfft); ln -sf djbfft.a libdjbfft.a
audiofile.cfg_params?=--enable-shared=no
audiofile.mak_params?=LIBS="-lm -lstdc++"
flac.cfg_params?= --enable-shared=no
flac.cflags?="$(call inc_path,libogg) $(call ld_path,libogg,src/.libs)"
giflib.cfg_params=echo "exec true" > ./configure; chmod +x ./configure;
lame.cfg_vars?= CFLAGS+=" -O"
lame.cfg_params?=--enable-shared=no
lame.mak_params?= ; cd $(call bld_path,lame,include); ln -sf . lame
libaom.cfg_vars?= mkdir aom.build && cd aom.build && $(call cmake_config,..)
libaom.cfg_params?=  -DENABLE_TESTS=no -DCMAKE_BUILD_TYPE=Release -DENABLE_DOCS=no -DENABLE_SHARED=no -DCMAKE_INSTALL_LIBDIR=lib \
 -DENABLE_EXAMPLES=no -DCMAKE_INSTALL_PREFIX=$(call bld_path,libaom)/usr/local
libaom.mak_params?= ; $(MAKE) -C libaom*/aom.build install
dav1d.cfg_vars?=echo "echo dav1d custom make" >> configure; chmod +x ./configure;
dav1d.mak_params?=; $(MAKE) -C dav1d* install DESTDIR="$(call bld_path,dav1d)"
libwebp.cfg_vars?= mkdir build && cd build && $(call cmake_config,..)
libwebp.mak_params?= -C build all install DESTDIR=$(call bld_path,libwebp)
mjpegtools.cflags?="$(call inc_path,libjpeg) $(call ld_path,libjpeg,build)"
mjpegtools.cfg_vars?= ./autogen.sh; 
mjpegtools.cfg_params?= --enable-shared=no --without-libquicktime --without-libdv \
 --without-libpng --without-dga --without-gtk --without-libsdl --without-sdlgfx
mjpegtools.mak_params?= all
ladspa.cfg_vars?= CFLAGS+=' -Dinline="" '
ladspa.mak_params?=; $(MAKE) -C ladspa* install DESTDIR=$(call bld_path,ladspa)
libavc1394.cfg_vars?=PKG_CONFIG_PATH=$(call bld_path,libraw1394)
libavc1394.cflags?="$(call inc_path,libraw1394)"
libavc1394.ldflags?="$(call ld_path,libraw1394,src/.libs)"
libavc1394.cfg_params?= --enable-shared=no
libavc1394.mak_params?=; cd $(call bld_path,libavc1394); ln -sf ../librom1394/rom1394.h libavc1394/.
libdv.cfg_params?= --disable-gtk --enable-shared=no
libiec61883.cfg_vars?=PKG_CONFIG_PATH=$(call bld_path,libraw1394)
libiec61883.cflags?="$(call inc_path,libraw1394)"
libiec61883.ldflags?="$(call ld_path,libraw1394,src/.libs)"
libiec61883.cfg_params?= --enable-shared=no
libiec61883.mak_params?=; cd $(call bld_path,libiec61883,src); ln -sf . libiec61883
libjpeg.cfg_vars?= mkdir build && cd build && $(call cmake_config,..)
libjpeg.cfg_params?= -DENABLE_SHARED=no -DCMAKE_INSTALL_LIBDIR=lib 
libjpeg.mak_params?= -C build all install DESTDIR=$(call bld_path,libjpeg)
libogg.cfg_params?= --enable-shared=no
libraw1394.cfg_params?=  --enable-shared=no; ln -sf src libraw1394
libtheora.cfg_vars?=PKG_CONFIG_PATH=$(call bld_path,libogg):$(call bld_path,libvorbis)
libtheora.cflags?="$(call inc_path,libogg) $(call inc_path,libvorbis)"
libtheora.ldflags?="$(call ld_path,libvorbis,lib/.libs) $(call ld_path,libogg,src/.libs)"
libtheora.cfg_params?= --disable-examples --disable-spec --enable-shared=no
libuuid.cfg_params?=--enable-shared=no
libvorbis.cfg_params?= --disable-oggtest --enable-shared=no
openjpeg.cfg_params?= -DBUILD_SHARED_LIBS:BOOL=OFF
openjpeg.cfg_vars?=$(call cmake_config,.)
openjpeg.mak_params?= ; cd $(call bld_path,openjpeg,src/lib/openjp2); ln -sf . openjpeg-2.1
opencv.cfg_vars?=$(call cmake_config,.)
opencv.cfg_params?= -DBUILD_SHARED_LIBS:BOOL=OFF
openexr.cfg_vars?=true; \#
openexr.mak_vars?=true; \#
opus.cfg_params?= --disable-extra-programs
speech_tools.mak_params?=-j1
tiff.cfg_vars+=LIBS+=" -lpthread"
tiff.cfg_params+= --enable-shared=no --disable-zstd $(call if_pkg,libwebp,\
 --with-webp-include-dir=$(call pkg_incl,libwebp)\
 --with-webp-lib-dir=$(call pkg_libs,libwebp))\
 $(call if_npkg,libwebp,--disable-webp)
twolame.cfg_params?=--enable-shared=no
x264.cfg_params?= --enable-static --enable-pic
x265.cfg_vars?=$(call cmake_config,source)
x265.cfg_params?= -DENABLE_SHARED=no
libvpx.cfg_params?= --enable-pic --disable-avx512 --disable-examples --disable-unit_tests

DS:=$$$$$$$$
pkg_cfg=$(call bld_path,$(1),usr/local/lib/pkgconfig):
LV2_PKGCFG:=$(subst : ,:,$(foreach p,lv2 serd sord sratom lilv suil,$(call pkg_cfg,$(p))))

# lv2
define waf-setup
$(1).cfg_vars+=./waf --version; find .waf* -type f -name "*.py" | \
 while read f; do sed -e '/StopIter/d' -i "$(DS)f"; done; \
 echo 'exec ./waf configure "$(DS)@"' > ./configure; chmod +x \
 ./configure;  PKG_CONFIG_PATH=$(LV2_PKGCFG):$(PKG_CONFIG_PATH)  LD_LIBRARY_PATH=$(LV2_LIBPATH)
$(1).cfg_params+=\
 CFLAGS="-I$(call bld_path,lv2) $(foreach f,$(2), $(call inc_path,$(f))) -fPIC -std=c99"\
 LDFLAGS="-lm `pkg-config --libs x11` $(foreach f,$(2), $(call ld_path,$(f),usr/local/lib)) "
$(1).mak_vars=(echo "all:"; echo "	./waf -v build") > $(call bld_path,$(1),Makefile); 
$(1).mak_params+=; ( cd $(1)*; ./waf install --destdir=$(call bld_path,$(1)) )
endef

SYS_LIB:=$(notdir $(lastword $(wildcard /usr/lib /usr/lib32 /usr/lib64)))
SYS_LV2:=$(lastword $(wildcard /usr/$(SYS_LIB)/lv2 /usr/local/$(SYS_LIB)/lv2))
$(if $(SYS_LV2),,$(eval SYS_LV2:=/usr/$(SYS_LIB)/lv2))
lv2.cfg_params?= --lv2dir=$(SYS_LV2)
SYS_LV2_PATH:=/usr/$(SYS_LIB)/lv2: /usr/local/$(SYS_LIB)/lv2:
DFT_LV2_PATH:= /usr/lib64/lv2: /usr/lib/lv2: /usr/local/lib64/lv2: /usr/local/lib/lv2: 
LV2_PATH:=$(subst : ,:,$(SYS_LV2_PATH)$(filter-out $(SYS_LV2_PATH),$(DFT_LV2_PATH)))
lilv.cfg_params?= --static --default-lv2-path=$(LV2_PATH)
sratom.cfg_params?= --static --no-shared
serd.cfg_params?= --static --no-shared
sord.cfg_params?= --static --no-shared
lilv.cfg_params?= --static --dyn-manifest
suil.cfg_params?= --static --no-qt --no-qt5

$(eval $(call waf-setup,lv2))
$(eval $(call waf-setup,serd))
$(eval $(call waf-setup,sord, serd))
$(eval $(call waf-setup,sratom, serd sord lv2))
$(eval $(call waf-setup,lilv, lv2 sratom serd sord))
$(eval $(call waf-setup,suil, lv2))

# add softlinks for legacy paths
waf_link=$(1).mak_params+=; (cd $(1)*; ln -s $(1)-0/$(1) usr/local/include/.)
$(foreach p,sratom serd sord lilv suil,$(eval $(call waf_link,$(p))))

# compile flag vars
$(foreach tgt,$(TARGETS), \
	$(if $($(tgt).cflags),$(eval $(tgt).cfg_vars+=CFLAGS+=$($(tgt).cflags))))
$(foreach tgt,$(TARGETS), \
	$(if $($(tgt).cppflags),$(eval $(tgt).cfg_vars+=CPPFLAGS+=$($(tgt).cppflags))))
$(foreach tgt,$(TARGETS), \
	$(if $($(tgt).ldflags),$(eval $(tgt).cfg_vars+=LDFLAGS+=$($(tgt).ldflags))))

# build rules
.PHONY:	rules
rules:

#$(eval $(call std-build,pkg,deps...))

ifeq ($(MAKECMDGOALS),rules)
rules=$(info $(1))
else
rules=$(eval $(1))
endif

uname := $(shell uname -o)
ifeq ($(uname), Android)
$(call rules,$(call std-build,libbthread))
endif
$(call rules,$(call std-build,a52dec,djbfft))
$(call rules,$(call std-build,djbfft))
$(call rules,$(call std-build,audiofile))
$(call rules,$(call std-build,encore))
$(call rules,$(call std-build,esound,audiofile))
$(call rules,$(call std-build,ffmpeg, twolame lame openjpeg opus \
	libtheora x264 x265 libvpx libaom dav1d libwebp \
	$(call if_want,NV, ffnvcodec)))
$(call rules,$(call std-build,fftw))
$(call rules,$(call std-build,flac,libogg))
$(call rules,$(call std-build,giflib))
$(call rules,$(call std-build,ilmBase, openexr))
$(call rules,$(call std-build,ladspa))
$(call rules,$(call std-build,lame))
$(call rules,$(call std-build,libaom))
$(call rules,$(call std-build,dav1d))
$(call rules,$(call std-build,libwebp))
$(call rules,$(call std-build,libavc1394,libraw1394))
$(call rules,$(call std-build,libdv))
$(call rules,$(call std-build,libiec61883,libraw1394))
$(call rules,$(call std-build,libjpeg))
$(call rules,$(call std-build,libogg))
$(call rules,$(call std-build,libraw1394))
$(call rules,$(call std-build,libsndfile))
$(call rules,$(call std-build,libtheora, libogg libvorbis))
$(call rules,$(call std-build,libuuid))
$(call rules,$(call std-build,libvorbis))
$(call rules,$(call std-build,mjpegtools, libjpeg))
$(call rules,$(call std-build,opencv))
$(call rules,$(call std-build,openexr))
$(call rules,$(call std-build,openExr, ilmBase))
$(call rules,$(call std-build,openjpeg))
$(call rules,$(call std-build,opus))
$(call rules,$(call std-build,speech_tools))
$(call rules,$(call std-build,tiff, libwebp))
$(call rules,$(call std-build,twolame))
$(call rules,$(call std-build,x264))
$(call rules,$(call std-build,x265))
$(call rules,$(call std-build,libvpx))
$(call rules,$(call std-build,lv2))
$(call rules,$(call std-build,serd))
$(call rules,$(call std-build,sord, serd))
$(call rules,$(call std-build,sratom, serd sord lv2))
$(call rules,$(call std-build,lilv, lv2 sratom serd sord))
$(call rules,$(call std-build,suil, lv2))
$(call rules,$(call std-build,ffnvcodec))

# specialize festival, multiple tarballs
festival:
.PHONY: festival festival-configure festival-src
festival: $(call pkg-built,festival)
festival-configure: $(call pkg-config,festival)
festival-src: $(call pkg-source,festival)
festival-clean:
	rm -rf festival* $(BLD)/festival.*

$(call pkg-source,festival):
	for f in src/festival* src/festlex_* src/festvox_*; do tar -xJf $$f; done
	touch $@

$(call pkg-config,festival): $(call pkg-source,festival) $(call pkg-built,speech_tools)
	cd festival* && [ -f ./configure ] && \
		./configure
	touch $@

$(call pkg-built,festival): $(call pkg-config,festival)
	$(MAKE) -C festival* -j1
	touch $@

clean:
	for f in $(static_pkgs); do rm -rf $$f*; done
	rm -rf $(BLD)

install:
	cp -a $(ver_mjpegtools)/mpeg2enc/mpeg2enc $(BINDIR)/.
	cp -a $(ver_mjpegtools)/mplex/mplex $(BINDIR)/.
else
# not WANT_CIN_3RDPARTY
all:
clean:
install:

endif
val-%:
	@echo $($(subst val-,,$@))

