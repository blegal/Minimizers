./merge ./testf_0    ./testf_1    ./testf_d0.2c 1
./merge ./testf_2    ./testf_3    ./testf_d1.2c 1
./merge ./testf_4    ./testf_5    ./testf_d2.2c 1
./merge ./testf_6    ./testf_7    ./testf_d3.2c 1
./merge ./testf_8    ./testf_9    ./testf_d4.2c 1
./merge ./testf_a    ./testf_b    ./testf_d5.2c 1
./merge ./testf_c    ./testf_d    ./testf_d6.2c 1
./merge ./testf_e    ./testf_f    ./testf_d7.2c 1

./merge ./testf_d0.2c ./testf_d1.2c ./testf_d0.4c 2
./merge ./testf_d2.2c ./testf_d3.2c ./testf_d1.4c 2
./merge ./testf_d4.2c ./testf_d5.2c ./testf_d2.4c 2
./merge ./testf_d6.2c ./testf_d7.2c ./testf_d3.4c 2

./merge ./testf_d0.4c ./testf_d1.4c ./testf_d0.8c 4
./merge ./testf_d2.4c ./testf_d3.4c ./testf_d1.8c 4

./merge ./testf_d0.8c ./testf_d1.8c ./testf_d0.16c 8

./merge ./testf_d0.8c ./testf_d1.8c ./testf_d0.64c  32

./merge ./testf_d0.8c ./testf_d1.8c ./testf_d0.128c 64

./merge ./testf_d0.128c ./testf_d0.128c ./testf_d0.256c 128

./raw_dump ./testf_1    0
echo "###################################################"
./raw_dump ./testf_d0.2c 2
echo "###################################################"
./raw_dump ./testf_d0.4c 4
echo "###################################################"
./raw_dump ./testf_d0.8c 8
echo "###################################################"
./raw_dump ./testf_d0.16c 16
echo "###################################################"
./raw_dump ./testf_d0.64c 64
echo "###################################################"
./raw_dump ./testf_d0.128c 128
echo "###################################################"
./raw_dump ./testf_d0.256c 256
echo "###################################################"
