#!/bin/bash
rm trace_0.raw
# touch trace_0.raw
# rm mypipe
mkfifo trace_0.raw
mkfifo mem-dump.out
#---------------------------------

gzip -c < trace_0.raw > trace_0.raw.gz & 
gzip -c < mem-dump.out > mem-dump.out.gz & 
# pin -t MemAddr.so  -- /home/nvedula/summer2016/mem-axc/pin-getmemaxc-memaddr/gzip-test-noprint/164.gzip-mwe-1
# pin -t MemAddr.so  -- /home/vnaveen0/vm-shared/work/workloads-bc-dot/164.gzip/164.gzip-mwe-1
pin -t obj-intel64/obj-intel64/MemAddr.so -- /mnt/D4DA3EA3DA3E8232/Users/naveen/wind_drive/sfu/cnn_hardware/pin-tools/src/sudoku/a.out 4 /mnt/D4DA3EA3DA3E8232/Users/naveen/wind_drive/sfu/cnn_hardware/pin-tools/src/sudoku/inputs/4by4.txt  


# pin -t MemAddr.so  -- /home/nvedula/summer2016/mem-axc/pin-getmemaxc-memaddr/ablackscholes/blackscholes-mwe-1179508 1 /home/nvedula/summer2016/mem-axc/macsim-noruby/test/blackscholes/data/input2.txt  o.txt 
# trace_0.raw > mypipe
