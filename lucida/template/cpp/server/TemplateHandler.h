#ifndef __TEMPLATEHANDLER_H__
#define __TEMPLATEHANDLER_H__

#include <vector>
#include <mutex>

#include "gen-cpp2/LucidaService.h"

// Define print for simple logging.
extern std::mutex cout_lock_cpp;
#define print( x ) \
	( \
			(cout_lock_cpp.lock()), \
			(std::cout << x << endl), \
			(cout_lock_cpp.unlock()), \
			(void)0 \
	)

namespace cpp2 {
class TemplateHandler : virtual public LucidaServiceSvIf {
public:
	TemplateHandler();

	folly::Future<folly::Unit> future_create
	(std::unique_ptr<std::string> LUCID,
			std::unique_ptr< ::cpp2::QuerySpec> spec);

	folly::Future<folly::Unit> future_learn
	(std::unique_ptr<std::string> LUCID,
			std::unique_ptr< ::cpp2::QuerySpec> knowledge);

	folly::Future<std::unique_ptr<std::string>> future_infer
	(std::unique_ptr<std::string> LUCID,
			std::unique_ptr< ::cpp2::QuerySpec> query);

private:
	// TODO: Define your own private methods
};
}

#endif
