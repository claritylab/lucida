#pragma once

//#include "gen-cpp2/DIG.h"
#include "../gen-cpp2/LucidaService.h"

#include "caffe/caffe.hpp"

/*
namespace facebook {
namespace windtunnel {
namespace treadmill {
namespace services {
namespace dig {
*/

namespace cpp2 {

//class DIGHandler : virtual public DIGSvIf {
class DIGHandler : virtual public LucidaServiceSvIf {
 public:
  DIGHandler();

  folly::Future<folly::Unit> future_create
  (std::unique_ptr<std::string> LUCID,
      std::unique_ptr< ::cpp2::QuerySpec> spec);

  folly::Future<folly::Unit> future_learn
  (std::unique_ptr<std::string> LUCID,
      std::unique_ptr< ::cpp2::QuerySpec> knowledge);

  folly::Future<std::unique_ptr<std::string>> future_infer
  (std::unique_ptr<std::string> LUCID,
      std::unique_ptr< ::cpp2::QuerySpec> query);


/*
  folly::Future<std::unique_ptr<std::string> >
  future_digitRecognition(std::unique_ptr<std::string> image);
*/
 private:
  void reshape(caffe::Net<float> *net, int input_size);

  std::string network_;
  std::string weights_;
  caffe::Net<float>* net_;
};

} // namespace cpp2

/*
} // namespace dig
} // namespace services
} // namespace treadmill
} // namespace windtunnel
} // namespace facebook
*/
