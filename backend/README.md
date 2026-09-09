# Aerium Backend

Private development API on Vercel Hobby with Neon Free PostgreSQL. Node 24, plain JavaScript ES modules, `oauth4webapi`, and the Neon HTTP driver. It does not serve or relay audio/video.

## Deployment

- Project: `aerium-api` in `chris-projects-0fe03e95`.
- API: https://api.aerium.tv (the apex `aerium.tv` is unchanged).
- Database: `aerium-auth-dev`, Neon Free, `iad1`, connected to production only.
- Function region: `iad1`; maximum invocation duration: 60 seconds.
- Vercel's environment label is **Production**, but this is a private development service, not a public product launch. Hobby's personal/non-commercial restrictions still apply.
- `GET /health` reports function health. `GET /ready` checks the schema and login configuration; it intentionally returns 503 until Twitch setup is complete.
- No desktop integration, friends, Whispers, presence, or WebRTC signaling endpoints are included yet. Future media is direct P2P only: no TURN, SFU, or server media relay.

## Enable Twitch Login

1. Register an Aerium-owned application at https://dev.twitch.tv/console/apps. Use a **Confidential** client because the backend owns the secret. Do not use OBS credentials or its auth service.
2. Set the exact OAuth redirect URI to `https://api.aerium.tv/v1/auth/twitch/callback`.
3. In the Vercel project's **Settings > Environment Variables**, add `TWITCH_CLIENT_ID` and `TWITCH_CLIENT_SECRET` for **Production only**. Mark the secret sensitive. Enter it directly in Vercel, never in chat, source, or command-line arguments.
4. Set `AERIUM_ALLOWED_TWITCH_IDS` to comma-separated numeric Twitch IDs for the initial testers. Usernames are not IDs. The backend checks these against the verified identity, never a client-supplied identity. No public-registration wildcard is supported.
5. The database URL, `AERIUM_ORIGIN`, and `AERIUM_TOKEN_KEY` are already configured. Do not replace the encryption key casually: existing encrypted provider credentials would become unreadable. Production secrets must never be copied to preview environments.
6. Set `AERIUM_LOGIN_ENABLED=true` and redeploy. Confirm `/ready` returns 200, then perform a real Twitch consent, logout, expiry/refresh, and disconnect test before relying on login.

The current deployment has login enabled for VastOnline (`476597509`) and SplorgeGG (`28271846`) only. Production build verification has confirmed the real Twitch credentials and exact allowlist. Live API checks cover readiness, authorization initiation, browser-state protection, pending handoff, and cancellation. Unit tests use a stub Twitch provider; a complete user-consent login and native desktop credential storage still require verification/implementation.

## Desktop Protocol

Use HTTPS and JSON. No database credentials, Twitch client secret, or Twitch provider tokens belong in the desktop binary. No browser CORS access is granted to arbitrary websites.

1. Generate a cryptographically random verifier (32 random bytes encoded as base64url produces 43 characters). Keep it in memory; compute `base64url(SHA256(verifier))`.
2. `POST /v1/auth/desktop` with `{"code_challenge":"..."}`. The response contains `request_id`, `authorization_url`, a 600-second expiry, and a 5-second poll interval.
3. Open `authorization_url` in the system browser. A secure, HttpOnly, SameSite=Lax cookie binds the OAuth callback to that browser. Only one pending authorization per browser cookie is supported.
4. Poll `POST /v1/auth/exchange` with `{"request_id":"...","code_verifier":"..."}` no faster than once every five seconds. Pending authorization returns 202. Expired/used attempts return 400, denial returns 403. On 429/503 respect `Retry-After` and use bounded backoff.
5. Successful exchange returns a 15-minute Aerium access token and a rotating refresh credential with an absolute 30-day lifetime. Store the refresh credential in the OS credential vault, not OBS profile configuration. Keep the access token in memory. The verifier is a proof for the Aerium handoff, not a claim of Twitch PKCE support.
6. Use `Authorization: Bearer <access_token>` for `GET /v1/me`. Its response contains only the Aerium ID and public Twitch profile fields.
7. Call `POST /v1/session/validate` with the bearer token at desktop startup and at least hourly while connected. It forces Twitch validation; ordinary authenticated requests also validate when the cached result is an hour old. There is no always-on background session or EventSub subscription in this version.
8. `POST /v1/session/refresh` with `{"refresh_token":"..."}` rotates both Aerium credentials. Replace the saved refresh credential atomically. Old credentials immediately stop working. After an ambiguous lost refresh response, re-authenticate rather than repeatedly reusing the old credential.
9. `POST /v1/session/logout` accepts either current Aerium credential as a bearer token, deletes that device session, and returns 204. Always clear local credentials as well. Logout does not disconnect Twitch for other devices.
10. `DELETE /v1/account` with the access token revokes the stored Twitch access token and deletes the Aerium account, stored credentials, pending attempts, and all device sessions. A Twitch outage causes a retryable error rather than falsely reporting successful revocation/deletion. Users can also disconnect Aerium in Twitch Connections settings; subsequent validation invalidates local sessions and clears stored provider credentials.

Core recording must remain usable without logging in or reaching this backend.

## Security And Limits

- Twitch tokens use AES-256-GCM with an identity-bound authentication tag. The 32-byte base64 key lives in Vercel, separately from Neon. Re-encryption/key versioning is required before a future key rotation.
- Aerium credentials are random 256-bit values; only SHA-256 hashes are stored. Handoff and refresh consumption are atomic SQL operations.
- Token refresh is serialized using a database lease. Rotated Twitch credentials are persisted before validation; the tiny external-provider/database failure window still needs a fresh consent if a refresh is lost.
- Initial authorization requests no email, streaming, or Whisper scopes. Add feature-specific consent later rather than over-scoping login.
- Shared database limits: 10 login starts per IP per 10 minutes, 1,000 starts globally per day, 180 API requests per IP per minute, 15 exchange polls per attempt per minute. Keys use an HMAC of Vercel's trusted IP header; raw IPs are not stored in these tables. Fixed-window limits can allow a burst at a window boundary and do not replace platform DDoS protection.
- Expired attempts, rate buckets, and sessions are removed opportunistically on new login starts, not by an always-on worker. Expiry is enforced on reads even before cleanup.
- No request/token payload logging. Do not add verbose OAuth logging; platform request logs may still contain short-lived callback query parameters, so restrict log access and retention.
- Free tiers are bounded, not unlimited. Keep the private allowlist and monitor Vercel/Neon usage. No paid plans or automatic recharge were enabled.

## Development And Verification

From this directory with Node 24:

```sh
npm ci
npm test
```

Tests run actual PostgreSQL SQL in disposable PGlite databases and stub only Twitch's network API. They cover OAuth response validation, Twitch-specific token formats, encrypted storage, browser state, proof-bound handoff, single-use callbacks, refresh rotation, revocation, outage recovery, account deletion, and HTTP routing.

Vercel exports **redacted placeholders** for sensitive environment variables. Their presence in an `env pull` file does not mean their real values are available locally. Do not send these placeholders to Twitch or downgrade a secret's protection to debug it.

For credential checks, the build supports an opt-in `AERIUM_VERIFY_TWITCH_USERS` setting. The check runs inside Vercel Production, uses the protected credentials, resolves the two requested usernames, verifies the allowlist when login is enabled, and revokes its temporary app token. It reports only public usernames/IDs and activation status, never provider credentials or tokens. It does not perform user consent or change configuration.

To migrate the configured development service without printing database secrets:

```sh
vercel env pull .env.production.local --environment production --scope chris-projects-0fe03e95
chmod 600 .env.production.local
npm run db:migrate
```

The migration is atomic and idempotent. Use a separate Neon database for local experimentation; do not run destructive tests against the deployed database. Environment files, provider-generated agent skills, dependencies, and `.vercel` are ignored; environment files and source are not public assets.

Deploy from the repository root:

```sh
npm --prefix backend test
vercel deploy --prod --yes --cwd backend --scope chris-projects-0fe03e95
```

To also verify the current two-account Twitch configuration during deployment:

```sh
vercel deploy --prod --yes --cwd backend --scope chris-projects-0fe03e95 --build-env AERIUM_VERIFY_TWITCH_USERS=vastonline,splorgegg
```

Deployment currently uses the local backend folder, not automatic GitHub deployment. No commits or pushes are made by this setup. Existing desktop/theme changes remain independent.