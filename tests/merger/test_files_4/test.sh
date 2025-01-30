./BreiZHMinimizer -d ../tests/merger/test_files_4 --skip-minimizer-step

./raw_dump    -f data_n0.4c -c 4

./color_stats -f data_n0.4c -c 4

./extract_color -i data_n0.4c -c 4 -o color_0 -s 0
./extract_color -i data_n0.4c -c 4 -o color_1 -s 1
./extract_color -i data_n0.4c -c 4 -o color_2 -s 2
./extract_color -i data_n0.4c -c 4 -o color_3 -s 3

diff color_0 ../tests/merger/test_files_4/testf_000.raw
diff color_1 ../tests/merger/test_files_4/testf_001.raw
diff color_2 ../tests/merger/test_files_4/testf_002.raw
diff color_3 ../tests/merger/test_files_4/testf_003.raw
