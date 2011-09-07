#!/bin/sh

POSARGS="-ww 400 -wh 400"

case "$1" in
	"svkbd")
		thingmenu $POSARGS -- \
			"svkbd de" "svkbd-de -wy -16" \
			"svkbd en" "svkbd-en -wy -16" \
			"svkbd arrows" "svkbd-arrows -wy -16" \
			"back" "thingmenu-menu.sh"
		;;
	"conn")
		thingmenu $POSARGS -- \
			"WWAN start" "sudo conn -s wwan" \
			"WWAN stop" "sudo conn -k wwan" \
			"WiFi start" "sudo conn -s wifi" \
			"WiFi stop" "sudo conn -k wifi" \
			"Ethernet start" "sudo conn -s eth" \
			"Ethernet stop" "sudo conn -k eth" \
			"back" "thingmenu-menu.sh"
		;;
	"fn")
		thingmenu $POSARGS -- \
			"backlight +10%" "xbacklight -inc 10%" \
			"backlight -10%" "xbacklight -dec 10%" \
			"battery" "sleep 1; xset dpms force off" \
			"suspend" "pm-suspend" \
			"hibernate" "pm-hibernate" \
			"rotate" "thinkpad-rotate.sh" \
			"monitor switch" "thinkpad-fn-f7.sh" \
			"back" "thingmenu-menu.sh"
		;;
	"sound")
		thingmenu $POSARGS -- \
			"volume +10%" "amixer set Master 10%+" \
			"volume -10%" "amixer set Master 10%-" \
			"toggle mute" "amixer set Master toggle" \
			"back" "thingmenu-menu.sh"
		;;
	*)
		thingmenu $POSARGS -- \
			"svkbd menu" "thingmenu-menu.sh svkbd" \
			"conn menu" "thingmenu-menu.sh conn" \
			"fn menu" "thingmenu-menu.sh fn" \
			"sound menu" "thingmenu-menu.sh sound" \
			"xkill" "xkill"
		;;
esac

