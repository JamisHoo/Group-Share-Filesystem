/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: Group-Share Filesystem
 *  Filename: dir_tree.cc 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: Jun  4, 2015
 *  Time: 22:41:32
 *  Description: TreeNode describes a node in file-tree.
 *               DirTree is a tree of TreeNodes
 *****************************************************************************/

#include "dir_tree.h"
#include <string>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <boost/filesystem.hpp>


// remove all nodes of a certain host
void DirTree::removeOf(const uint64_t host_id) {
    for (auto ite = _root->children.begin(); ite != _root->children.end();)
        if (ite->host_id == host_id) 
            _root->children.erase(ite++);
        else 
            ++ite;
}

// remove all nodes but those of a certain node
void DirTree::removeNotOf(const uint64_t host_id) {
    for (auto ite = _root->children.begin(); ite != _root->children.end();)
        if (ite->host_id != host_id)
            _root->children.erase(ite++);
        else 
            ++ite;
}

std::vector<std::string> DirTree::hasConflict(const DirTree& tree) const {
    std::vector<std::string> conflicts;

    auto first1 = tree.root()->children.begin();
    auto last1 = tree.root()->children.begin();
    auto first2 = _root->children.begin();
    auto last2 = _root->children.end();

    while (first1 != last1 && first2 != last2) {
        if (*first1 < *first2) ++first1;
        else if (*first2 < *first1) ++first2;
        else {
            conflicts.push_back(first1->name);
            ++first1, ++first2;
        }
    }

    return conflicts;
}

// merge dirtree from host_id to self's dirtree
// assert no conflicts
void DirTree::merge(const DirTree& tree) {
    for (auto treenode: tree.root()->children) 
        _root->children.insert(treenode);
}

const DirTree::TreeNode* DirTree::find(const std::string& path) const {
    boost::filesystem::path p(path);
    const TreeNode* node = nullptr;
    TreeNode key;
    for (auto ite = p.begin(); ite != p.end(); ++ite) {
        assert(*ite != "..");
        if (*ite == ".") continue;
        else if (*ite == "/" && !node) node = _root;
        else if (!node) continue;
        else {
            key.name = ite->string();
            auto set_ite = node->children.find(key);
            if (set_ite == node->children.end()) return nullptr;
            node = &(*set_ite);
        }
    }

    return node;
}

std::string DirTree::serialize(const DirTree& tree) {
    std::ostringstream ofs;
    boost::archive::text_oarchive oa(ofs);

    oa << tree;

    return ofs.str();
}

DirTree DirTree::deserialize(const std::string& byte_sequence) {
    std::istringstream ifs(byte_sequence);

    boost::archive::text_iarchive ia(ifs);

    DirTree tree;

    ia >> tree;

    return tree;
}


