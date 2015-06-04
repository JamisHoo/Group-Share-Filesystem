/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: Group-Share Filesystem
 *  Filename: dir_tree.h 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: May 29, 2015
 *  Time: 23:01:21
 *  Description: TreeNode describes a node in file-tree.
 *               DirTree is a tree of TreeNodes
 *****************************************************************************/
#ifndef DIR_TREE_H_
#define DIR_TREE_H_

#include <set>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/set.hpp>


class DirTree {
private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int /* version */) {
        ar & _root;
    }

public:
    class TreeNode {
    private:
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive& ar, const unsigned int /* version */) {
            ar & type;
            ar & size;
            ar & uid;
            ar & gid;
            ar & atime;
            ar & mtime;
            ar & ctime;
            ar & host_id;
            ar & num_links;
            ar & name;
            ar & children;
        }

    public:
        void setHostID(const uint64_t host_id) const {
            for (const auto& node: children) {
                node.host_id = host_id;
                node.setHostID(host_id);
            }
        }

        bool operator<(const TreeNode& node) const { return name < node.name; }
        
        enum FileType { REGULAR, DIRECTORY, CHRDEVICE, BLKDEVICE, FIFO, SYMLINK, SOCKET, UNKNOWN };

        // file type
        FileType type;
        // file size
        size_t size;
        // owner user id
        size_t uid;
        // owner group id 
        size_t gid;
        // time of last access
        size_t atime;
        // time of last modification
        size_t mtime;
        // time of last status change
        size_t ctime;
        // this tree node belongs to which host
        mutable uint64_t host_id;
        // number of hard links
        size_t num_links;
        
        // if name is empty, this is root node
        std::string name;
        mutable std::set<TreeNode> children;

        TreeNode() { }
    };

    DirTree(): _root(nullptr) { }
    ~DirTree() { delete _root; }

    void initialize() { _root = new TreeNode; }
    TreeNode* root() const { return _root; }

    // remove all nodes of a certain host
    void removeOf(const uint64_t host_id);

    // remove all nodes but those of a certain node
    void removeNotOf(const uint64_t host_id);

    std::vector<std::string> hasConflict(const DirTree& tree) const;

    // merge dirtree from host_id to self's dirtree
    // assert no conflicts
    void merge(const DirTree& tree);

    const TreeNode* find(const std::string& path) const;

    static std::string serialize(const DirTree& tree);

    static DirTree deserialize(const std::string& byte_sequence);

private:
    TreeNode* _root;

};

#endif /* DIR_TREE_H_ */
