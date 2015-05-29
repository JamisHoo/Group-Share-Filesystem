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



class DirTree {
public:
    struct TreeNode {
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

    DirTree(): _root(new TreeNode) { }
    ~DirTree() { delete root; }

private:
    TreeNode* _root;

};


#endif /* DIR_TREE_H_ */
