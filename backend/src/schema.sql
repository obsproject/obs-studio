DO $migration$
BEGIN
CREATE TABLE IF NOT EXISTS accounts (
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    twitch_id text NOT NULL UNIQUE,
    login text NOT NULL,
    display_name text NOT NULL,
    avatar_url text NOT NULL,
    token_payload text NOT NULL,
    token_version uuid NOT NULL DEFAULT gen_random_uuid(),
    validated_at timestamptz NOT NULL DEFAULT now(),
    lease_until timestamptz,
    created_at timestamptz NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS login_attempts (
    id text PRIMARY KEY,
    challenge text NOT NULL,
    state_hash text UNIQUE,
    status text NOT NULL DEFAULT 'waiting' CHECK (status IN ('waiting', 'authorizing', 'processing', 'complete', 'denied')),
    account_id uuid REFERENCES accounts(id) ON DELETE CASCADE,
    expires_at timestamptz NOT NULL DEFAULT now() + interval '10 minutes'
);
CREATE INDEX IF NOT EXISTS login_attempts_expiry ON login_attempts(expires_at);

CREATE TABLE IF NOT EXISTS sessions (
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    account_id uuid NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    access_hash text NOT NULL UNIQUE,
    refresh_hash text NOT NULL UNIQUE,
    access_expires_at timestamptz NOT NULL DEFAULT now() + interval '15 minutes',
    expires_at timestamptz NOT NULL DEFAULT now() + interval '30 days',
    created_at timestamptz NOT NULL DEFAULT now()
);
CREATE INDEX IF NOT EXISTS sessions_account ON sessions(account_id);
CREATE INDEX IF NOT EXISTS sessions_expiry ON sessions(expires_at);

CREATE TABLE IF NOT EXISTS rate_limits (
    key text NOT NULL,
    bucket bigint NOT NULL,
    hits integer NOT NULL,
    expires_at timestamptz NOT NULL,
    PRIMARY KEY (key, bucket)
);
CREATE INDEX IF NOT EXISTS rate_limits_expiry ON rate_limits(expires_at);
END;
$migration$;