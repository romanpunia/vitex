if command -v vcpkg &> /dev/null
then
	vcpkg install glew
elif command -v apt-get &> /dev/null
then
	sudo apt-get install libglew-dev
elif command -v brew &> /dev/null
then
	sudo brew install glew && brew link glew --force
else
	echo "install cannot be done automatically"
fi
sleep 1d