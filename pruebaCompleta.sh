bash clean.sh
cd Config/Pruebas
cp CPUCompleta.txt ../CPU.txt
cp S-AFACompleta.txt ../S-AFA.txt
cp DAMCompleta.txt ../DAM.txt
cp MDJCompleta.txt ../MDJ.txt
cp FM9Completa.txt ../FM9.txt
cd ../../..
cd fifa-examples
git reset --hard
git clean -fd
cd ..
cd tp-2018-2c-smlc
