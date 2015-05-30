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
 *  Description: 
 *****************************************************************************/
#ifndef DIR_TREE_H_
#define DIR_TREE_H_

#include <string>
#include <vector>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>


class DirTree {
private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int /* version */) {
        ar & _root;
    }

public:
    struct TreeNode {
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
            ar & name;
            ar & children;
        }

        // TODO: maybe more types?
        enum FileType { REGULAR, DIRECTORY };

        // TODO: maybe more attributes
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
        size_t host_id;
        
        // if name is empty, this is root node
        std::string name;
        std::vector<TreeNode> children;

        TreeNode() { }
    };

    DirTree(): _root(nullptr) { }
    ~DirTree() { delete _root; }

    void initialize() { _root = new TreeNode; }
    TreeNode* root() const { return _root; }

private:
    TreeNode* _root;

};



#endif /* DIR_TREE_H_ */
