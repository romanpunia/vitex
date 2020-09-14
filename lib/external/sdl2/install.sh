if command -v vcpkg &> /dev/null
then
	vcpkg install sdl2
elif command -v apt-get &> /dev/null
then
	sudo apt-get install libsdl2-dev
elif command -v brew &> /dev/null
then
	sudo brew install sdl2 && brew link sdl2 --force
else
	echo "install cannot be done automatically"
fi
sleep 1d