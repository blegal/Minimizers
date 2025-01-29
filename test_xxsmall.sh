./min_sorter_ref --file ../data/xxsmall_ecoli.fastx --output ref
./min_sorter     --file ../data/xxsmall_ecoli.fastx --output v1
./min_sorter_v2  --file ../data/xxsmall_ecoli.fastx --output v2
./raw_dump ref 0
./raw_dump v1  0
./raw_dump v2  0

diff ref v1
diff ref v2

./min_sorter_ref --file ../data/vsmall_ecoli.fastx --output ref
./min_sorter     --file ../data/vsmall_ecoli.fastx --output v1
./min_sorter_v2  --file ../data/vsmall_ecoli.fastx --output v2
./raw_dump ref 0
./raw_dump v1  0
./raw_dump v2  0

diff ref v1
diff ref v2

./min_sorter_ref --file ../data/wsmall_ecoli.fastx --output ref
./min_sorter     --file ../data/wsmall_ecoli.fastx --output v1
./min_sorter_v2  --file ../data/wsmall_ecoli.fastx --output v2
./raw_dump ref 0
./raw_dump v1  0
./raw_dump v2  0

diff ref v1
diff ref v2
