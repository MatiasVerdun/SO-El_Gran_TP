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
