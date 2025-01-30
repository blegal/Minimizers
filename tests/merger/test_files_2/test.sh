./BreiZHMinimizer -d ../tests/merger/test_files_2 --skip-minimizer-step

./raw_dump    -f data_n0.2c -c 2

./color_stats -f data_n0.2c -c 2

./extract_color -i data_n0.2c -c 2 -o color_0 -s 0
./extract_color -i data_n0.2c -c 2 -o color_1 -s 1

diff color_0 ../tests/merger/test_files_2/testf_000.raw
diff color_1 ../tests/merger/test_files_2/testf_001.raw
