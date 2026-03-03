BIN="/home/adrian/code/ring-pg/build"

unzip -p json.zip | $BIN/process
mv edges.tsv wiki-edges.tsv
mv nodes.tsv wiki-nodes.tsv
$BIN/tsvclean wiki
$BIN/tsv2ids wiki

#Prepare for CDS
mkdir data
mv wiki.data* data/
#Build CDS
$BIN/build-index data/wiki.data pg

#Prepare queries
$BIN/query-filter data/wiki.data gql.tsv 1

#Prepare for Neo4j
$BIN/tsv2neo4jtsv wiki-clean --compact

