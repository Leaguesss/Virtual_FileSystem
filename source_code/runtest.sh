#!/bin/bash

cd ~

cp -rf reset_testcase/ testcase/

cd testcase

mv reset_testcase/* .

cd ..

gcc runtest.c -o runtest myfilesystem.c selffunctions.c

./runtest


