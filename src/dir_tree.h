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
#include <set>
#include <iterator>
#include <algorithm>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/filesystem.hpp>


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
            ar & name;
            ar & children;
        }
    public:
        bool operator<(const TreeNode& node) const { return name < node.name; }
        
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
        mutable std::set<TreeNode> children;

        TreeNode() { }
    };

    DirTree(): _root(nullptr) { }
    ~DirTree() { delete _root; }

    void initialize() { _root = new TreeNode; }
    TreeNode* root() const { return _root; }

    std::vector<std::string> hasConflict(const DirTree& tree) const {
        std::vector<std::string> conflicts;
        // TODO: rewrite this
        std::vector<TreeNode> confs; 

        auto treenode_comparator = [](const TreeNode& a, const TreeNode& b)->bool {
            return a.name == b.name;
        };

        set_intersection(_root->children.begin(), _root->children.end(),
                         tree.root()->children.begin(), tree.root()->children.end(),
                         std::back_inserter(confs), 
                         treenode_comparator);

        for (const auto i: confs)
            conflicts.push_back(i.name);
        return conflicts;
    }

    // merge dirtree from host_id to self's dirtree
    // assert no conflicts
    void merge(const DirTree& tree, const size_t host_id) {
        for (auto treenode: tree.root()->children) {
            treenode.host_id = host_id;
            _root->children.insert(treenode);
        }
    }

    const TreeNode* find(const std::string& path) const {
        using namespace boost::filesystem;
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

private:
    TreeNode* _root;

};

#endif /* DIR_TREE_H_ */
