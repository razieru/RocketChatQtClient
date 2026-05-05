#pragma once

#include <memory>

#include "isessionstore.h"

namespace rc {

struct SessionStoreBuildResult {
	std::unique_ptr<ISessionStore> store;
	bool usingSecureStorage = false;
};

class SessionStoreFactory {
public:
	static SessionStoreBuildResult createDefault();
};

} // namespace rc
