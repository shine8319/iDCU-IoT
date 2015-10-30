#!/bin/bash

echo "run30Minute.sh"

read_dom () {
    local IFS=\>
    read -d \< ENTITY CONTENT
    local ret=$?
    TAG_NAME=${ENTITY%% *}
    ATTRIBUTES=${ENTITY#* }
    return $ret
}
parse_dom () {
    if [[ $TAG_NAME = "ADDRESS" ]] ; then
        #eval local $ATTRIBUTES
	server=$CONTENT
        echo $server
    elif [[ $TAG_NAME = "CYCLE" ]] ; then
        #eval local $ATTRIBUTES
	cycle=$CONTENT
        echo $cycle
    fi
}
while read_dom; do
    parse_dom
done < /work/config/timeSync.xml

if [ "$cycle" = "run30Minute" ] ; then

	dir="/work/log/TimeSync/"
	mkdir $dir
	rdate $server &> /dev/null
	if [ "$?" != "0" ] ; then
		echo "$(date '+%F %X')  rdate [FAIL]" >> "$dir$(date '+%F').txt"
	fi

	sleep 1
	/work/smart/hwclock -w &

fi
