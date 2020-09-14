if command -v vcpkg &> /dev/null
then
	vcpkg install assimp
elif command -v git &> /dev/null
then
	git clone https://github.com/assimp/assimp && cd assimp && mkdir make && cd make && cmake .. && make && make install
else
	echo "install cannot be done automatically"
fi
sleep 1d