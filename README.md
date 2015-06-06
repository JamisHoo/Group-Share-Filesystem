#Group-Share Filesystem
The Group-Share Filesystem (GSFS) is a filesystem client that lets a host share files with some remote hosts within a group and also view the files they've shared over network. All these hosts need to have GSFS client, and require TCP/IP, SSH, SFTP installed since clients use TCP/IP to send metadata and SFTP to transfer file. FUSE is also required because all shared files are mounted as a userspace filesystem. Hosts in the group for now can only read the files shared by other hosts. The same host can simultaneously join different groups and the opetions won't disturb each other.

Thanks to Terran Lee for naming this project.

##How It Works
Each group has one master node and zero or more stand by nodes. The master node stores and maintains all metadata, including status of all shared files (file name, file size, access permission, on which node this file stored...) and connection configuration of each node. Each stand by node stores a copy of metadata, but only master node have permission to modify it. All nodes can see each other's status from the metadata.

A stand by node that wants to join a group first connects to the master node and sends its local metadata to the master, which contains status of files it wants to share and its connection configuration. Master node checks the metadata. If there's no conflics with pre-existing metadata, the new stand by node is accepted and the master node will send new metadata to all nodes. Now, all other nodes can see each other's status again. 

When a read operation arises, the reader directly connects to the store node via SFTP.

##Options

##Example

##References
1. [FUSE API documentation](http://fuse.sourceforge.net/doxygen/)
2. [libssh Documentation](http://api.libssh.org/master/index.html)
3. [Boost Library Documentation](http://www.boost.org/doc/)

##License
Copyright (c) 2015 Jamis Hoo. See the LICENSE file for license rights and limitations(MIT).