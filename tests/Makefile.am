
DBUS_RUNNER=../src/dbus-test-runner --dbus-config $(srcdir)/../data/session.conf
TESTS = 
DISTCLEANFILES = $(TESTS)

TESTS += test-simple
test-simple: Makefile.am
	@echo "#!/bin/sh" > $@
	@echo $(DBUS_RUNNER) --task true >> $@
	@chmod +x $@

TESTS += test-manytask
test-manytask: Makefile.am
	@echo "#!/bin/sh" > $@
	@echo $(DBUS_RUNNER) --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true >> $@
	@chmod +x $@

TESTS += test-ignore
test-ignore: Makefile.am
	@echo "#!/bin/sh" > $@
	@echo $(DBUS_RUNNER) --task false --ignore-return >> $@
	@chmod +x $@

TESTS += test-ignore-second
test-ignore-second: Makefile.am
	@echo "#!/bin/sh" > $@
	@echo $(DBUS_RUNNER) --task true --task false --ignore-return >> $@
	@chmod +x $@

TESTS += test-invert
test-invert: Makefile.am
	@echo "#!/bin/sh" > $@
	@echo $(DBUS_RUNNER) --task false --invert-return >> $@
	@chmod +x $@

TESTS += test-invert-second
test-invert-second: Makefile.am
	@echo "#!/bin/sh" > $@
	@echo $(DBUS_RUNNER) --task true --task false --invert-return >> $@
	@chmod +x $@

TESTS += test-param
test-param: Makefile.am
	@echo "#!/bin/sh" > $@
	@echo $(DBUS_RUNNER) --task \"$(srcdir)/delayrm.sh\" --parameter \"$(builddir)/bob\" --task touch --parameter \"$(builddir)/bob\" --ignore-return >> $@
	@chmod +x $@

TESTS += test-output
test-output: Makefile.am
	@echo "#!/bin/sh -e" > $@
	@echo "$(DBUS_RUNNER) --task cat --parameter \"$(top_srcdir)/src/dbus-test-runner.c\" --task-name \"cat1\" --task cat --parameter \"$(top_srcdir)/src/dbus-test-runner.c\" --task-name \"cat2\" > testcat.output.txt" >> $@
	@echo "echo Finding cat1 data" >> $@
	@echo "grep ^cat1: testcat.output.txt > testcat.output.cat1.txt" >> $@
	@echo "echo Finding cat2 data" >> $@
	@echo "grep ^cat2: testcat.output.txt > testcat.output.cat2.txt" >> $@
	@echo "echo Filtering cat1 data" >> $@
	@echo "sed -e s/cat1:\\ //g testcat.output.cat1.txt > testcat.output.cat1.filtered.txt" >> $@
	@echo "echo Filtering cat2 data" >> $@
	@echo "sed -e s/cat2:\\ //g testcat.output.cat2.txt > testcat.output.cat2.filtered.txt" >> $@
	@echo "echo Verifying cat 1" >> $@
	@echo "diff testcat.output.cat1.filtered.txt \"$(top_srcdir)/src/dbus-test-runner.c\" > /dev/null" >> $@
	@echo "echo Verifying cat 2" >> $@
	@echo "diff testcat.output.cat2.filtered.txt \"$(top_srcdir)/src/dbus-test-runner.c\" > /dev/null" >> $@
	@chmod +x $@
DISTCLEANFILES += testcat.output.txt testcat.output.cat1.txt testcat.output.cat2.txt testcat.output.cat1.filtered.txt testcat.output.cat2.filtered.txt

EXTRA_DIST = \
	delayrm.sh
