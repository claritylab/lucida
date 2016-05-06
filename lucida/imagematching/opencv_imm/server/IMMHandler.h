#pragma once

#include <vector>

#include "gen-cpp2/LucidaService.h"
#include "../opencv/detect.h"

namespace cpp2 {
class IMMHandler : virtual public LucidaServiceSvIf {
public:
	IMMHandler();

	folly::Future<folly::Unit> future_create
	(std::unique_ptr<std::string> LUCID, std::unique_ptr< ::cpp2::QuerySpec> spec);

	folly::Future<folly::Unit> future_learn
	(std::unique_ptr<std::string> LUCID, std::unique_ptr< ::cpp2::QuerySpec> knowledge);

	folly::Future<std::unique_ptr<std::string>> future_infer
	(std::unique_ptr<std::string> LUCID, std::unique_ptr< ::cpp2::QuerySpec> query);

private:
	struct timeval tp;
	DescriptorMatcher *matcher;
	std::vector<std::string> trainImgs;

	std::string infer(unique_ptr<string> LUCID, std::unique_ptr< ::cpp2::QuerySpec> query);
	void extractImageFromQuery(std::unique_ptr< ::cpp2::QuerySpec> query, std::string &image);

};
}
