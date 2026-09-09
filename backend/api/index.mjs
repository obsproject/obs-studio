import { createRuntime } from "../src/runtime.mjs";

let app;

export default async function handler(request, response) {
  try {
    const headers = new Headers();
    for (const [name, value] of Object.entries(request.headers)) {
      if (value !== undefined) headers.set(name, Array.isArray(value) ? value.join(", ") : value);
    }
    let body;
    if (!["GET", "HEAD"].includes(request.method)) {
      if (request.body !== undefined) body = typeof request.body === "string" ? request.body : JSON.stringify(request.body);
      else {
        const chunks = [];
        let size = 0;
        for await (const chunk of request) {
          size += Buffer.byteLength(chunk);
          if (size > 8192) break;
          chunks.push(chunk);
        }
        if (size > 8192) {
          response.writeHead(413, { "Cache-Control": "no-store" });
          response.end();
          return;
        }
        body = Buffer.concat(chunks).toString("utf8");
      }
      if (Buffer.byteLength(body) > 8192) {
        response.writeHead(413, { "Cache-Control": "no-store" });
        response.end();
        return;
      }
    }
    app ??= createRuntime();
    const result = await app(new Request(`https://api.aerium.tv${request.url}`, { method: request.method, headers, body }), {
      ip: process.env.VERCEL ? headers.get("x-vercel-forwarded-for") ?? "unknown" : "local",
    });
    response.statusCode = result.status;
    for (const [name, value] of result.headers) response.setHeader(name, value);
    response.end(await result.text());
  } catch {
    response.writeHead(503, { "Content-Type": "application/json", "Cache-Control": "no-store" });
    response.end(JSON.stringify({ error: "service_unavailable" }));
  }
}