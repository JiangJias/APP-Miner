#!/bin/sh
result=$(pwd)/$2
if [ -f "$result" ]; then
	rm $result
fi
scandir() {
    local cur_dir parent_dir workdir
    workdir=$1
    cd ${workdir}
    if [ ${workdir} = "/" ]
    then
        cur_dir=""
    else
        cur_dir=$(pwd)
    fi

    for dirlist in $(ls ${cur_dir})
    do
        if test -d ${dirlist};then
            cd ${dirlist}
            scandir ${cur_dir}/${dirlist}
            cd ..
        else
            if [ "${dirlist##*.}"x = "bc"x ];then
                echo ${cur_dir}/${dirlist}
                echo ${cur_dir}/${dirlist} >> $result
            elif [ "${dirlist##*.}"x = "list"x ];then
                rm ${cur_dir}/${dirlist}
            fi
        fi
    done
}

if test -d $1
then
    scandir $1
elif test -f $1
then
    echo "you input a file but not a directory,pls reinput and try again"
    exit 1
else
    echo "the Directory isn't exist which you input,pls input a new one!!"
    exit 1
fi
