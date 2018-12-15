cd ..
git clone https://github.com/sisoputnfrba/so-commons-library
cd so-commons-library
sudo make install
cd ..
git clone https://github.com/sisoputnfrba/fifa-examples
cd tp-2018-2c-smlc
cd sharedlib/Debug
make clean
make all
sudo mv libsharedlib.so /usr/lib
cd ../..
cd S-AFA/Debug
make clean
make all
cd ../..
cd MDJ/Debug
make clean
make all
cd ../..
cd FM9/Debug
make clean
make all
cd ../..
cd DAM/Debug
make clean
make all
cd ../..
cd CPU/Debug
make clean
make all
cd ../..
cd Config/Pruebas
echo "S-AFA_IP=$4" > CPUMinima.txt
echo "S-AFA_PUERTO=8000" >> CPUMinima.txt
echo "DAM_IP=$1" >> CPUMinima.txt
echo "DAM_PUERTO=8002" >> CPUMinima.txt
echo "FM9_IP=$2" >> CPUMinima.txt
echo "FM9_PUERTO=8004" >> CPUMinima.txt
echo "RETARDO=100" >> CPUMinima.txt
echo "IP_ESCUCHA=$4" > S-AFAMinima.txt
echo "CPU_PUERTO=8000" >> S-AFAMinima.txt
echo "DAM_PUERTO=8001" >> S-AFAMinima.txt
echo "ALGO_PLANI=RR" >> S-AFAMinima.txt
echo "Q=2" >> S-AFAMinima.txt
echo "GMP=3" >> S-AFAMinima.txt
echo "RETARDO=10" >> S-AFAMinima.txt
echo "IP_ESCUCHA=$1" > DAMMinima.txt
echo "CPU_PUERTO=8002" >> DAMMinima.txt
echo "S-AFA_IP=$4" >> DAMMinima.txt
echo "S-AFA_PUERTO=8001" >> DAMMinima.txt
echo "FM9_IP=$2" >> DAMMinima.txt
echo "FM9_PUERTO=8003" >> DAMMinima.txt
echo "MDJ_IP=$3" >> DAMMinima.txt
echo "MDJ_PUERTO=8004" >> DAMMinima.txt
echo "TSIZE=32" >> DAMMinima.txt
echo "IP_ESCUCHA=$3" > MDJMinima.txt
echo "DAM_PUERTO=8004" >> MDJMinima.txt
echo "PUNTO_MONTAJE=/home/utnso/fifa-examples/fifa-entrega/" >> MDJMinima.txt
echo "RETARDO=1000" >> MDJMinima.txt
echo "IP_ESCUCHA=$2" > FM9Minima.txt
echo "DAM_PUERTO=8003" >> FM9Minima.txt
echo "CPU_PUERTO=8004" >> FM9Minima.txt
echo "MODO_EJ=SEG" >> FM9Minima.txt
echo "TMM=8192" >> FM9Minima.txt
echo "TML=64" >> FM9Minima.txt
echo "TMP=128" >> FM9Minima.txt
echo "S-AFA_IP=$4" > CPUAlgoritmos.txt
echo "S-AFA_PUERTO=8000" >> CPUAlgoritmos.txt
echo "DAM_IP=$1" >> CPUAlgoritmos.txt
echo "DAM_PUERTO=8002" >> CPUAlgoritmos.txt
echo "FM9_IP=$2" >> CPUAlgoritmos.txt
echo "FM9_PUERTO=8004" >> CPUAlgoritmos.txt
echo "RETARDO=200" >> CPUAlgoritmos.txt
echo "IP_ESCUCHA=$4" > S-AFAAlgoritmos.txt
echo "CPU_PUERTO=8000" >> S-AFAAlgoritmos.txt
echo "DAM_PUERTO=8001" >> S-AFAAlgoritmos.txt
echo "ALGO_PLANI=VRR" >> S-AFAAlgoritmos.txt
echo "Q=2" >> S-AFAAlgoritmos.txt
echo "GMP=2" >> S-AFAAlgoritmos.txt
echo "RETARDO=500" >> S-AFAAlgoritmos.txt
echo "IP_ESCUCHA=$1" > DAMAlgoritmos.txt
echo "CPU_PUERTO=8002" >> DAMAlgoritmos.txt
echo "S-AFA_IP=$4" >> DAMAlgoritmos.txt
echo "S-AFA_PUERTO=8001" >> DAMAlgoritmos.txt
echo "FM9_IP=$2" >> DAMAlgoritmos.txt
echo "FM9_PUERTO=8003" >> DAMAlgoritmos.txt
echo "MDJ_IP=$3" >> DAMAlgoritmos.txt
echo "MDJ_PUERTO=8004" >> DAMAlgoritmos.txt
echo "TSIZE=16" >> DAMAlgoritmos.txt
echo "IP_ESCUCHA=$3" > MDJAlgoritmos.txt
echo "DAM_PUERTO=8004" >> MDJAlgoritmos.txt
echo "PUNTO_MONTAJE=/home/utnso/fifa-examples/fifa-entrega/" >> MDJAlgoritmos.txt
echo "RETARDO=500" >> MDJAlgoritmos.txt
echo "IP_ESCUCHA=$2" > FM9Algoritmos.txt
echo "DAM_PUERTO=8003" >> FM9Algoritmos.txt
echo "CPU_PUERTO=8004" >> FM9Algoritmos.txt
echo "MODO_EJ=TPI" >> FM9Algoritmos.txt
echo "TMM=8192" >> FM9Algoritmos.txt
echo "TML=64" >> FM9Algoritmos.txt
echo "TMP=128" >> FM9Algoritmos.txt
echo "IP_ESCUCHA=$4" > S-AFAAlgoritmos2.txt
echo "CPU_PUERTO=8000" >> S-AFAAlgoritmos2.txt
echo "DAM_PUERTO=8001" >> S-AFAAlgoritmos2.txt
echo "ALGO_PLANI=VRR" >> S-AFAAlgoritmos2.txt
echo "Q=4" >> S-AFAAlgoritmos2.txt
echo "GMP=5" >> S-AFAAlgoritmos2.txt
echo "RETARDO=500" >> S-AFAAlgoritmos2.txt
echo "S-AFA_IP=$4" > CPUFile.txt
echo "S-AFA_PUERTO=8000" >> CPUFile.txt
echo "DAM_IP=$1" >> CPUFile.txt
echo "DAM_PUERTO=8002" >> CPUFile.txt
echo "FM9_IP=$2" >> CPUFile.txt
echo "FM9_PUERTO=8004" >> CPUFile.txt
echo "RETARDO=10" >> CPUFile.txt
echo "IP_ESCUCHA=$4" > S-AFAFile.txt
echo "CPU_PUERTO=8000" >> S-AFAFile.txt
echo "DAM_PUERTO=8001" >> S-AFAFile.txt
echo "ALGO_PLANI=VRR" >> S-AFAFile.txt
echo "Q=4" >> S-AFAFile.txt
echo "GMP=4" >> S-AFAFile.txt
echo "RETARDO=10" >> S-AFAFile.txt
echo "IP_ESCUCHA=$1" > DAMFile.txt
echo "CPU_PUERTO=8002" >> DAMFile.txt
echo "S-AFA_IP=$4" >> DAMFile.txt
echo "S-AFA_PUERTO=8001" >> DAMFile.txt
echo "FM9_IP=$2" >> DAMFile.txt
echo "FM9_PUERTO=8003" >> DAMFile.txt
echo "MDJ_IP=$3" >> DAMFile.txt
echo "MDJ_PUERTO=8004" >> DAMFile.txt
echo "TSIZE=32" >> DAMFile.txt
echo "IP_ESCUCHA=$3" > MDJFile.txt
echo "DAM_PUERTO=8004" >> MDJFile.txt
echo "PUNTO_MONTAJE=/home/utnso/fifa-examples/fifa-entrega/" >> MDJFile.txt
echo "RETARDO=100" >> MDJFile.txt
echo "IP_ESCUCHA=$2" > FM9File.txt
echo "DAM_PUERTO=8003" >> FM9File.txt
echo "CPU_PUERTO=8004" >> FM9File.txt
echo "MODO_EJ=SPA" >> FM9File.txt
echo "TMM=8192" >> FM9File.txt
echo "TML=64" >> FM9File.txt
echo "TMP=128" >> FM9File.txt
echo "S-AFA_IP=$4" > CPUCompleta.txt
echo "S-AFA_PUERTO=8000" >> CPUCompleta.txt
echo "DAM_IP=$1" >> CPUCompleta.txt
echo "DAM_PUERTO=8002" >> CPUCompleta.txt
echo "FM9_IP=$2" >> CPUCompleta.txt
echo "FM9_PUERTO=8004" >> CPUCompleta.txt
echo "RETARDO=1000" >> CPUCompleta.txt
echo "IP_ESCUCHA=$4" > S-AFACompleta.txt
echo "CPU_PUERTO=8000" >> S-AFACompleta.txt
echo "DAM_PUERTO=8001" >> S-AFACompleta.txt
echo "ALGO_PLANI=VRR" >> S-AFACompleta.txt
echo "Q=2" >> S-AFACompleta.txt
echo "GMP=5" >> S-AFACompleta.txt
echo "RETARDO=10" >> S-AFACompleta.txt
echo "IP_ESCUCHA=$1" > DAMCompleta.txt
echo "CPU_PUERTO=8002" >> DAMCompleta.txt
echo "S-AFA_IP=$4" >> DAMCompleta.txt
echo "S-AFA_PUERTO=8001" >> DAMCompleta.txt
echo "FM9_IP=$2" >> DAMCompleta.txt
echo "FM9_PUERTO=8003" >> DAMCompleta.txt
echo "MDJ_IP=$3" >> DAMCompleta.txt
echo "MDJ_PUERTO=8004" >> DAMCompleta.txt
echo "TSIZE=32" >> DAMCompleta.txt
echo "IP_ESCUCHA=$3" > MDJCompleta.txt
echo "DAM_PUERTO=8004" >> MDJCompleta.txt
echo "PUNTO_MONTAJE=/home/utnso/fifa-examples/fifa-entrega/" >> MDJCompleta.txt
echo "RETARDO=500" >> MDJCompleta.txt
echo "IP_ESCUCHA=$2" > FM9Completa.txt
echo "DAM_PUERTO=8003" >> FM9Completa.txt
echo "CPU_PUERTO=8004" >> FM9Completa.txt
echo "MODO_EJ=SEG" >> FM9Completa.txt
echo "TMM=4096" >> FM9Completa.txt
echo "TML=64" >> FM9Completa.txt
echo "TMP=128" >> FM9Completa.txt
cd ../..
