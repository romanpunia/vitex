if [ -z "$1" ]; then
	echo "pass triplet as first argument, please";
else
	echo "install packages"
	vcpkg install zlib assimp glew mongo-c-driver openal-soft openssl libpq sdl2 freetype --triplet=$1
fi
echo "install packages - done"