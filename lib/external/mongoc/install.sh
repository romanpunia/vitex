if command -v vcpkg &> /dev/null
then
	vcpkg install mongo-c-driver
elif command -v git &> /dev/null
then
	git clone https://github.com/mongodb/mongo-c-driver mongodb && cd mongodb && mkdir make && cd make && cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF .. && make && make install
else
	echo "install cannot be done automatically"
fi
sleep 1d