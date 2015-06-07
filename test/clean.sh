#!/bin/sh

pkill -SIGKILL gsfs

umount -f mount_point*
