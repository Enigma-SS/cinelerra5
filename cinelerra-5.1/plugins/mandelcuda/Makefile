TOPDIR?=../..
include $(TOPDIR)/plugin_defs
include $(TOPDIR)/plugin_cuda

PLUGIN = mandelcuda

OBJS := \
	$(OBJDIR)/mandelbrot.o \
	$(OBJDIR)/mandelbrotwindow.o \
	$(OBJDIR)/mandelcuda.o \

include $(TOPDIR)/plugin_config

$(OBJDIR)/mandelbrot.o: mandelbrot.C  mandelbrot.h 
$(OBJDIR)/mandelbrotwindow.o: mandelbrotwindow.C  mandelbrotwindow.h
$(OBJDIR)/mandelcuda.o: mandelcuda.cu  mandelcuda.h 

