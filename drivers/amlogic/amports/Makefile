#
# Makefile for the linux kernel.
#

# Object file lists.

ifeq ($(CONFIG_ARCH_MESON1),y)
meson-arch := m1
endif
ifeq ($(CONFIG_ARCH_MESON2),y)
meson-arch := m2
endif
ifeq ($(CONFIG_ARCH_MESON3),y)
meson-arch := m3
endif
ifeq ($(CONFIG_ARCH_MESON6),y)
meson-arch := m6
endif
ifeq ($(CONFIG_ARCH_MESON6TV),y)
meson-arch := m6
endif

obj-$(CONFIG_AM_STREAMING) += amports.o
ifeq ($(CONFIG_ARCH_MESON6),y)

amports-objs := amstream.o streambuf.o vdec.o adec.o tsdemux.o psparser.o esparser.o rmparser.o amvdec.o amstream_profile.o
amports-y    := amstream.o streambuf.o vdec.o adec.o tsdemux.o psparser.o esparser.o rmparser.o amvdec.o amstream_profile.o

else

ifeq ($(CONFIG_ARCH_MESON6TV),y)
amports-objs := amstream.o streambuf.o vdec.o adec.o tsdemux.o psparser.o esparser.o rmparser.o amvdec.o amstream_profile.o
amports-y    := amstream.o streambuf.o vdec.o adec.o tsdemux.o psparser.o esparser.o rmparser.o amvdec.o amstream_profile.o
else
amports-objs := amstream.o streambuf.o vdec.o adec.o tsdemux.o psparser.o esparser.o rmparser.o amvdec.o amve.o amcm.o amstream_profile.o
amports-y    := amstream.o streambuf.o vdec.o adec.o tsdemux.o psparser.o esparser.o rmparser.o amvdec.o amve.o amcm.o amstream_profile.o
endif

endif

obj-$(CONFIG_AM_VDEC_MPEG12) += amvdec_mpeg12.o
amvdec_mpeg12-objs := vmpeg12.o ${meson-arch}/ucode/mpeg12/vmpeg12_mc.o
amvdec_mpeg12-y    := vmpeg12.o ${meson-arch}/ucode/mpeg12/vmpeg12_mc.o

obj-$(CONFIG_AM_VDEC_MPEG4) += amvdec_mpeg4.o
amvdec_mpeg4-objs := vmpeg4.o ${meson-arch}/ucode/mpeg4/vmpeg4_mc.o
amvdec_mpeg4-y    := vmpeg4.o ${meson-arch}/ucode/mpeg4/vmpeg4_mc.o

obj-$(CONFIG_AM_VDEC_VC1) += amvdec_vc1.o
amvdec_vc1-objs := vvc1.o ${meson-arch}/ucode/vc1/vc1_mc.o
amvdec_vc1-y    := vvc1.o ${meson-arch}/ucode/vc1/vc1_mc.o

obj-$(CONFIG_AM_VDEC_H264) += amvdec_h264.o
amvdec_h264-objs := vh264.o ${meson-arch}/ucode/h264/vh264_mc.o
amvdec_h264-y    := vh264.o ${meson-arch}/ucode/h264/vh264_mc.o

obj-$(CONFIG_AM_VDEC_H264MVC) += amvdec_h264mvc.o
amvdec_h264mvc-objs := vh264mvc.o ${meson-arch}/ucode/h264mvc/vh264mvc_mc.o
amvdec_h264mvc-y    := vh264mvc.o ${meson-arch}/ucode/h264mvc/vh264mvc_mc.o

obj-$(CONFIG_AM_VDEC_MJPEG) += amvdec_mjpeg.o
amvdec_mjpeg-objs := vmjpeg.o ${meson-arch}/ucode/mjpeg/vmjpeg_mc.o
amvdec_mjpeg-y    := vmjpeg.o ${meson-arch}/ucode/mjpeg/vmjpeg_mc.o

obj-$(CONFIG_AM_VDEC_REAL) += amvdec_real.o
amvdec_real-objs := vreal.o ${meson-arch}/ucode/real/vreal_mc.o
amvdec_real-y    := vreal.o ${meson-arch}/ucode/real/vreal_mc.o

obj-$(CONFIG_AM_JPEGDEC) += jpegdec.o ${meson-arch}/ucode/jpeg/jpeg_mc.o

obj-$(CONFIG_AM_ENCODER) += encoder.o ${meson-arch}/ucode/encoder/encoder_mc.o
obj-$(CONFIG_AM_VIDEO) += amvideo.o
amvideo-objs := video.o vpp.o
amvideo-y    := video.o vpp.o

obj-$(CONFIG_AM_VIDEO2) += amvideo2.o
amvideo2-objs := video2.o vpp2.o
amvideo2-y    := video2.o vpp2.o

obj-$(CONFIG_AM_DEINTERLACE) += deinterlace.o

obj-$(CONFIG_AM_TIMESYNC) += amtsync.o
amtsync-objs := tsync.o timestamp.o
amtsync-y    := tsync.o timestamp.o

obj-$(CONFIG_AM_PTSSERVER) += ptsserv.o

obj-$(CONFIG_AM_SUBTITLE) += subtitle.o

ifneq ($(CONFIG_AM_DVB),)
EXTRA_CFLAGS = -DENABLE_DEMUX_DRIVER -DMODIFY_TIMESTAMP_INC_WITH_PLL -DCALC_CACHED_TIME #-DUSE_VMASTER_MODE
endif
