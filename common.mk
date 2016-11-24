DEPDIR = .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d
CFLAGS+=-MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td
CXXFLAGS+=-MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td
OBJECTS = $(patsubst %,%.o,$(basename $(SOURCES)))

ALLPRODUCTS += $(OBJECTS)

define BINARY_template
$(1): $$(EXTRA_DEPS) $(1).o $$($(1)_OBJS)
	$$(CC) $$(LDFLAGS) -o $$@ $(1).o $$($(1)_OBJS) $$(LDLIBS)

$(1).S: $$(EXTRA_DEPS) $(1).c
	$$(CC) -S -o $$@ $$(CFLAGS) $$<

ALLPRODUCTS += $(1).o $$($(1)_OBJS) $(1).S $(1) $(DEPDIR)/.d/$(1).d
endef

%.o : %.c
%.o : %.c $(DEPDIR)/%.d
	$(CC) -o $@ -c $< $(CFLAGS)
	@$(POSTCOMPILE)

%.o : %.cc
%.o : %.cc $(DEPDIR)/%.d
	$(CXX) -o $@ -c $< $(CXXFLAGS)
	@$(POSTCOMPILE)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

clean: defaultclean
defaultclean:
	rm -f $(ALLPRODUCTS) core

-include $(patsubst %,$(DEPDIR)/%.d,$(basename $(SOURCES)))
