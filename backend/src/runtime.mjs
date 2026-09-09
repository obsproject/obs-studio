import { neon } from "@neondatabase/serverless";
import { createApp } from "./app.mjs";
import { createStore } from "./store.mjs";
import { createTwitch } from "./twitch.mjs";
import { createVault } from "./security.mjs";

export function createRuntime(env = process.env) {
  const origin = env.AERIUM_ORIGIN ?? "https://api.aerium.tv";
  if (new URL(origin).origin !== origin || !origin.startsWith("https://")) throw new Error("Invalid API origin");
  const sql = env.DATABASE_URL ? neon(env.DATABASE_URL) : null;
  const store = sql ? createStore((text, values) => sql.query(text, values, { fetchOptions: { signal: AbortSignal.timeout(8000) } })) : null;
  const vault = env.AERIUM_TOKEN_KEY ? createVault(env.AERIUM_TOKEN_KEY) : null;
  const twitch = env.TWITCH_CLIENT_ID && env.TWITCH_CLIENT_SECRET ? createTwitch({
    clientId: env.TWITCH_CLIENT_ID, clientSecret: env.TWITCH_CLIENT_SECRET, origin,
  }) : null;
  const allowedIds = (env.AERIUM_ALLOWED_TWITCH_IDS ?? "").split(",").map((value) => value.trim()).filter((value) => /^\d+$/.test(value));
  return createApp({ store, twitch, vault, origin, allowedIds, enabled: env.AERIUM_LOGIN_ENABLED === "true" });
}