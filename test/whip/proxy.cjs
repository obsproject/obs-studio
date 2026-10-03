// Loopback-only WHIP integration probe. Never records tokens, SDP, or addresses.
// Run alongside the OBS/browser matrix; point OBS at http://127.0.0.1:8101/.
const http = require('node:http');
const fs = require('node:fs');
const assert = require('node:assert/strict');
const sessions = new Map();
const events = [];
const output = process.env.WHIP_PROXY_REPORT;
assert.ok(output, 'Set WHIP_PROXY_REPORT to an artifact path');
function save() { fs.writeFileSync(output, JSON.stringify(events, null, 2)); }
const server = http.createServer(async (req, res) => {
 const event = {method:req.method, start:Date.now()};
 events.push(event);
 res.on('close', () => { event.clientAborted = !res.writableFinished; save(); });
 try {
  const chunks = [];
  for await (const chunk of req) chunks.push(chunk);
  const body = Buffer.concat(chunks).toString();
  event.candidates = (body.match(/^a=candidate:/gm) || []).length;
  event.endOfCandidates = body.includes('a=end-of-candidates');
  event.reverseTrickle = req.headers['x-whip-trickle-in'] === '1';
  const headers = {...req.headers};
  for (const name of ['host','connection','content-length']) delete headers[name];
  if (process.env.WHIP_PROXY_NO_REVERSE === '1') delete headers['x-whip-trickle-in'];
  if (req.method === 'POST' && process.env.WHIP_PROXY_POST_DELAY) {
   await new Promise(r => setTimeout(r, Number(process.env.WHIP_PROXY_POST_DELAY)));
  }
  if (req.method === 'PATCH' && process.env.WHIP_PROXY_PATCH_DELAY) {
   await new Promise(r => setTimeout(r, Number(process.env.WHIP_PROXY_PATCH_DELAY)));
  }
  const target = req.url === '/' ? 'https://whip.vdo.ninja/' : sessions.get(req.url);
  assert.ok(target, 'Unknown resource');
  const response = await fetch(target, {method:req.method, headers, body:body || undefined, signal:AbortSignal.timeout(15000)});
  event.status = response.status;
  const responseHeaders = Object.fromEntries(response.headers);
  for (const name of ['content-length','content-encoding','transfer-encoding','connection']) delete responseHeaders[name];
  if (response.status === 201) {
   const resource = '/session/' + sessions.size;
   sessions.set(resource, new URL(response.headers.get('location'), target).href);
   responseHeaders.location = 'http://127.0.0.1:8101' + resource;
  }
  if (req.method === 'OPTIONS' && process.env.WHIP_PROXY_NO_OPTIONS === '1') delete responseHeaders.link;
  if (req.method === 'OPTIONS' && process.env.WHIP_PROXY_STUN_ONLY === '1' && responseHeaders.link) {
   responseHeaders.link = responseHeaders.link.split(',').filter(link => /<stun:/.test(link)).join(',');
  }
  if (process.env.WHIP_PROXY_NO_ETAG === '1') delete responseHeaders.etag;
  let answer = await response.text();
  if (req.method === 'POST' && process.env.WHIP_PROXY_BAD_SDP === '1') {
   answer = 'v=0\r\nm=audio 9 UDP/TLS/RTP/SAVPF 111\r\na=rtpmap:bad opus/48000/2\r\n';
  }
  event.responseCandidates = (answer.match(/^a=candidate:/gm) || []).length;
  event.responseEndOfCandidates = answer.includes('a=end-of-candidates');
  event.duration = Date.now() - event.start;
  res.writeHead(response.status, responseHeaders);
  res.end(answer);
 } catch (e) {
  event.error = e.message;
  res.writeHead(502); res.end('Probe failed');
 } finally { save(); }
});
server.listen(8101, '127.0.0.1', () => console.log('WHIP probe ready'));
