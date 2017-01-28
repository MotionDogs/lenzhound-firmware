ARDUINO=~/.lenzhound/Arduino.app/Contents

UNAME_STR=`uname`
if [ "$UNAME_STR" == 'Darwin' ]; then
	ARDUINO=~/.lenzhound/Arduino.app/Contents
elif [ "$UNAME_STR" == 'Linux' ];
	ARDUINO=~/.lenzhound/arduino/Contents
else
	echo "Unkown platform $UNAME_STR" 1>&2
	exit 1
fi

BUILDER=$ARDUINO/Java/arduino-builder
HARDWARE_DIR=$ARDUINO/Java/hardware
BOARD_NAME=arduino:avr:leonardo
TOOLS_1=$ARDUINO/Java/tools-builder
TOOLS_2=$HARDWARE_DIR/tools
BUILD_OUTPUT=~/.lenzhound/build

txr () {
	eval "$BUILDER -hardware $HARDWARE_DIR -fqbn $BOARD_NAME -tools $TOOLS_1 -tools $TOOLS_2 -libraries ./libraries -build-path $BUILD_OUTPUT -compile Txr/Txr.ino"
}

rxr () {
	eval "$BUILDER -hardware $HARDWARE_DIR -fqbn $BOARD_NAME -tools $TOOLS_1 -tools $TOOLS_2 -libraries ./libraries -build-path $BUILD_OUTPUT -compile Rxr/Rxr.ino"
}

set -- `getopt "tr" "$@"`

while [ ! -z "$1" ]
do
  case "$1" in
    -t) txr ;;
    -r) rxr ;;
  esac

  shift
done