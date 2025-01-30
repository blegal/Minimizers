./BreiZHMinimizer -d ../tests/merger/test_files_8 --skip-minimizer-step

./raw_dump    -f data_n0.8c -c 8

./color_stats -f data_n0.8c -c 8

./extract_color -i data_n0.8c -c 8 -o color_0 -s 0
./extract_color -i data_n0.8c -c 8 -o color_1 -s 1
./extract_color -i data_n0.8c -c 8 -o color_2 -s 2
./extract_color -i data_n0.8c -c 8 -o color_3 -s 3
./extract_color -i data_n0.8c -c 8 -o color_4 -s 4
./extract_color -i data_n0.8c -c 8 -o color_5 -s 5
./extract_color -i data_n0.8c -c 8 -o color_6 -s 6
./extract_color -i data_n0.8c -c 8 -o color_7 -s 7

diff color_0 ../tests/merger/test_files_8/testf_000.raw
diff color_1 ../tests/merger/test_files_8/testf_001.raw
diff color_2 ../tests/merger/test_files_8/testf_002.raw
diff color_3 ../tests/merger/test_files_8/testf_003.raw
diff color_4 ../tests/merger/test_files_8/testf_004.raw
diff color_5 ../tests/merger/test_files_8/testf_005.raw
diff color_6 ../tests/merger/test_files_8/testf_006.raw
diff color_7 ../tests/merger/test_files_8/testf_007.raw
