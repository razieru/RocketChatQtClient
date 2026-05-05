#include "sessionstorefactory.h"

#include "keychainsessionstore.h"
#include "settingssessionstore.h"

namespace rc {

SessionStoreBuildResult SessionStoreFactory::createDefault() {
#ifdef RC_HAS_QTKEYCHAIN
	auto secureStore = std::make_unique<KeychainSessionStore>();
	if (secureStore->isAvailable()) {
		// One-shot migration from legacy QSettings-only storage to keychain-backed storage.
		SettingsSessionStore legacyStore;
		const SessionData secureSession = secureStore->load();
		const SessionData legacySession = legacyStore.load();
		if (!secureSession.isValid() && legacySession.isValid()) {
			secureStore->save(legacySession);
			legacyStore.clear();
		}

		return SessionStoreBuildResult{
			.store = std::move(secureStore),
			.usingSecureStorage = true,
		};
	}
#endif

	return SessionStoreBuildResult{
		.store = std::make_unique<SettingsSessionStore>(),
		.usingSecureStorage = false,
	};
}

} // namespace rc
