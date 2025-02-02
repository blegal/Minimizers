./BreiZHMinimizer -d ../tests/merger/test_files_2 --skip-minimizer-step  > /dev/null 2>&1

./raw_dump    -f result.2c -c 2 > /dev/null 2>&1

./color_stats -f result.2c -c 2 > /dev/null 2>&1

./extract_color -i result.2c -c 2 -o color_0 -s 0 > /dev/null 2>&1
./extract_color -i result.2c -c 2 -o color_1 -s 1 > /dev/null 2>&1

diff color_0 ../tests/merger/test_files_2/testf_000.raw
diff color_1 ../tests/merger/test_files_2/testf_001.raw
