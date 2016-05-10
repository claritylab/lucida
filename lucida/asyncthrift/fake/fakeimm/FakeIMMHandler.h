#pragma once

#include <mutex>
#include "gen-cpp2/LucidaService.h"

namespace cpp2 {
class FakeImmHandler : virtual public LucidaServiceSvIf {
private:
	std::string askQA();
	static std::mutex cout_lock;

public:
	static void print(const char *s) {
		cout_lock.lock();
		std::cout << s << std::endl;
		cout_lock.unlock();
	}
	static void print(const std::string &s) {
		cout_lock.lock();
		std::cout << s << std::endl;
		cout_lock.unlock();
	}

	FakeImmHandler();

	folly::Future<folly::Unit> future_create
	(std::unique_ptr<std::string> LUCID, std::unique_ptr< ::cpp2::QuerySpec> spec);

	folly::Future<folly::Unit> future_learn
	(std::unique_ptr<std::string> LUCID, std::unique_ptr< ::cpp2::QuerySpec> knowledge);

	folly::Future<std::unique_ptr<std::string>> future_infer
	(std::unique_ptr<std::string> LUCID, std::unique_ptr< ::cpp2::QuerySpec> query);
};
}
