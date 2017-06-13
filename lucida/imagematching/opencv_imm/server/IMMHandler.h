#pragma once

#include <vector>
#include <mutex>

#include "gen-cpp2/LucidaService.h"
#include "Image.h"
#include "mongo/client/dbclient.h"

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
	mongo::DBClientConnection conn;



	int countImages(const std::string &LUCID);

	void addImage(const std::string &LUCID,
			const std::string &label, const std::string &data);

	void deleteImage(const std::string &LUCID,
			const std::string &label);

	std::vector<std::unique_ptr<StoredImage>> getImages(
			const std::string &LUCID);

	std::string getImageLabelFromId(
		const std::string &LUCID, const std::string &image_id);

	std::unique_ptr<mongo::DBClientBase> getConnection();
};
}
