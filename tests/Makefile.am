
DBUS_RUNNER=../src/dbus-test-runner --dbus-config $(srcdir)/../data/session.conf
TESTS = 
DISTCLEANFILES = $(TESTS)

TESTS += test-simple
test-simple:
	@echo "#!/bin/sh" > $@
	@echo $(DBUS_RUNNER) --task true >> $@
	@chmod +x $@

TESTS += test-manytask
test-manytask:
	@echo "#!/bin/sh" > $@
	@echo $(DBUS_RUNNER) --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true >> $@
	@chmod +x $@

TESTS += test-ignore
test-ignore:
	@echo "#!/bin/sh" > $@
	@echo $(DBUS_RUNNER) --task false --ignore-return >> $@
	@chmod +x $@

TESTS += test-ignore-second
test-ignore-second:
	@echo "#!/bin/sh" > $@
	@echo $(DBUS_RUNNER) --task true --task false --ignore-return >> $@
	@chmod +x $@

TESTS += test-invert
test-invert:
	@echo "#!/bin/sh" > $@
	@echo $(DBUS_RUNNER) --task false --invert-return >> $@
	@chmod +x $@

TESTS += test-invert-second
test-invert-second:
	@echo "#!/bin/sh" > $@
	@echo $(DBUS_RUNNER) --task true --task false --invert-return >> $@
	@chmod +x $@

TESTS += test-param
test-param:
	@echo "#!/bin/sh" > $@
	@echo $(DBUS_RUNNER) --task \"$(srcdir)/delayrm.sh\" --parameter \"$(builddir)/bob\" --task touch --parameter \"$(builddir)/bob\" --ignore-return >> $@
	@chmod +x $@

EXTRA_DIST = \
	delayrm.sh
