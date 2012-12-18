#!/bin/bash -e

[ `bustle --count $1 | cut -d " " -f 4` -eq $2 ]
