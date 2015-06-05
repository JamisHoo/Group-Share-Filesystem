vpath %.cc src
vpath %.h src


# BUILDDIR should end up with /
BUILDDIR = build/

SOURCES = $(shell cd src; ls *.cc)
OBJECTS = $(patsubst %.cc,%.o,$(SOURCES))


FSFLAGS = -D_FILE_OFFSET_BITS=64

CXXFLAGS += -std=c++11 -Wextra $(FSFLAGS)

DYLIB = pthread ssh osxfuse boost_system boost_filesystem boost_serialization boost_program_options boost_thread

LDFLAGS += $(addprefix -l,$(DYLIB))

.PHONY: all
all: $(BUILDDIR) gsfs

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Auto-Dependency Generation
$(BUILDDIR)%.o : %.cc 
	$(CXX) $(CXXFLAGS) -MMD -c -o $@ $<
	@cp $(BUILDDIR)$*.d $(BUILDDIR)$*.P; \
		sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(BUILDDIR)$*.d >> $(BUILDDIR)$*.P; \
		rm -f $(BUILDDIR)$*.d

-include $(BUILDDIR)*.P

gsfs: $(addprefix $(BUILDDIR),$(OBJECTS))
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@


.PHONY: clean
clean:
	$(RM) -r $(BUILDDIR)

.PHONY: cleanall
cleanall: clean
	$(RM) gsfs
