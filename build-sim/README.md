pin -t /local-scratch/changan/nvedula/cnn_hardware/SkimCaffe/simulator/pin-tool/inscount/obj-intel64/opcodemix.so --  ../../build/tools/caffe.bin test -model ../../models/bvlc_reference_caffenet/test_direct_sconv_mkl.prototxt -weights ../../models/bvlc_reference_caffenet/logs/acc_57.5_0.001_5e-5_ft_0.001_5e-5/0.001_5e-05_0_1_0_0_0_0_Sun_Jan__8_07-35-54_PST_2017/caffenet_train_iter_640000.caffemodel  -iterations 1

(no sparse conv)
build/tools/caffe.bin test -model models/bvlc_reference_caffenet/train_val.prototxt -weights models/bvlc_reference_caffenet/logs/acc_57.5_0.001_5e-5_ft_0.001_5e-5/0.001_5e-05_0_1_0_0_0_0_Sun_Jan__8_07-35-54_PST_2017/caffenet_train_iter_640000.caffemodel



pin -t /local-scratch/changan/nvedula/cnn_hardware/SkimCaffe/simulator/pin-tool/inscount/obj-intel64/opcodemix.so -- ../../build/tools/caffe.bin test -model ../../models/bvlc_reference_caffenet/train_val.prototxt -weights ../../models/bvlc_reference_caffenet/logs/acc_57.5_0.001_5e-5_ft_0.001_5e-5/0.001_5e-05_0_1_0_0_0_0_Sun_Jan__8_07-35-54_PST_2017/caffenet_train_iter_640000.caffemodel

 
