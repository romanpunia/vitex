if command -v vcpkg &> /dev/null
then
	vcpkg install openal-soft
elif command -v apt-get &> /dev/null
then
	sudo apt-get install libopenal-dev
elif command -v brew &> /dev/null
then
	sudo brew install openal-soft && ln -s /usr/local/opt/openal-soft/include/AL /usr/local/include && ln -s /usr/local/opt/openal-soft/lib/*.dylib /usr/local/lib && ln -s /usr/local/opt/openal-soft/lib/*.a /usr/local/lib
else
	echo "install cannot be done automatically"
fi
sleep 1d