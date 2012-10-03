#!/bin/sh
sleep 1
gdbus emit --session --object-path /test/dbustestrunner/signal --signal com.launchpad.dbustestrunner.signal
