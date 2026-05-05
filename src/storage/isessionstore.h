#pragma once

#include "../network/apitypes.h"

namespace rc {

class ISessionStore {
public:
	virtual ~ISessionStore() = default;
	virtual bool save(const SessionData& session) = 0;
	virtual SessionData load() const = 0;
	virtual void clear() = 0;
};

} // namespace rc
