# Rocket.Chat Qt Client (C++ / Qt 6.8.3)

This folder contains a standalone Qt-based C++ client skeleton for Rocket.Chat:

- login with `user/password` (`/api/v1/login`)
- interactive 2FA flow (`x-2fa-code`, `x-2fa-method`)
- session persistence (`authToken`, `userId`, server URL)
- session restore + validation (`/api/v1/me`)
- authenticated requests (`X-Auth-Token`, `X-User-Id`)
- sample data request (`/api/v1/users.info`)
- QML UI with login form and full `userinfo` JSON output
- state-driven QML screens: `Login` -> `TwoFactor` -> `Authenticated`

## Structure

- `src/network/rocketchatclient.*` - high-level API for app/UI
- `src/network/httptransport.*` - HTTP transport on top of `QNetworkAccessManager`
- `src/network/apitypes.h` - DTO/error/session types
- `src/storage/isessionstore.h` - session storage abstraction
- `src/storage/settingssessionstore.*` - `QSettings` implementation
- `src/main.cpp` - minimal console demo

## Build (Windows, CMake)

### 1) Install dependencies

- Qt 6.8.3 (MSVC kit, e.g. `msvc2022_64`)
- `qtkeychain` for secure storage (recommended, optional)

If `qtkeychain` is not installed, configure/build still works and the app automatically falls back to `QSettings` session storage.
You can also explicitly disable keychain support with `-DRC_ENABLE_QTKEYCHAIN=OFF`.

### 2) Configure and build

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64;C:/dev/qtkeychain"
cmake --build build --config Release
```

If Qt/qtkeychain are installed in different directories, update `CMAKE_PREFIX_PATH` accordingly.

When `RC_ENABLE_QTKEYCHAIN=ON` and qtkeychain is missing, CMake prints a status message and enables automatic fallback to `QSettings`.

### 3) Run tests

```powershell
ctest --test-dir build -C Release --output-on-failure
```

### 4) Run app

```powershell
.\build\Release\rocketchat_qt_client.exe
```

## Notes

- Current implementation uses `KeychainSessionStore` (secure) when qtkeychain is available.
- `SettingsSessionStore` is used automatically as fallback backend when qtkeychain is unavailable.
- One-shot legacy migration is included: existing `QSettings` session is moved to keychain on first run.
- Supported 2FA methods: `totp`, `email`, `password`.
- 2FA UX flow:
  1. Call `login(user, password)`.
  2. If server requires 2FA, client emits `twoFactorRequired(method, emailOrUsername, invalidAttempt)`.
  3. Collect code in UI and call `submitTwoFactorCode(code, method)`.
- Realtime (DDP/WebSocket) is intentionally not implemented in this iteration.

## QML Smoke Scenario

1. Launch the app.
2. Fill:
   - `Server URL` (for example `http://localhost:3000`)
   - `Username`
   - `Password`
3. Click `Login`.
4. If prompted for 2FA, enter code and submit.
5. On success, the app fetches `/api/v1/me` and displays full user info as raw JSON in `User Info JSON`.

## QML Screen States

- `Login`: shows `qml/components/LoginForm.qml` with `serverUrl`, `username`, `password`.
- `TwoFactor`: shows method/hint and code submission UI.
- `Authenticated`: shows logout action and full `userinfo` JSON payload.

Transition flow:
- `Login -> TwoFactor`: when server requires 2FA.
- `Login -> Authenticated`: on successful login or restored session.
- `TwoFactor -> Authenticated`: after valid 2FA code.
- `Authenticated -> Login`: on logout or invalid session restore.
