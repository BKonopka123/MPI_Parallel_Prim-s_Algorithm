source /opt/nfs/config/source_upcxx_2023.9.sh
echo "Checking upcxx directory: "
which upcxx

touch nodes
/opt/nfs/config/station204_name_list.sh 1 16 > nodes
echo ""
echo "Checking avaliable nodes: "
cat nodes

make

echo ""
echo "Running Prim's algorithm for 4 nodes: "
make run ARG=4

echo ""
echo "Running Prim's algorithm for 8 nodes: "
make run ARG=8
