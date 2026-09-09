import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import { randomBytes } from "node:crypto";
import { after, test } from "node:test";
import { PGlite } from "@electric-sql/pglite";
import { createStore } from "../src/store.mjs";
import { createVault, hash, randomToken } from "../src/security.mjs";

const database = new PGlite();
await database.exec(await readFile(new URL("../src/schema.sql", import.meta.url), "utf8"));
const store = createStore(async (text, values) => (await database.query(text, values)).rows);
after(() => database.close());

test("tokens are encrypted, authenticated, and bound to the Twitch identity", () => {
  const vault = createVault(randomBytes(32).toString("base64"));
  const encrypted = vault.encrypt({ refresh_token: "secret" }, "123");
  assert(!encrypted.includes("secret"));
  assert.equal(vault.decrypt(encrypted, "123").refresh_token, "secret");
  assert.throws(() => vault.decrypt(encrypted, "456"));
  assert.throws(() => vault.decrypt(encrypted.slice(0, -5), "123"));
});

test("login, callbacks, handoff and session refresh are single-use", async () => {
  const id = randomToken();
  const challenge = hash(randomToken());
  await store.createLogin(id, challenge);
  assert(await store.beginLogin(id, "state-hash"));
  assert.equal(await store.beginLogin(id, "other-state"), undefined);
  const claims = await Promise.all([store.claimCallback("state-hash"), store.claimCallback("state-hash")]);
  assert.equal(claims.filter(Boolean).length, 1);
  const account = await store.completeLogin(id, { id: "123", login: "tester", display_name: "Tester", profile_image_url: "" }, "encrypted");
  assert(account.id);
  assert.equal(await store.exchangeLogin(id, "wrong-challenge", "access", "refresh"), undefined);
  const sessions = await Promise.all([
    store.exchangeLogin(id, challenge, "access", "refresh"),
    store.exchangeLogin(id, challenge, "access2", "refresh2"),
  ]);
  assert.equal(sessions.filter(Boolean).length, 1);
  const active = await store.accessSession("access");
  assert.equal(active.twitch_id, "123");
  assert(await store.acquireLease(active.id, active.token_version));
  assert.equal(await store.acquireLease(active.id, active.token_version), undefined);
  await store.releaseLease(active.id, active.token_version);
  const rotations = await Promise.all([
    store.rotateSession("refresh", "new-access", "new-refresh"),
    store.rotateSession("refresh", "duplicate-access", "duplicate-refresh"),
  ]);
  assert.equal(rotations.filter(Boolean).length, 1);
  assert.equal(await store.accessSession("access"), undefined);
  assert.equal(await store.refreshSession("refresh"), undefined);
  assert(await store.accessSession("new-access"));
  await store.logout("new-refresh");
  assert.equal(await store.accessSession("new-access"), undefined);
});

test("expired attempts cannot authorize and rate limits are shared and bounded", async () => {
  const id = randomToken();
  await store.createLogin(id, "challenge");
  await database.query("UPDATE login_attempts SET expires_at = now() - interval '1 second' WHERE id = $1", [id]);
  assert.equal(await store.beginLogin(id, "expired-state"), undefined);
  assert.equal(await store.rateLimit("ip", 2, 600), true);
  assert.equal(await store.rateLimit("ip", 2, 600), true);
  assert.equal(await store.rateLimit("ip", 2, 600), false);
  await store.cleanup();
  assert.equal((await database.query("SELECT id FROM login_attempts WHERE id = $1", [id])).rows.length, 0);
});