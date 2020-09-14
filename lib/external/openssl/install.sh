if command -v vcpkg &> /dev/null
then
	vcpkg install openssl
elif command -v apt-get &> /dev/null
then
	sudo apt-get install libssl-dev
elif command -v brew &> /dev/null
then
	sudo brew install openssl@1.1 && ln -s /usr/local/opt/openssl@1.1/include/openssl /usr/local/include && ln -s /usr/local/opt/openssl@1.1/lib/*.dylib /usr/local/lib && ln -s /usr/local/opt/openssl@1.1/lib/*.a /usr/local/lib
else
	echo "install cannot be done automatically"
fi
sleep 1d