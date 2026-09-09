import assert from "node:assert/strict";
import { createServer } from "node:http";
import { test } from "node:test";
import handler from "../api/index.mjs";

test("Node HTTP adapter serves health, rejects oversized bodies, and hides source paths", async () => {
  const server = createServer(handler);
  await new Promise((resolve) => server.listen(0, "127.0.0.1", resolve));
  const origin = `http://127.0.0.1:${server.address().port}`;
  try {
    const health = await fetch(`${origin}/health`);
    assert.equal(health.status, 200);
    assert.equal((await health.json()).service, "aerium-api");
    assert.equal((await fetch(`${origin}/src/runtime.mjs`)).status, 404);
    assert.equal((await fetch(`${origin}/v1/auth/desktop`, { method: "POST", body: "x".repeat(9000) })).status, 413);
  } finally {
    await new Promise((resolve) => server.close(resolve));
  }
});