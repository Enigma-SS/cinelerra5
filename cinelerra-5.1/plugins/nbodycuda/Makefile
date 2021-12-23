TOPDIR?=../..
include $(TOPDIR)/plugin_defs
include $(TOPDIR)/plugin_cuda

PLUGIN = nbodycuda

OBJS := \
	$(OBJDIR)/nbody.o \
	$(OBJDIR)/nbodywindow.o \
	$(OBJDIR)/nbodycuda.o \
	$(OBJDIR)/bodysys.o \
	$(OBJDIR)/renders.o \

include $(TOPDIR)/plugin_config

$(OBJDIR)/nbody.o: nbody.C  nbody.h 
$(OBJDIR)/nbodywindow.o: nbodywindow.C  nbodywindow.h
$(OBJDIR)/nbodycuda.o: nbodycuda.cu  nbodycuda.h 
$(OBJDIR)/bodysys.o: bodysys.cu
$(OBJDIR)/renders.o: renders.C
