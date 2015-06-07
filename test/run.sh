#!/bin/sh

ip=192.168.1.101
tcp_port=10000
ssh_port=22

if [[ -z $1 ]]; then
    arg=l
    mount_point=mount_point1
    working_dir=master
elif [ $1 == 1 ]; then
    arg=l
    mount_point=mount_point1
    working_dir=master
elif [ $1 == 2 ]; then
    arg=c
    mount_point=mount_point2
    working_dir=standby1
else
    arg=c
    mount_point=mount_point3
    working_dir=standby2
fi

./gsfs -$arg $ip -t $tcp_port -s $ssh_port -w $working_dir -m $mount_point
