#pragma once

#include <vector>

#include "gen-cpp2/LucidaService.h"
#include "../opencv/Image.h"
#include "client/dbclient.h" // MongoDB

namespace cpp2 {
class IMMHandler : virtual public LucidaServiceSvIf {
public:
	IMMHandler();

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
	void addImage(const std::string &LUCID,
			const std::string &label, const std::string &data);

	std::vector<std::unique_ptr<StoredImage>> getImages(
			const std::string &LUCID);

	std::unique_ptr<mongo::DBClientBase> getConnection();

};
}
