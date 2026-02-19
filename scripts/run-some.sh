BIN="/home/adrian/code/ring-pg/build"
$BIN/process < some.json
mv edges.tsv some-edges.tsv
mv nodes.tsv some-nodes.tsv
$BIN/tsvclean some
$BIN/tsv2ids some

mkdir data
mv some.data* data/
$BIN/build-index data/some.data pg

$BIN/query-filter data/some.data gql.tsv 1
$BIN/tsv2cypher some-clean some-clean-wd.cypher


