#/bin/sh
rm -f unicensd xml2struct
mkdir -p out
cd out
cmake ..
make
mv unicensd ..
mv xml2struct ..

