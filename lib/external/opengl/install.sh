if command -v apt-get &> /dev/null
then
	sudo apt-get install libglu1-mesa-dev mesa-common-dev
else
	echo "OpenGL is probably already installed"
fi
sleep 1d