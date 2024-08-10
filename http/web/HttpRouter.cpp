#include "http/web/HttpRouter.h"
#include "http/web/HttpContext.h"
#include "base/log/Logging.h"
namespace Miren
{
namespace http
{

namespace detail
{

  void parsePattern(const std::string& pattern, std::vector<std::string>& parts) {
    split(pattern, '/', parts);
  }

  void split(const std::string& pattern, char delim, std::vector<std::string>& parts) {
    size_t begin = 0;
    size_t end = 0;
    if(pattern[begin] == delim) {
      begin = 1;
      end = 1;
    }
    for(; end < pattern.size(); end++) {
      if(pattern[end] == delim) {
        parts.push_back(pattern.substr(begin, end - begin));
        begin = end + 1;
        end = end + 1;
      }
    }
    parts.push_back(pattern.substr(begin, pattern.size() - begin + 1));
  }

}// namespace detail


void HttpRouter::addRouter(const std::string& method, const std::string& pattern, HttpContextHandler handler)
{
  std::vector<std::string> parts;
  detail::parsePattern(pattern, parts);

  if(roots_.find(method) == roots_.end()) {
    roots_[method] = new Tire();
  }
  roots_[method]->insert(pattern, std::move(handler));
}

void HttpRouter::handle(std::shared_ptr<HttpContext> c)
{
  if(findRoute(c.get())) {
    LOG_INFO <<  "router 请求命中路由: " << c->Path();
    c->Next();
  }else {
      LOG_INFO <<  "router 请求未命中路由: " << c->Path();
      c->STRING(llhttp_status::HTTP_STATUS_NOT_FOUND, "NOT FOUND!");
  }
}

bool HttpRouter::findRoute(HttpContext* ctx)
{
  std::string mth = ctx->Method();
  if(roots_.find(mth) == roots_.end()) return false;
   
    std::vector<std::string> parts;
    detail::parsePattern(ctx->Path(), parts);
    
    Node* node = roots_[ctx->Method()]->search(parts);
    
    if(node != nullptr) {
        std::vector<std::string> node_parts;
        detail::parsePattern(node->pattern_, node_parts);
        for(int i = 0; i < node_parts.size(); ++i) {
            if(node_parts[i][0] == ':') {
                ctx->router_params_[node_parts[i].substr(1)] = parts[i];
            }
        }
        ctx->AddHandler(node->handler_);
        return true;
    }
    
    return false;

  
}

} // namespace http
} // namespace Miren
