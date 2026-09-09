import assert from "node:assert/strict";
import { after, test } from "node:test";
import { randomBytes } from "node:crypto";
import { readFile } from "node:fs/promises";
import { PGlite } from "@electric-sql/pglite";
import { createApp, handleRequest } from "../src/app.mjs";
import { createStore } from "../src/store.mjs";
import { createVault, hash, randomToken } from "../src/security.mjs";
import { InvalidTwitchToken } from "../src/twitch.mjs";

test("health is available without credentials and is not cached", async () => {
  const response = await handleRequest(new Request("https://api.aerium.tv/health"));
  assert.equal(response.status, 200);
  assert.equal(response.headers.get("cache-control"), "no-store");
  assert.deepEqual(await response.json(), { service: "aerium-api", status: "ok" });
});

test("unknown endpoints fail closed", async () => {
  const response = await handleRequest(new Request("https://api.aerium.tv/auth/twitch"));
  assert.equal(response.status, 404);
});

const database = new PGlite();
await database.exec(await readFile(new URL("../src/schema.sql", import.meta.url), "utf8"));
const store = createStore(async (text, values) => (await database.query(text, values)).rows);
const vault = createVault(randomBytes(32).toString("base64"));
let revoked = false;
let validations = 0;
const twitch = {
  authorizationUrl: (state) => `https://id.twitch.tv/oauth2/authorize?state=${state}`,
  async exchange() {
    return { user: { id: "123", login: "tester", display_name: "Tester", profile_image_url: "" },
      tokens: { access_token: "twitch-access", refresh_token: "twitch-refresh", expires_at: Date.now() + 14400000 } };
  },
  async validate() {
    validations++;
    if (revoked) throw new InvalidTwitchToken();
    return { user_id: "123", expires_in: 14400 };
  },
  async revoke() { revoked = true; },
};
const app = createApp({ store, twitch, vault, allowedIds: ["123"], enabled: true });
const request = (path, { method = "GET", body, token, headers = {} } = {}) => new Request(`https://api.aerium.tv${path}`, {
  method, headers: { ...(body ? { "Content-Type": "application/json" } : {}),
    ...(token ? { Authorization: `Bearer ${token}` } : {}), ...headers }, body: body ? JSON.stringify(body) : undefined,
});
after(() => database.close());

test("unconfigured login is disabled, not a simulated login", async () => {
  assert.equal((await handleRequest(request("/v1/auth/desktop", { method: "POST", body: {} }))).status, 503);
});

test("desktop login keeps provider tokens private and rejects CSRF, replay, and wrong verifiers", async () => {
  const verifier = randomToken();
  const start = await app(request("/v1/auth/desktop", { method: "POST", body: { code_challenge: hash(verifier) } }));
  assert.equal(start.status, 201);
  const attempt = await start.json();
  const authorization = await app(new Request(attempt.authorization_url));
  assert.equal(authorization.status, 302);
  const state = new URL(authorization.headers.get("location")).searchParams.get("state");
  const cookie = authorization.headers.get("set-cookie").split(";")[0];
  const callbackPath = `/v1/auth/twitch/callback?code=code&state=${state}`;
  assert.equal((await app(request(callbackPath))).status, 400);
  assert.equal((await app(request(callbackPath, { headers: { Cookie: cookie } }))).status, 200);
  assert.equal((await app(request(callbackPath, { headers: { Cookie: cookie } }))).status, 400);
  assert.equal((await app(request("/v1/auth/exchange", { method: "POST", body: { request_id: attempt.request_id, code_verifier: randomToken() } }))).status, 400);
  const exchange = await app(request("/v1/auth/exchange", { method: "POST", body: { request_id: attempt.request_id, code_verifier: verifier } }));
  assert.equal(exchange.status, 200);
  const tokens = await exchange.json();
  assert(!JSON.stringify(tokens).includes("twitch-"));
  assert.equal((await app(request("/v1/auth/exchange", { method: "POST", body: { request_id: attempt.request_id, code_verifier: verifier } }))).status, 400);
  const profile = await app(request("/v1/me", { token: tokens.access_token }));
  assert.equal(profile.status, 200);
  assert.equal((await profile.json()).user.login, "tester");
  const saved = (await database.query("SELECT * FROM accounts")).rows[0];
  assert(!saved.token_payload.includes("twitch-refresh"));
  assert.equal(vault.decrypt(saved.token_payload, "123").refresh_token, "twitch-refresh");
  const storedSession = (await database.query("SELECT * FROM sessions")).rows[0];
  assert.equal(storedSession.access_hash, hash(tokens.access_token));
  const validated = await app(request("/v1/session/validate", { method: "POST", token: tokens.access_token }));
  assert.equal(validated.status, 200);
  assert.equal(validations, 1);
  const refreshed = await app(request("/v1/session/refresh", { method: "POST", body: { refresh_token: tokens.refresh_token } }));
  assert.equal(refreshed.status, 200);
  const next = await refreshed.json();
  assert.equal((await app(request("/v1/me", { token: tokens.access_token }))).status, 401);
  assert.equal((await app(request("/v1/session/refresh", { method: "POST", body: { refresh_token: tokens.refresh_token } }))).status, 401);
  revoked = true;
  assert.equal((await app(request("/v1/session/validate", { method: "POST", token: next.access_token }))).status, 401);
  assert.equal((await app(request("/v1/me", { token: next.access_token }))).status, 401);
  assert.equal((await app(request("/v1/session/refresh", { method: "POST", body: { refresh_token: next.refresh_token } }))).status, 401);
});

test("cross-origin, invalid JSON and oversized requests are rejected", async () => {
  assert.equal((await app(request("/v1/auth/desktop", { method: "POST", body: {}, headers: { Origin: "https://evil.example" } }))).status, 403);
  assert.equal((await app(request("/v1/auth/desktop", { method: "POST", body: { code_challenge: "short" } }))).status, 400);
  assert.equal((await app(request("/v1/auth/desktop", { method: "POST", body: { code_challenge: "x".repeat(9000) } }))).status, 413);
  assert.equal((await app(request("/v1/me"))).status, 401);
});

test("rotated Twitch credentials survive a temporary validation failure; logout and deletion revoke sessions", async () => {
  const id = randomToken();
  const verifier = randomToken();
  await store.createLogin(id, hash(verifier));
  await store.beginLogin(id, "refresh-state");
  await store.claimCallback("refresh-state");
  const account = await store.completeLogin(id, { id: "456", login: "second", display_name: "Second", profile_image_url: "" },
    vault.encrypt({ access_token: "expired", refresh_token: "old", expires_at: 0 }, "456"));
  await database.query("UPDATE accounts SET validated_at = now() - interval '2 hours' WHERE id = $1", [account.id]);
  const access = randomToken();
  const refresh = randomToken();
  await store.exchangeLogin(id, hash(verifier), hash(access), hash(refresh));
  let offline = true;
  let refreshes = 0;
  let revocations = 0;
  const refreshApp = createApp({ store, vault, allowedIds: ["456"], enabled: true, twitch: {
    async refresh() {
      refreshes++;
      return { access_token: "rotated-access", refresh_token: "rotated-refresh", expires_at: Date.now() + 14400000 };
    },
    async validate() {
      if (offline) throw new Error("Upstream network detail must not escape");
      return { user_id: "456", expires_in: 14400 };
    },
    async revoke() { revocations++; },
  } });
  const unavailable = await refreshApp(request("/v1/me", { token: access }));
  assert.equal(unavailable.status, 503);
  assert.deepEqual(await unavailable.json(), { error: "service_unavailable" });
  const saved = await store.accessSession(hash(access));
  assert.equal(vault.decrypt(saved.token_payload, "456").refresh_token, "rotated-refresh");
  offline = false;
  assert.equal((await refreshApp(request("/v1/me", { token: access }))).status, 200);
  assert.equal(refreshes, 1);
  assert.equal((await refreshApp(request("/v1/session/logout", { method: "POST", token: refresh }))).status, 204);
  assert.equal((await refreshApp(request("/v1/me", { token: access }))).status, 401);
  await database.query("INSERT INTO sessions (account_id, access_hash, refresh_hash) VALUES ($1, $2, $3)", [account.id, hash(access), hash(refresh)]);
  assert.equal((await refreshApp(request("/v1/account", { method: "DELETE", token: access }))).status, 204);
  assert.equal(revocations, 1);
  assert.equal(await store.refreshSession(hash(refresh)), undefined);
});