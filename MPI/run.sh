source /opt/nfs/config/source_mpich430.sh
source /opt/nfs/config/source_cuda121.sh
echo "Checking mpicc directory: "
which mpicc

touch nodes
/opt/nfs/config/station204_name_list.sh 1 16 > nodes
echo ""
echo "Checking avaliable stations: "
cat nodes

make

echo ""
echo "Running Prim's algorithm for 3 nodes: "
mpiexec -l -f nodes -n 3 ./main_program

echo ""
echo "Running Prim's algorithm for 8 nodes: "
mpiexec -l -f nodes -n 8 ./main_program

echo ""
echo "Running Prim's algorithm for 16 nodes: "
mpiexec -l -f nodes -n 16 ./main_program