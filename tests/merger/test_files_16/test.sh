./BreiZHMinimizer -d ../tests/merger/test_files_16 --skip-minimizer-step

./raw_dump    -f data_n0.16c -c 16

./color_stats -f data_n0.16c -c 16

./extract_color -i data_n0.16c -c 16 -o color_0 -s 0
./extract_color -i data_n0.16c -c 16 -o color_1 -s 1
./extract_color -i data_n0.16c -c 16 -o color_2 -s 2
./extract_color -i data_n0.16c -c 16 -o color_3 -s 3
./extract_color -i data_n0.16c -c 16 -o color_4 -s 4
./extract_color -i data_n0.16c -c 16 -o color_5 -s 5
./extract_color -i data_n0.16c -c 16 -o color_6 -s 6
./extract_color -i data_n0.16c -c 16 -o color_7 -s 7
./extract_color -i data_n0.16c -c 16 -o color_8 -s 8
./extract_color -i data_n0.16c -c 16 -o color_9 -s 9
./extract_color -i data_n0.16c -c 16 -o color_10 -s 10
./extract_color -i data_n0.16c -c 16 -o color_11 -s 11
./extract_color -i data_n0.16c -c 16 -o color_12 -s 12
./extract_color -i data_n0.16c -c 16 -o color_13 -s 13
./extract_color -i data_n0.16c -c 16 -o color_14 -s 14
./extract_color -i data_n0.16c -c 16 -o color_15 -s 15

diff color_0 ../tests/merger/test_files_16/testf_000.raw
diff color_1 ../tests/merger/test_files_16/testf_001.raw
diff color_2 ../tests/merger/test_files_16/testf_002.raw
diff color_3 ../tests/merger/test_files_16/testf_003.raw
diff color_4 ../tests/merger/test_files_16/testf_004.raw
diff color_5 ../tests/merger/test_files_16/testf_005.raw
diff color_6 ../tests/merger/test_files_16/testf_006.raw
diff color_7 ../tests/merger/test_files_16/testf_007.raw
diff color_8 ../tests/merger/test_files_16/testf_008.raw
diff color_9 ../tests/merger/test_files_16/testf_009.raw
diff color_10 ../tests/merger/test_files_16/testf_010.raw
diff color_11 ../tests/merger/test_files_16/testf_011.raw
diff color_12 ../tests/merger/test_files_16/testf_012.raw
diff color_13 ../tests/merger/test_files_16/testf_013.raw
diff color_14 ../tests/merger/test_files_16/testf_014.raw
diff color_15 ../tests/merger/test_files_16/testf_015.raw
