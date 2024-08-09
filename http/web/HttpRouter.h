#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace Miren
{
namespace http
{
namespace detail
{
  void parsePattern(const std::string& pattern, std::vector<std::string>& parts);
  void split(const std::string& pattern, char delim, std::vector<std::string>& parts);
}

  class HttpContext;
  typedef std::function<void(std::shared_ptr<HttpContext>)> HttpContextHandler;

  struct Node
  {
    std::string pattern_;
    std::string part_;
    std::vector<Node*> children_;
    bool isWild_;
    HttpContextHandler handler_;
  
    Node(const std::string& part = "/", bool isWild = false) : part_(part), isWild_(isWild) {}

    virtual ~Node() {
      for(Node* child : children_) {
        delete child;
      }
    }

    void insertChild(Node* child) { children_.push_back(child); }

    Node* findChild(const std::string& part) {
      for(Node* child : children_) {
        if(child->part_ == part || child->isWild_) {
          return child;
        }
      }
      return nullptr;
    }

    std::vector<Node*> findChildren(const std::string& part) {
      std::vector<Node*> nodes;
      for(Node* child : children_) {
        if(child->part_ == part || child->isWild_) {
          nodes.push_back(child);
        }
      }
      return nodes;
    }
  };

  class Tire
  {
  private:
    Node* root_;
  
  public:
    Tire() : root_(new Node()) {}
    ~Tire() { delete root_; root_ = nullptr; }

    void insert(const std::string& pattern, HttpContextHandler handler)
    {
      std::vector<std::string> parts;
      detail::parsePattern(pattern, parts);
      insert(parts, root_, 0, pattern, std::move(handler));
    }

    Node* search(const std::string& pattern) 
    {
      std::vector<std::string> parts;
      detail::parsePattern(pattern, parts);
      return search(parts, root_, 0);
    }

    Node* search(std::vector<std::string>& parts) 
    {
      return search(parts, root_, 0);
    }
  private:
    void insert(std::vector<std::string>& parts, Node* node, size_t high, 
                const std::string& pattern, HttpContextHandler handler)
    {
      if(parts.size() == high) {
        node->pattern_ = pattern;
        node->handler_ = std::move(handler);  //后续handler为nullptr
        return;
      }
      std::string part = parts[high];
      Node* child = node->findChild(part);
      if(child == nullptr) {
        child = new Node(part, part[0] == ':');
        node->insertChild(child);
      }

      insert(parts, child, high + 1, pattern, handler);
    }

    Node* search(std::vector<std::string>& parts, Node* node, size_t high)
    {
      if(parts.size() == high) {
        if(node->pattern_ == "") {
          return nullptr;
        }
        return node;
      }

      std::string part = parts[high];
      std::vector<Node*> children = node->findChildren(part);
      for(Node* child : children) {
        Node* res = search(parts, child, high + 1);
        if(res != nullptr) {
          return res;
        }
      }
      return nullptr;
    }
  };

  class HttpRouter
  {
    std::unordered_map<std::string, Tire*> roots_;
    bool findRoute(HttpContext* ctx);

  public:
    virtual ~HttpRouter()
    {
      for(auto it = roots_.begin(); it != roots_.end(); it++) {
        delete it->second;
        it->second = nullptr;
      }
    }

    void addRouter(const std::string& method, const std::string& pattern, HttpContextHandler handler);
    void handle(std::shared_ptr<HttpContext> c);
  };


} // namespace http

} // namespace Miren
