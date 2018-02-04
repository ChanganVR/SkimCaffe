#!/bin/bash

HOME=/local-scratch/changan/nvedula/cnn_hardware/SkimCaffe/simulator/cnn_hardware

#Build Bloom Filter
mkdir -p $HOME/build/b-bloom
cd $HOME/build/b-bloom
cmake ../../bloom.s/
make 
#Note libbloom.a is created

#Build Gems LIbrary
# need python2.7,bison, flex (we built on 2.6) 
cd gems-lib-ooo/ruby_clean
make PROTOCOL=MESI_CMP_directory_m  NOTESTER=1 DEBUG=1

#should generate "libruby.so" in
# $HOME/gems-lib-ooo/ruby_clean/amd64-linux/generated/MESI_CMP_directory_m/lib


vim $HOME/mem_sim/CMakeLists.txt
# set libbloom.a folder path
# set(BLOOM_PATH /local-scratch/changan/nvedula/cnn_hardware/SkimCaffe/simulator/cnn_hardware/build/b-bloom)
mkdir -p $HOME/build/b-mem_sim
cd  $HOME/build/b-mem_sim
cmake ../../mem_sim/
make -j 8
cd bin
ln -s $HOME/gems-lib-ooo/ruby_clean/DRAM
ln -s $HOME/gems-lib-ooo/ruby_clean/network
cp -r mem_sim/src/test.gz .
./mem_sim




