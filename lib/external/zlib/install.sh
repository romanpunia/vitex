if command -v vcpkg &> /dev/null
then
	vcpkg install zlib
elif command -v apt-get &> /dev/null
then
	sudo apt-get install zlib1g-dev
elif command -v brew &> /dev/null
then
	sudo brew install zlib && ln -s /usr/local/opt/zlib/include/* /usr/local/include && ln -s /usr/local/opt/zlib/lib/*.dylib /usr/local/lib && ln -s /usr/local/opt/zlib/lib/*.a /usr/local/lib
else
	echo "install cannot be done automatically"
fi
sleep 1d