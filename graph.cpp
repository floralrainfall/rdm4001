#include "graph.hpp"

namespace rdm {
  Graph::Graph() {
    root = std::unique_ptr<Node>(new Node());
    root->parent = NULL;
  }

  void Graph::Node::setParent(Graph::Node* node) {
    onParentChanging.fire(this, node);
    parent = node;
    node->addChild(this);
  }

  void Graph::Node::addChild(Graph::Node* node) {
    onChildAdding.fire(this, node);
    children.push_back(node);
    descendantAdding(this, node);
  }

  void Graph::Node::descendantAdding(Graph::Node* parent, Graph::Node* node) {
    if(parent)
      parent->descendantAdding(parent, node);
    onDescendantAdding.fire(this, parent, node);
  }

  glm::mat3 Graph::Node::worldTransform() {
    glm::mat3 base = glm::mat3(1);
    if(parent)
      base = parent->worldTransform();
    base *= transform;
    return base;
  }
}