#!/bin/bash

#Using pin3.0 with gcc-5


rm trace_0.raw
rm mem-dump.out 
# touch trace_0.raw
# rm mypipe
mkfifo trace_0.raw
mkfifo mem-dump.out
#---------------------------------

gzip -c < trace_0.raw > trace_0.raw.gz & 
gzip -c < mem-dump.out > mem-dump.out.gz & 
pin -t /home/vnaveen0/wind_drive/sfu/nachos/mem-axc-64/tools/pin-tools/stack-vs-heap-func/obj-intel64/MemTrace.so  -- ./test_xor -m ./xor.model 
# pin -t /home/vnaveen0/wind_drive/sfu/nachos/mem-axc-64/tools/pin-tools/pin-ex/obj-intel64/opcodemix.so  -- ./test_xor -m ./xor.model


# trace_0.raw > mypipe
