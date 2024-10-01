#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "signal.hpp"

namespace rdm {
class Graph {
 public:
  Graph();

  struct Node {
    Node* parent;
    std::vector<Node*> children;

    glm::mat3 transform;

    glm::mat3 worldTransform();

    /**
     * @brief Sets the parent of the node.
     *
     * This will cause a bunch of signals to fire.
     *
     * @param node
     */
    void setParent(Node* node);

    /**
     * @brief Signal for when a child is added.
     *
     * First node param is this node, second node param is the added child.
     */
    Signal<Node*, Node*> onChildAdding;
    /**
     * @brief Signal for when a parent is set.
     *
     * First node param is this node, second node param is the new parent.
     */
    Signal<Node*, Node*> onParentChanging;
    /**
     * @brief Signal for when a node is added to the children of this node, or
     * any nodes that are descendants of this node.
     *
     * First node param is this node, second node param is the node that
     * received the new child, and the third param is the new child.
     */
    Signal<Node*, Node*, Node*> onDescendantAdding;

   private:
    void addChild(Node* node);

    void descendantAdding(Node* parent, Node* node);
  };

  Node* getRootNode() { return root.get(); }

 private:
  std::unique_ptr<Node> root;
};
}  // namespace rdm