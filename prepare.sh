set -e  # exit immediately on error
set -x  # display all commands

sudo apt-get install cmake

# ForestDB
if [ ! -f forestdb/build/libforestdb.a ]; then
	sudo apt-get install libsnappy-dev
	if [ ! -d forestdb ]; then
		git clone https://github.com/ForestDB-KVStore/forestdb.git ./forestdb
	fi
	if [ ! -d forestdb/build ]; then
		mkdir forestdb/build
	fi	
	cd forestdb/build
	cmake ../
	make clean
	make static_lib $1
	cd ../../
fi
