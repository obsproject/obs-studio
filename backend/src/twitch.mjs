import * as oauth from "oauth4webapi";

export class InvalidTwitchToken extends Error {}

const server = {
  issuer: "https://id.twitch.tv/oauth2",
  authorization_endpoint: "https://id.twitch.tv/oauth2/authorize",
  token_endpoint: "https://id.twitch.tv/oauth2/token",
};

async function normalizeTokenResponse(response) {
  const payload = await response.clone().json();
  if (Array.isArray(payload.scope) && payload.scope.every((scope) => typeof scope === "string")) {
    return Response.json({ ...payload, scope: payload.scope.join(" ") }, { status: response.status });
  }
  return response;
}

export function createTwitch({ clientId, clientSecret, origin, fetcher = fetch }) {
  const client = { client_id: clientId };
  const authenticate = oauth.ClientSecretPost(clientSecret);
  const redirectUri = `${origin}/v1/auth/twitch/callback`;
  const options = () => ({ signal: AbortSignal.timeout(8000), [oauth.customFetch]: fetcher });

  async function validate(accessToken) {
    const response = await fetcher("https://id.twitch.tv/oauth2/validate", {
      headers: { Authorization: `OAuth ${accessToken}` }, signal: AbortSignal.timeout(8000), redirect: "error",
    });
    if (response.status === 401) throw new InvalidTwitchToken();
    if (!response.ok) throw new Error("Twitch validation unavailable");
    const identity = await response.json();
    if (identity.client_id !== clientId || !/^\d+$/.test(identity.user_id ?? "") || !(identity.expires_in > 0)) {
      throw new InvalidTwitchToken();
    }
    return identity;
  }

  return {
    authorizationUrl(state) {
      const url = new URL(server.authorization_endpoint);
      url.search = new URLSearchParams({ client_id: clientId, redirect_uri: redirectUri, response_type: "code", scope: "", state });
      return url.toString();
    },
    async exchange(callbackUrl, state) {
      const params = oauth.validateAuthResponse(server, client, new URL(callbackUrl), state);
      const response = await oauth.authorizationCodeGrantRequest(server, client, authenticate, params, redirectUri, oauth.nopkce, options());
      const tokens = await oauth.processAuthorizationCodeResponse(server, client, await normalizeTokenResponse(response));
      if (!tokens.refresh_token) throw new Error("Missing refresh token");
      const identity = await validate(tokens.access_token);
      const userResponse = await fetcher("https://api.twitch.tv/helix/users", {
        headers: { Authorization: `Bearer ${tokens.access_token}`, "Client-Id": clientId },
        signal: AbortSignal.timeout(8000), redirect: "error",
      });
      if (!userResponse.ok) throw new Error("Twitch profile unavailable");
      const user = (await userResponse.json()).data?.[0];
      if (!user || user.id !== identity.user_id || typeof user.login !== "string" ||
          typeof user.display_name !== "string" || typeof user.profile_image_url !== "string") {
        throw new InvalidTwitchToken();
      }
      return { user, tokens: { access_token: tokens.access_token, refresh_token: tokens.refresh_token,
        expires_at: Date.now() + identity.expires_in * 1000 } };
    },
    validate,
    async refresh(tokens) {
      try {
        const response = await oauth.refreshTokenGrantRequest(server, client, authenticate, tokens.refresh_token, options());
        if (response.status === 401 || (response.status === 400 &&
          (await response.clone().json()).message === "Invalid refresh token")) throw new InvalidTwitchToken();
        const next = await oauth.processRefreshTokenResponse(server, client, await normalizeTokenResponse(response));
        return { access_token: next.access_token, refresh_token: next.refresh_token ?? tokens.refresh_token,
          expires_at: Date.now() + (next.expires_in ?? 0) * 1000 };
      } catch (error) {
        if (error instanceof InvalidTwitchToken) throw error;
        if (error instanceof oauth.ResponseBodyError && ["invalid_grant", "invalid_token"].includes(error.error)) {
          throw new InvalidTwitchToken();
        }
        throw new Error("Twitch refresh unavailable");
      }
    },
    async revoke(accessToken) {
      const response = await fetcher("https://id.twitch.tv/oauth2/revoke", {
        method: "POST", body: new URLSearchParams({ client_id: clientId, token: accessToken }),
        signal: AbortSignal.timeout(8000), redirect: "error",
      });
      if (!response.ok) throw new Error("Twitch revocation unavailable");
    },
  };
}