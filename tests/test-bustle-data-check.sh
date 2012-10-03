#!/bin/bash -e

[ `grep ^sig $1 | grep dbustestrunner | wc -l` -eq $2 ]
