source /opt/nfs/config/source_mpich430.sh
source /opt/nfs/config/source_cuda121.sh
echo "Checking mpicc directory: "
which mpicc

touch nodes
/opt/nfs/config/station204_name_list.sh 1 16 > nodes
echo ""
echo "Checking avaliable nodes: "
cat nodes

make

echo ""
echo "Running Prim's algorithm for 3 nodes: "
make post_build ARG=3

echo ""
echo "Running Prim's algorithm for 8 nodes: "
make post_build ARG=8

echo ""
echo "Running Prim's algorithm for 16 nodes: "
make post_build ARG=16