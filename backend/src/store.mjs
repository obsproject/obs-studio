export function createStore(query) {
  const first = async (text, values = []) => (await query(text, values))[0];
  return {
    async ready() {
      await query("SELECT id FROM login_attempts LIMIT 0");
    },
    async rateLimit(key, limit, seconds) {
      const row = await first(`INSERT INTO rate_limits (key, bucket, hits, expires_at)
        VALUES ($1, floor(extract(epoch FROM now()) / $3), 1, now() + make_interval(secs => $3))
        ON CONFLICT (key, bucket) DO UPDATE SET hits = least(rate_limits.hits + 1, $2 + 1)
        RETURNING hits`, [key, limit, seconds]);
      return row.hits <= limit;
    },
    async cleanup() {
      await query("DELETE FROM login_attempts WHERE expires_at < now()");
      await query("DELETE FROM rate_limits WHERE expires_at < now()");
      await query("DELETE FROM sessions WHERE expires_at < now()");
    },
    async createLogin(id, challenge) {
      await query("INSERT INTO login_attempts (id, challenge) VALUES ($1, $2)", [id, challenge]);
    },
    beginLogin(id, stateHash) {
      return first(`UPDATE login_attempts SET state_hash = $2, status = 'authorizing'
        WHERE id = $1 AND status = 'waiting' AND expires_at > now() RETURNING id`, [id, stateHash]);
    },
    claimCallback(stateHash) {
      return first(`UPDATE login_attempts SET status = 'processing', state_hash = NULL
        WHERE state_hash = $1 AND status = 'authorizing' AND expires_at > now() RETURNING id`, [stateHash]);
    },
    async denyLogin(id) {
      await query("UPDATE login_attempts SET status = 'denied' WHERE id = $1", [id]);
    },
    completeLogin(id, user, tokenPayload) {
      return first(`WITH account AS (
        INSERT INTO accounts (twitch_id, login, display_name, avatar_url, token_payload)
        SELECT $2, $3, $4, $5, $6 WHERE EXISTS (
          SELECT 1 FROM login_attempts WHERE id = $1 AND status = 'processing' AND expires_at > now()
        ) ON CONFLICT (twitch_id) DO UPDATE SET
          login = EXCLUDED.login, display_name = EXCLUDED.display_name, avatar_url = EXCLUDED.avatar_url,
          token_payload = EXCLUDED.token_payload, token_version = gen_random_uuid(), validated_at = now(), lease_until = NULL
        RETURNING id
      ) UPDATE login_attempts SET status = 'complete', account_id = account.id
        FROM account WHERE login_attempts.id = $1 RETURNING account.id`,
      [id, user.id, user.login, user.display_name, user.profile_image_url, tokenPayload]);
    },
    loginStatus(id, challenge) {
      return first("SELECT status FROM login_attempts WHERE id = $1 AND challenge = $2 AND expires_at > now()", [id, challenge]);
    },
    exchangeLogin(id, challenge, accessHash, refreshHash) {
      return first(`WITH consumed AS (
        DELETE FROM login_attempts WHERE id = $1 AND challenge = $2 AND status = 'complete'
          AND expires_at > now() RETURNING account_id
      ) INSERT INTO sessions (account_id, access_hash, refresh_hash)
        SELECT account_id, $3, $4 FROM consumed RETURNING id`, [id, challenge, accessHash, refreshHash]);
    },
    accessSession(accessHash) {
      return first(`SELECT accounts.*, sessions.id AS session_id FROM sessions JOIN accounts ON accounts.id = sessions.account_id
        WHERE access_hash = $1 AND access_expires_at > now() AND expires_at > now()`, [accessHash]);
    },
    refreshSession(refreshHash) {
      return first(`SELECT accounts.*, sessions.id AS session_id FROM sessions JOIN accounts ON accounts.id = sessions.account_id
        WHERE refresh_hash = $1 AND expires_at > now()`, [refreshHash]);
    },
    rotateSession(refreshHash, accessHash, nextRefreshHash) {
      return first(`UPDATE sessions SET access_hash = $2, refresh_hash = $3, access_expires_at = now() + interval '15 minutes'
        WHERE refresh_hash = $1 AND expires_at > now() RETURNING expires_at`, [refreshHash, accessHash, nextRefreshHash]);
    },
    async logout(secretHash) {
      await query("DELETE FROM sessions WHERE access_hash = $1 OR refresh_hash = $1", [secretHash]);
    },
    acquireLease(id, version) {
      return first(`UPDATE accounts SET lease_until = now() + interval '90 seconds'
        WHERE id = $1 AND token_version = $2 AND (lease_until IS NULL OR lease_until < now()) RETURNING id`, [id, version]);
    },
    persistRefresh(id, version, payload) {
      return first("UPDATE accounts SET token_payload = $3 WHERE id = $1 AND token_version = $2 RETURNING id", [id, version, payload]);
    },
    saveTokens(id, version, payload) {
      return first(`UPDATE accounts SET token_payload = $3, validated_at = now(), lease_until = NULL,
        token_version = gen_random_uuid() WHERE id = $1 AND token_version = $2 RETURNING id`, [id, version, payload]);
    },
    async releaseLease(id, version) {
      await query("UPDATE accounts SET lease_until = NULL WHERE id = $1 AND token_version = $2", [id, version]);
    },
    async invalidateAccount(id, version) {
      await query(`WITH disconnected AS (
        UPDATE accounts SET token_payload = '', token_version = gen_random_uuid(), lease_until = NULL,
          validated_at = to_timestamp(0) WHERE id = $1 AND token_version = $2 RETURNING id
      ), cleared_logins AS (
        DELETE FROM login_attempts WHERE account_id IN (SELECT id FROM disconnected)
      ) DELETE FROM sessions WHERE account_id IN (SELECT id FROM disconnected)`, [id, version]);
    },
    async deleteAccount(id) {
      await query("DELETE FROM accounts WHERE id = $1", [id]);
    },
  };
}