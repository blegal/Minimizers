./BreiZHMinimizer -d ../tests/merger/test_files_4 --skip-minimizer-step > /dev/null 2>&1

./raw_dump    -f result.4c -c 4 > /dev/null 2>&1

./color_stats -f result.4c -c 4 > /dev/null 2>&1

./extract_color -i result.4c -c 4 -o color_0 -s 0 > /dev/null 2>&1
./extract_color -i result.4c -c 4 -o color_1 -s 1 > /dev/null 2>&1
./extract_color -i result.4c -c 4 -o color_2 -s 2 > /dev/null 2>&1
./extract_color -i result.4c -c 4 -o color_3 -s 3 > /dev/null 2>&1

diff color_0 ../tests/merger/test_files_4/testf_000.raw
diff color_1 ../tests/merger/test_files_4/testf_001.raw
diff color_2 ../tests/merger/test_files_4/testf_002.raw
diff color_3 ../tests/merger/test_files_4/testf_003.raw
