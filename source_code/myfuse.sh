#!/bin/bash

gcc myfuse.c -o myfuse -lfuse myfilesystem.c selffunctions.c
