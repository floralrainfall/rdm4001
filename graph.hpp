#pragma once
#include <memory>
#include <vector>
#include "signal.hpp"
#include <glm/glm.hpp>

namespace rdm {
  class Graph {
  public:
    Graph();

    struct Node {
      Node* parent;
      std::vector<Node*> children;

      glm::mat3 transform;

      glm::mat3 worldTransform();
      
      void setParent(Node* node);

      // this, child
      Signal<Node*, Node*> onChildAdding;
      // this, parent
      Signal<Node*, Node*> onParentChanging;
      // this, parent, descendant
      Signal<Node*, Node*, Node*> onDescendantAdding;
    private:
      void addChild(Node* node);

      void descendantAdding(Node* parent, Node* node);
    };

    Node* getRootNode() { return root.get(); }
  private:
    std::unique_ptr<Node> root;
  };
}