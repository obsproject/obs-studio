import assert from "node:assert/strict";
import { test } from "node:test";
import { createTwitch, InvalidTwitchToken } from "../src/twitch.mjs";
import { verifyTwitch } from "../scripts/verify-twitch.mjs";

test("Twitch code exchange uses the registered callback and server-side secret, without extra scopes", async () => {
  const requests = [];
  const twitch = createTwitch({ clientId: "client", clientSecret: "server-secret", origin: "https://api.aerium.tv",
    async fetcher(url, options) {
      requests.push({ url: String(url), options });
      if (String(url).endsWith("/token")) return Response.json({ access_token: "provider-access", refresh_token: "provider-refresh", token_type: "bearer", expires_in: 14400, scope: [] });
      if (String(url).endsWith("/validate")) return Response.json({ client_id: "client", user_id: "123", expires_in: 14400 });
      return Response.json({ data: [{ id: "123", login: "tester", display_name: "Tester", profile_image_url: "" }] });
    } });
  const url = new URL(twitch.authorizationUrl("state"));
  assert.equal(url.searchParams.get("scope"), "");
  assert.equal(url.searchParams.has("client_secret"), false);
  const callback = "https://api.aerium.tv/v1/auth/twitch/callback?code=code&state=state";
  const result = await twitch.exchange(callback, "state");
  assert.equal(result.user.id, "123");
  assert.equal(result.tokens.refresh_token, "provider-refresh");
  const form = new URLSearchParams(requests[0].options.body);
  assert.equal(form.get("client_secret"), "server-secret");
  assert.equal(form.get("redirect_uri"), "https://api.aerium.tv/v1/auth/twitch/callback");
  await assert.rejects(() => twitch.exchange(callback, "wrong-state"));
});

test("Twitch identity must belong to our OAuth application", async () => {
  const twitch = createTwitch({ clientId: "ours", clientSecret: "secret", origin: "https://api.aerium.tv",
    fetcher: async () => Response.json({ client_id: "other-app", user_id: "123", expires_in: 14400 }) });
  await assert.rejects(() => twitch.validate("access"), InvalidTwitchToken);
});

test("Twitch refresh stores rotated refresh tokens and identifies revoked grants", async () => {
  let rejected = false;
  const twitch = createTwitch({ clientId: "client", clientSecret: "secret", origin: "https://api.aerium.tv",
    fetcher: async () => rejected ? Response.json({ error: "invalid_grant" }, { status: 400 }) :
      Response.json({ access_token: "new-access", refresh_token: "new-refresh", token_type: "bearer", expires_in: 14400, scope: [] }) });
  assert.equal((await twitch.refresh({ refresh_token: "old" })).refresh_token, "new-refresh");
  rejected = true;
  await assert.rejects(() => twitch.refresh({ refresh_token: "old" }), InvalidTwitchToken);
});

test("Twitch-specific invalid refresh responses require a new login", async () => {
  for (const status of [400, 401]) {
    const twitch = createTwitch({ clientId: "client", clientSecret: "secret", origin: "https://api.aerium.tv",
      fetcher: async () => Response.json({ error: "Bad Request", status, message: "Invalid refresh token" }, { status }) });
    await assert.rejects(() => twitch.refresh({ refresh_token: "old" }), InvalidTwitchToken);
  }
});

test("deployment verification returns only public IDs and revokes its temporary token", async () => {
  const env = { TWITCH_CLIENT_ID: "client", TWITCH_CLIENT_SECRET: "secret", AERIUM_VERIFY_TWITCH_USERS: "first,second",
    AERIUM_LOGIN_ENABLED: "true", AERIUM_ALLOWED_TWITCH_IDS: "123,456" };
  let revocations = 0;
  const fetcher = async (url, options) => {
    if (url.endsWith("/token")) {
      assert.equal(options.body.get("client_secret"), "secret");
      return Response.json({ access_token: "private-token" });
    }
    if (url.endsWith("/revoke")) {
      assert.equal(options.body.get("token"), "private-token");
      revocations++;
      return new Response(null, { status: 200 });
    }
    return Response.json({ data: [{ login: "first", id: "123", email: "not-for-logs" }, { login: "second", id: "456" }] });
  };
  assert.deepEqual(await verifyTwitch({ env, fetcher }), { twitch_verified: true, login_enabled: true,
    testers: [{ login: "first", id: "123" }, { login: "second", id: "456" }] });
  assert.equal(revocations, 1);
  await assert.rejects(() => verifyTwitch({ env: { ...env, AERIUM_ALLOWED_TWITCH_IDS: "123,789" }, fetcher }), /allowlist validation/);
  assert.equal(revocations, 2);
});

test("deployment verification suppresses credential-bearing provider errors", async () => {
  await assert.rejects(() => verifyTwitch({ env: { TWITCH_CLIENT_ID: "client", TWITCH_CLIENT_SECRET: "secret",
    AERIUM_VERIFY_TWITCH_USERS: "first,second" }, fetcher: async () => { throw new Error("secret private-token"); } }),
  { message: "Twitch deployment check failed during credential validation; credential values withheld." });
});