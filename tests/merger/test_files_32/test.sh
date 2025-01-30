./BreiZHMinimizer -d ../tests/merger/test_files_32 --skip-minimizer-step

./raw_dump    -f data_n0.32c -c 32

./color_stats -f data_n0.32c -c 32

./extract_color -i data_n0.32c -c 32 -o color_0 -s 0
./extract_color -i data_n0.32c -c 32 -o color_1 -s 1
./extract_color -i data_n0.32c -c 32 -o color_2 -s 2
./extract_color -i data_n0.32c -c 32 -o color_3 -s 3
./extract_color -i data_n0.32c -c 32 -o color_4 -s 4
./extract_color -i data_n0.32c -c 32 -o color_5 -s 5
./extract_color -i data_n0.32c -c 32 -o color_6 -s 6
./extract_color -i data_n0.32c -c 32 -o color_7 -s 7
./extract_color -i data_n0.32c -c 32 -o color_8 -s 8
./extract_color -i data_n0.32c -c 32 -o color_9 -s 9
./extract_color -i data_n0.32c -c 32 -o color_10 -s 10
./extract_color -i data_n0.32c -c 32 -o color_11 -s 11
./extract_color -i data_n0.32c -c 32 -o color_12 -s 12
./extract_color -i data_n0.32c -c 32 -o color_13 -s 13
./extract_color -i data_n0.32c -c 32 -o color_14 -s 14
./extract_color -i data_n0.32c -c 32 -o color_15 -s 15
./extract_color -i data_n0.32c -c 32 -o color_16 -s 16
./extract_color -i data_n0.32c -c 32 -o color_17 -s 17
./extract_color -i data_n0.32c -c 32 -o color_18 -s 18
./extract_color -i data_n0.32c -c 32 -o color_19 -s 19
./extract_color -i data_n0.32c -c 32 -o color_20 -s 20
./extract_color -i data_n0.32c -c 32 -o color_21 -s 21
./extract_color -i data_n0.32c -c 32 -o color_22 -s 22
./extract_color -i data_n0.32c -c 32 -o color_23 -s 23
./extract_color -i data_n0.32c -c 32 -o color_24 -s 24
./extract_color -i data_n0.32c -c 32 -o color_25 -s 25
./extract_color -i data_n0.32c -c 32 -o color_26 -s 26
./extract_color -i data_n0.32c -c 32 -o color_27 -s 27
./extract_color -i data_n0.32c -c 32 -o color_28 -s 28
./extract_color -i data_n0.32c -c 32 -o color_29 -s 29
./extract_color -i data_n0.32c -c 32 -o color_30 -s 30
./extract_color -i data_n0.32c -c 32 -o color_31 -s 31

diff color_0 ../tests/merger/test_files_32/testf_000.raw
diff color_1 ../tests/merger/test_files_32/testf_001.raw
diff color_2 ../tests/merger/test_files_32/testf_002.raw
diff color_3 ../tests/merger/test_files_32/testf_003.raw
diff color_4 ../tests/merger/test_files_32/testf_004.raw
diff color_5 ../tests/merger/test_files_32/testf_005.raw
diff color_6 ../tests/merger/test_files_32/testf_006.raw
diff color_7 ../tests/merger/test_files_32/testf_007.raw
diff color_8 ../tests/merger/test_files_32/testf_008.raw
diff color_9 ../tests/merger/test_files_32/testf_009.raw
diff color_10 ../tests/merger/test_files_32/testf_010.raw
diff color_11 ../tests/merger/test_files_32/testf_011.raw
diff color_12 ../tests/merger/test_files_32/testf_012.raw
diff color_13 ../tests/merger/test_files_32/testf_013.raw
diff color_14 ../tests/merger/test_files_32/testf_014.raw
diff color_15 ../tests/merger/test_files_32/testf_015.raw
diff color_16 ../tests/merger/test_files_32/testf_016.raw
diff color_17 ../tests/merger/test_files_32/testf_017.raw
diff color_18 ../tests/merger/test_files_32/testf_018.raw
diff color_19 ../tests/merger/test_files_32/testf_019.raw
diff color_20 ../tests/merger/test_files_32/testf_020.raw
diff color_21 ../tests/merger/test_files_32/testf_021.raw
diff color_22 ../tests/merger/test_files_32/testf_022.raw
diff color_23 ../tests/merger/test_files_32/testf_023.raw
diff color_24 ../tests/merger/test_files_32/testf_024.raw
diff color_25 ../tests/merger/test_files_32/testf_025.raw
diff color_26 ../tests/merger/test_files_32/testf_026.raw
diff color_27 ../tests/merger/test_files_32/testf_027.raw
diff color_28 ../tests/merger/test_files_32/testf_028.raw
diff color_29 ../tests/merger/test_files_32/testf_029.raw
diff color_30 ../tests/merger/test_files_32/testf_030.raw
diff color_31 ../tests/merger/test_files_32/testf_031.raw
