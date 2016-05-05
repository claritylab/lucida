#pragma once

#include "gen-cpp2/LucidaService.h"

namespace cpp2 {
class FakeImmHandler : virtual public LucidaServiceSvIf {
public:
	FakeImmHandler();

	folly::Future<folly::Unit> future_create
	(std::unique_ptr<std::string> LUCID, std::unique_ptr< ::cpp2::QuerySpec> spec);

	folly::Future<folly::Unit> future_learn
	(std::unique_ptr<std::string> LUCID, std::unique_ptr< ::cpp2::QuerySpec> knowledge);

	folly::Future<std::unique_ptr<std::string>> future_infer
	(std::unique_ptr<std::string> LUCID, std::unique_ptr< ::cpp2::QuerySpec> query);

private:
	std::string askQA();
};
}
