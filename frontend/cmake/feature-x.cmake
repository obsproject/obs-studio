# X Livestream API (OAuth 2.0 user context).
#
# The maintainer supplies an X app client id and secret. Do not commit them.
# The app must be enabled for the Livestream API (Enterprise). Request access:
#   https://docs.x.com/forms/livestream-api-access
# Scopes: broadcast.read, broadcast.write, offline.access, plus users.read so
# GET /2/users/me can supply the numeric :user_id required on source and
# broadcast paths.
# Register this exact redirect URI on the app:
#   http://127.0.0.1:42813/callback
#
# A hash of 0 leaves the value as plaintext (the deobfuscate step XORs with 0).
# Non-zero hashes use the same deobfuscate_str scheme as the YouTube client.
#
#   cmake -DX_CLIENTID=... -DX_SECRET=... -DX_CLIENTID_HASH=0 -DX_SECRET_HASH=0
#
# CI reads X_CLIENTID, X_CLIENTID_HASH, X_SECRET, and X_SECRET_HASH from the
# environment via the environmentVars configure preset. When they are unset the
# service entry still appears, and the account connection stays compiled out.

if(
  X_CLIENTID
  AND X_SECRET
  AND X_CLIENTID_HASH MATCHES "^(0|[a-fA-F0-9]+)$"
  AND X_SECRET_HASH MATCHES "^(0|[a-fA-F0-9]+)$"
)
  target_sources(
    obs-studio
    PRIVATE
      dialogs/OBSXBroadcastActions.cpp
      dialogs/OBSXBroadcastActions.hpp
      oauth/XAuth.cpp
      oauth/XAuth.hpp
      utility/XApiWrappers.cpp
      utility/XApiWrappers.hpp
  )

  target_enable_feature(obs-studio "X Broadcasts API connection" X_ENABLED)
else()
  target_disable_feature(obs-studio "X Broadcasts API connection")
  set(X_CLIENTID "")
  set(X_SECRET "")
  set(X_CLIENTID_HASH 0)
  set(X_SECRET_HASH 0)
endif()
