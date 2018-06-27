#/bin/sh
rm -f unicensd
mkdir -p out
cd out
cmake ..
make
mv unicensd ..
mv xml2struct ..
mv unicensc ..

