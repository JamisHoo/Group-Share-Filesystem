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
#include <sstream>
#include <iterator>
#include <algorithm>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/set.hpp>
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
            ar & num_links;
            ar & name;
            ar & children;
        }
    public:
        bool operator<(const TreeNode& node) const { return name < node.name; }
        
        // TODO: maybe more types?
        enum FileType { REGULAR, DIRECTORY, CHRDEVICE, BLKDEVICE, FIFO, SYMLINK, SOCKET, UNKNOWN };

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
        uint64_t host_id;
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
    void removeOf(const uint64_t host_id) {
        for (auto ite = _root->children.begin(); ite != _root->children.end();)
            if (ite->host_id == host_id) 
                _root->children.erase(ite++);
            else 
                ++ite;
    }

    std::vector<std::string> hasConflict(const DirTree& tree) const {
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
    void merge(const DirTree& tree, const size_t host_id) {
        for (auto treenode: tree.root()->children) {
            treenode.host_id = host_id;
            _root->children.insert(treenode);
        }
    }

    const TreeNode* find(const std::string& path) const {
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

    static std::string serialize(const DirTree& tree) {
        std::ostringstream ofs;
        boost::archive::text_oarchive oa(ofs);

        oa << tree;

        return ofs.str();
    }

    static DirTree deserialize(const std::string& byte_sequence) {
        std::istringstream ifs(byte_sequence);

        boost::archive::text_iarchive ia(ifs);

        DirTree tree;

        ia >> tree;

        return tree;
    }


private:
    TreeNode* _root;

};

#endif /* DIR_TREE_H_ */
