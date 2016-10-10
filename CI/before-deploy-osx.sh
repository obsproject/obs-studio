export GIT_HASH=$(git rev-parse --short HEAD)
export FILE_DATE=$(date +%Y-%m-%d.%H:%M:%S)
export FILENAME=$FILE_DATE-$GIT_HASH-osx.zip
mkdir nightly
cd ./build/rundir/RelWithDebInfo
zip -r -X $FILENAME .
mv ./$FILENAME ../../../nightly
