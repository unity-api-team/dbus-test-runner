check: tests

DBUS_RUNNER=../src/dbus-test-runner --dbus-config ../data/session.conf

tests: test-simple test-manytask test-ignore test-ignore-second test-invert test-invert-second

test-simple:
	$(DBUS_RUNNER) --task true

test-manytask:
	$(DBUS_RUNNER) --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true --task true

test-ignore:
	$(DBUS_RUNNER) --task false --ignore-return

test-ignore-second:
	$(DBUS_RUNNER) --task true --task false --ignore-return

test-invert:
	$(DBUS_RUNNER) --task false --invert-return

test-invert-second:
	$(DBUS_RUNNER) --task true --task false --invert-return

