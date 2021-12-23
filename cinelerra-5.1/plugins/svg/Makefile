include ../../plugin_defs

OBJS = 	$(OBJDIR)/svg.o  \
	$(OBJDIR)/svgwin.o \
	$(OBJDIR)/new_svg.o

PLUGIN = svg

include ../../plugin_config

$(OBJDIR)/svg.o: svg.C
$(OBJDIR)/svgwin.o: svgwin.C

$(OBJDIR)/new_svg.o: new.svg
	cd $(OBJDIR); ../../../guicast/$(OBJDIR)/bootstrap $(notdir $@) ../$^
