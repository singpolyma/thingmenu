# Thingmenu - a simple X11 menu

This application evolved out of the need to be able to run commands
in a touchscreen environment.

## Installation

	% tar -xzvf thingmenu-*.tar.gz
	% cd thingmenu
	% make
	% sudo PREFIX=/usr make install

## Usage

	# This will open a 300px wide menu, which is showing an
	# entry "Reboot now". When being clicked this entry will run
	# "reboot". After that the menu will not exit (-s).
	% thingmenu -s -ww 300 -- "Reboot now:reboot"

	# This will create a centered menu, which is aligned based
	# on the length of the label texts. After the first clicked
	# entry it will exit.
	% thingmenu "Force reboot:reboot -f" "Shutdown:shutdown"

Have fun!

