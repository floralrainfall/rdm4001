#include "graph.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace rdm {
Graph::Graph() {
  root = std::unique_ptr<Node>(new Node());
  root->parent = NULL;
}

Graph::Node::Node() {
  basis = glm::mat3(1);
  origin = glm::vec3(0);
  scale = glm::vec3(1);
  parent = NULL;
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
  if (parent) parent->descendantAdding(parent, node);
  onDescendantAdding.fire(this, parent, node);
}

glm::mat4 Graph::Node::worldTransform() {
  glm::mat4 base = glm::mat4(1);
  if (parent) base = parent->worldTransform();
  base *= glm::translate(origin);
  base *= glm::mat4(glm::inverse(basis));
  base *= glm::scale(scale);
  return base;
}
}  // namespace rdm
