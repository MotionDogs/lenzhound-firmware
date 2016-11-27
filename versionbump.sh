# rm -r /tmp/build*

# $lower = $(echo $1 | tr '[:upper:' '[:lower:')
# eval 'make verify-$lower
# cp /tmp/build*/$1.ino.hex ./bin/$1.ino.leonardo-$2.hex


txr () {
    contents=$(cat ./Txr/version.h)
    regex="VERSION \"2\.([0-9]+)\""

    if [[ $contents =~ $regex ]]
    then
        version="${BASH_REMATCH[1]}"
        echo "#define VERSION \"2.$((version + 1))\"" > ./Txr/version.h
    else
        echo "$contents doesn't match" >&2 # this could get noisy if there are a lot of non-matching files
    fi

    make verify-txr

    cp /tmp/build*/Txr.ino.hex ./bin/Txr.ino.leonardo-2.$((version + 1)).hex
}

rxr () {
    contents=$(cat ./Rxr/version.h)
    regex="VERSION \"2\.([0-9]+)\""

    if [[ $contents =~ $regex ]]
    then
        version="${BASH_REMATCH[1]}"
        echo "#define VERSION \"2.$((version + 1))\"" > ./Rxr/version.h
    else
        echo "$contents doesn't match" >&2 # this could get noisy if there are a lot of non-matching files
    fi

    make verify-rxr
    
    cp /tmp/build*/Rxr.ino.hex ./bin/Rxr.ino.leonardo-2.$((version + 1)).hex
}


E_OPTERR=65

rm -r /tmp/build*

set -- `getopt "tr" "$@"`

while [ ! -z "$1" ]
do
  case "$1" in
    -t) txr ;;
    -r) rxr ;;
  esac

  shift
done
