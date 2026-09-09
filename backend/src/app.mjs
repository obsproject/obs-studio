import { equal, hash, randomToken, validToken, validVerifier } from "./security.mjs";
import { InvalidTwitchToken } from "./twitch.mjs";

const securityHeaders = {
  "Cache-Control": "no-store",
  "X-Content-Type-Options": "nosniff",
  "Referrer-Policy": "no-referrer",
  "Content-Security-Policy": "default-src 'none'; frame-ancestors 'none'; base-uri 'none'",
  "Strict-Transport-Security": "max-age=31536000",
};
const cookieName = "__Host-aerium-oauth";
const cookie = (value, age) => `${cookieName}=${value}; Path=/; HttpOnly; Secure; SameSite=Lax; Max-Age=${age}`;
const json = (value, status = 200, headers = {}) => Response.json(value, { status, headers: { ...securityHeaders, ...headers } });
const fail = (status, code) => Object.assign(new Error(code), { status, code });
const userProfile = (account) => ({ id: account.id, twitch_id: account.twitch_id, login: account.login,
  display_name: account.display_name, avatar_url: account.avatar_url });

async function readBody(request) {
  if (request.headers.get("content-type")?.split(";")[0] !== "application/json") throw fail(415, "json_required");
  const body = await request.text();
  if (Buffer.byteLength(body) > 8192) throw fail(413, "body_too_large");
  try {
    const parsed = JSON.parse(body);
    if (!parsed || typeof parsed !== "object" || Array.isArray(parsed)) throw new Error();
    return parsed;
  } catch {
    throw fail(400, "invalid_json");
  }
}

function bearer(request) {
  const value = request.headers.get("authorization")?.match(/^Bearer ([A-Za-z0-9_-]{43})$/)?.[1];
  if (!value) throw fail(401, "unauthorized");
  return value;
}

export function createApp({ store, twitch, vault, origin = "https://api.aerium.tv", allowedIds = [], enabled = false } = {}) {
  const configured = Boolean(enabled && store && twitch && vault && allowedIds.length);

  async function ensureTwitch(account, force = false) {
    if (!allowedIds.includes(account.twitch_id)) throw fail(403, "not_in_private_beta");
    if (!force && Date.now() - new Date(account.validated_at).getTime() < 3600000) return;
    if (!await store.acquireLease(account.id, account.token_version)) throw fail(503, "retry_later");
    try {
      let tokens = vault.decrypt(account.token_payload, account.twitch_id);
      if (tokens.expires_at <= Date.now() + 60000) {
        tokens = await twitch.refresh(tokens);
        if (!await store.persistRefresh(account.id, account.token_version, vault.encrypt(tokens, account.twitch_id))) {
          throw fail(503, "retry_later");
        }
      }
      const identity = await twitch.validate(tokens.access_token);
      if (identity.user_id !== account.twitch_id) throw new InvalidTwitchToken();
      tokens.expires_at = Date.now() + identity.expires_in * 1000;
      if (!await store.saveTokens(account.id, account.token_version, vault.encrypt(tokens, account.twitch_id))) {
        throw fail(503, "retry_later");
      }
    } catch (error) {
      if (error instanceof InvalidTwitchToken) {
        await store.invalidateAccount(account.id, account.token_version);
        throw fail(401, "twitch_disconnected");
      }
      throw error;
    } finally {
      await store.releaseLease(account.id, account.token_version);
    }
  }

  return async function handle(request, { ip = "unknown" } = {}) {
    const url = new URL(request.url);
    const route = `${request.method} ${url.pathname}`;
    let callbackId;
    const callback = url.pathname === "/v1/auth/twitch/callback";
    try {
      if (route === "GET /health") return json({ service: "aerium-api", status: "ok" });
      if (route === "GET /") return json({ service: "aerium-api", stage: "private-development", media: "direct-peer-to-peer-only" });
      if (route === "GET /ready") {
        if (store) await store.ready();
        return json({ database: store ? "ready" : "not_configured", twitch_login: configured ? "ready" : "not_configured" }, configured ? 200 : 503);
      }
      const routes = ["POST /v1/auth/desktop", "GET /v1/auth/twitch", "GET /v1/auth/twitch/callback",
        "POST /v1/auth/exchange", "GET /v1/me", "POST /v1/session/validate", "POST /v1/session/refresh",
        "POST /v1/session/logout", "DELETE /v1/account"];
      if (!routes.includes(route)) return json({ error: "not_found" }, 404);
      if (!configured) throw fail(503, "login_not_configured");
      const requestOrigin = request.headers.get("origin");
      if (requestOrigin && requestOrigin !== origin) throw fail(403, "origin_not_allowed");
      if (!await store.rateLimit(vault.rateKey(`request:${ip}`), 180, 60)) throw fail(429, "rate_limited");

      if (route === "POST /v1/auth/desktop") {
        const { code_challenge: challenge } = await readBody(request);
        if (!validToken(challenge)) throw fail(400, "invalid_challenge");
        if (!await store.rateLimit(vault.rateKey(`login:${ip}`), 10, 600) ||
            !await store.rateLimit("login-global", 1000, 86400)) throw fail(429, "rate_limited");
        await store.cleanup();
        const id = randomToken();
        await store.createLogin(id, challenge);
        return json({ request_id: id, authorization_url: `${origin}/v1/auth/twitch?request=${id}`, expires_in: 600, interval: 5 }, 201);
      }
      if (route === "GET /v1/auth/twitch") {
        const id = url.searchParams.get("request");
        if (!validToken(id)) throw fail(400, "invalid_request");
        const state = randomToken();
        if (!await store.beginLogin(id, hash(state))) throw fail(400, "login_expired_or_used");
        return new Response(null, { status: 302, headers: { ...securityHeaders,
          Location: twitch.authorizationUrl(state), "Set-Cookie": cookie(state, 600) } });
      }
      if (route === "GET /v1/auth/twitch/callback") {
        const state = url.searchParams.get("state");
        const browserState = request.headers.get("cookie")?.split(";").map((part) => part.trim())
          .find((part) => part.startsWith(`${cookieName}=`))?.slice(cookieName.length + 1);
        if (!validToken(state) || !equal(state, browserState) || url.searchParams.getAll("state").length !== 1) {
          throw fail(400, "invalid_oauth_state");
        }
        const attempt = await store.claimCallback(hash(state));
        if (!attempt) throw fail(400, "login_expired_or_used");
        callbackId = attempt.id;
        if (url.searchParams.has("error")) throw fail(400, "twitch_authorization_declined");
        const { user, tokens } = await twitch.exchange(url, state);
        if (!allowedIds.includes(user.id)) {
          await twitch.revoke(tokens.access_token);
          throw fail(403, "not_in_private_beta");
        }
        if (!await store.completeLogin(attempt.id, user, vault.encrypt(tokens, user.id))) throw fail(400, "login_expired_or_used");
        return new Response("Signed in. You can return to Aerium.", { headers: { ...securityHeaders,
          "Content-Type": "text/plain; charset=utf-8", "Set-Cookie": cookie("", 0) } });
      }
      if (route === "POST /v1/auth/exchange") {
        const { request_id: id, code_verifier: verifier } = await readBody(request);
        if (!validToken(id) || !validVerifier(verifier)) throw fail(400, "invalid_request");
        if (!await store.rateLimit(vault.rateKey(`poll:${id}`), 15, 60)) throw fail(429, "rate_limited");
        const challenge = hash(verifier);
        const attempt = await store.loginStatus(id, challenge);
        if (!attempt) throw fail(400, "login_expired_or_used");
        if (attempt.status === "denied") throw fail(403, "authorization_denied");
        if (attempt.status !== "complete") return json({ status: "authorization_pending" }, 202, { "Retry-After": "5" });
        const access = randomToken();
        const refresh = randomToken();
        if (!await store.exchangeLogin(id, challenge, hash(access), hash(refresh))) throw fail(400, "login_expired_or_used");
        return json({ token_type: "Bearer", access_token: access, refresh_token: refresh, expires_in: 900, refresh_expires_in: 2592000 });
      }
      if (route === "POST /v1/session/logout") {
        await store.logout(hash(bearer(request)));
        return new Response(null, { status: 204, headers: securityHeaders });
      }
      if (route === "POST /v1/session/refresh") {
        const { refresh_token: refresh } = await readBody(request);
        if (!validToken(refresh)) throw fail(401, "unauthorized");
        const account = await store.refreshSession(hash(refresh));
        if (!account) throw fail(401, "unauthorized");
        await ensureTwitch(account);
        const access = randomToken();
        const nextRefresh = randomToken();
        const rotated = await store.rotateSession(hash(refresh), hash(access), hash(nextRefresh));
        if (!rotated) throw fail(401, "unauthorized");
        return json({ token_type: "Bearer", access_token: access, refresh_token: nextRefresh, expires_in: 900,
          refresh_expires_in: Math.max(0, Math.floor((new Date(rotated.expires_at).getTime() - Date.now()) / 1000)) });
      }
      const account = await store.accessSession(hash(bearer(request)));
      if (!account) throw fail(401, "unauthorized");
      await ensureTwitch(account, route === "POST /v1/session/validate");
      if (route === "DELETE /v1/account") {
        const latest = await store.accessSession(hash(bearer(request)));
        if (!latest) throw fail(401, "unauthorized");
        await twitch.revoke(vault.decrypt(latest.token_payload, latest.twitch_id).access_token);
        await store.deleteAccount(account.id);
        return new Response(null, { status: 204, headers: securityHeaders });
      }
      return json({ user: userProfile(account) });
    } catch (error) {
      if (callbackId) await store.denyLogin(callbackId).catch(() => {});
      const knownError = Number.isInteger(error.status) && typeof error.code === "string";
      const status = knownError ? error.status : 503;
      return json({ error: knownError ? error.code : "service_unavailable" }, status, {
        ...(callback ? { "Set-Cookie": cookie("", 0) } : {}),
        ...([429, 503].includes(status) ? { "Retry-After": "5" } : {}),
      });
    }
  };
}

export const handleRequest = createApp();