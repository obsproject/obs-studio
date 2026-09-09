import { createCipheriv, createDecipheriv, createHash, createHmac, randomBytes, timingSafeEqual } from "node:crypto";

export const randomToken = () => randomBytes(32).toString("base64url");
export const hash = (value) => createHash("sha256").update(value).digest("base64url");
export const validToken = (value) => typeof value === "string" && /^[A-Za-z0-9_-]{43}$/.test(value);
export const validVerifier = (value) => typeof value === "string" && /^[A-Za-z0-9._~-]{43,128}$/.test(value);

export function equal(left, right) {
  return typeof left === "string" && typeof right === "string" &&
    Buffer.byteLength(left) === Buffer.byteLength(right) &&
    timingSafeEqual(Buffer.from(left), Buffer.from(right));
}

export function createVault(encodedKey) {
  const key = Buffer.from(encodedKey, "base64");
  if (key.length !== 32) throw new Error("Invalid encryption key");
  return {
    rateKey(value) {
      return createHmac("sha256", key).update(value).digest("hex");
    },
    encrypt(value, subject) {
      const nonce = randomBytes(12);
      const cipher = createCipheriv("aes-256-gcm", key, nonce);
      cipher.setAAD(Buffer.from(`aerium:twitch:v1:${subject}`));
      const ciphertext = Buffer.concat([cipher.update(JSON.stringify(value), "utf8"), cipher.final()]);
      return ["v1", nonce.toString("base64url"), cipher.getAuthTag().toString("base64url"), ciphertext.toString("base64url")].join(".");
    },
    decrypt(value, subject) {
      const [version, nonce, tag, ciphertext, extra] = value.split(".");
      if (version !== "v1" || !ciphertext || extra) throw new Error("Invalid encrypted token");
      const decipher = createDecipheriv("aes-256-gcm", key, Buffer.from(nonce, "base64url"));
      decipher.setAAD(Buffer.from(`aerium:twitch:v1:${subject}`));
      decipher.setAuthTag(Buffer.from(tag, "base64url"));
      return JSON.parse(Buffer.concat([decipher.update(Buffer.from(ciphertext, "base64url")), decipher.final()]).toString("utf8"));
    },
  };
}