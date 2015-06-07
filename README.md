#Group-Share Filesystem
The Group-Share Filesystem (GSFS) is a filesystem client that lets a host share files with some remote hosts within a group and also view the files remote hosts have shared over network. All these hosts need to have GSFS client, and require TCP/IP, SSH, SFTP installed since clients use TCP/IP to send metadata and SFTP to transfer file. FUSE is also required because all shared files are mounted as a userspace filesystem. Hosts in the group for now can only read the files shared by other hosts. The same host is allowed to join different groups simultaneously and the opetions won't disturb each other.

Thanks to Terran Lee for naming this project.

##How It Works
Each group has one master node and zero or more stand by nodes. The master node stores and maintains all metadata, including status of all shared files (file name, file size, access permission, on which node this file stored...) and connection configuration of each node. Each stand by node stores a copy of metadata, but only master node have permission to modify it. All nodes can see each other's status from the metadata.

A stand by node that wants to join a group first connects to the master node and sends its local metadata to the master, which contains status of files it wants to share and its connection configuration. Master node checks the metadata. If there's no conflics with pre-existing metadata, the new stand by node is accepted and the master node will send new metadata to all nodes. Now, all other nodes can see each other's status again. 

When a read operation arises, the reader directly connects to the store node via SFTP.

##Options

* -h [ --help ]

    Display help message.

* -v [ --version ]
    
    Display version number.

* -l [ --listen ] _ip address_

    Run as a master node and listen at this IP address. Both IPv4 and IPv6 are supported. This option is incompatible with -c.
    
* -c [ --connect ] _ip address_

    Run as a stand by node and connect to this IP address. Both IPv4 and IPv6 are supported. This option is incompatible with -l.
    
* -t [ --tcp-port ] _port_

    Specify TCP port. Master node listens at this port while stand by node connects to this port of master node. 
    
* -s [ --ssh-port ] _port_

    Specify SSH port. Default value is 22. Listen at this port so that other nodes can create a SFTP connection.

* -m [ --mount-point ] _directory_

    Specify filesysem mount point. Shared files from all nodes can be viewed in this directory.
    
* -w [ --working-directory ] _directory_
    
    Specify working directory, files in this directory will be shared with other hosts.

##Example 
###Dependency 

* FUSE for OS X (for Mac OS X) or FUSE (for Linux)
* libssh
* OpenSSH
* Boost C++ Libraries 1.58.0 or later
* all nodes belong to the same network
* SSH is properly configured so that all nodes can directly login to each other via public key authentication

###Master Node
Master node creates a group and wants to share files in directory `master`, mount point is `mount_point1`. It listens at `192.168.1.101:10000`.

```
./gsfs -l 192.168.1.101 -t 10000 -w master/ -m mount_point1/
```

###Stand By Node
Node 1 wants to share directory `standby1`, mount point is `mount_point2`.

```
./gsfs -c 192.168.1.101 -t 10000 -w standby1/ -m mount_point2/
```
Node 2 wants to share directory `standby2`, mount point is `mount_point3`.

```
./gsfs -c 192.168.1.101 -t 10000 -w standby2/ -m mount_point3/
```


### Preview
Now each node can view all shared files. E.g. on stand by node 1.

```
$ ls -l mount_point2
total 32
drwxr-xr-x  2 root  wheel   0 Jun  7 09:50 master_dir
-r--r--r--  1 root  wheel  22 Jun  7 09:50 master_file1
-r--r--r--  1 root  wheel  22 Jun  7 09:50 master_file2
-r--r--r--  1 root  wheel  34 Jun  7 09:51 standby1_file1
-r--r--r--  1 root  wheel  33 Jun  7 09:51 standby2_file1
$ cat mount_point2/master_dir/master_file3
This is master file3.
```

###Exit
Unmount the mount point, this node will quit from group. All other nodes can no longer see files from this node.



##References
1. [FUSE API documentation](http://fuse.sourceforge.net/doxygen/)
2. [libssh Documentation](http://api.libssh.org/master/index.html)
3. [Boost Library Documentation](http://www.boost.org/doc/)

##License
Copyright (c) 2015 Jamis Hoo. See the LICENSE file for license rights and limitations(MIT).