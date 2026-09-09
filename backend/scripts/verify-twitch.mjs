export async function verifyTwitch({ env, fetcher = fetch }) {
  let phase = "configuration";
  let token;
  try {
    const names = (env.AERIUM_VERIFY_TWITCH_USERS ?? "").split(",");
    if (!env.TWITCH_CLIENT_ID || !env.TWITCH_CLIENT_SECRET ||
        names.length !== 2 || new Set(names).size !== 2 || names.some((name) => !/^[a-z0-9_]{1,25}$/.test(name))) {
      throw new Error();
    }
    phase = "credential validation";
    const response = await fetcher("https://id.twitch.tv/oauth2/token", {
      method: "POST", body: new URLSearchParams({ client_id: env.TWITCH_CLIENT_ID,
        client_secret: env.TWITCH_CLIENT_SECRET, grant_type: "client_credentials" }),
      signal: AbortSignal.timeout(15000), redirect: "error",
    });
    if (!response.ok) throw new Error();
    token = (await response.json()).access_token;
    if (typeof token !== "string" || !token) throw new Error();
    phase = "user lookup";
    const query = new URLSearchParams();
    for (const name of names) query.append("login", name);
    const usersResponse = await fetcher(`https://api.twitch.tv/helix/users?${query}`, {
      headers: { "Client-Id": env.TWITCH_CLIENT_ID, Authorization: `Bearer ${token}` },
      signal: AbortSignal.timeout(15000), redirect: "error",
    });
    if (!usersResponse.ok) throw new Error();
    const users = (await usersResponse.json()).data;
    const testers = names.map((name) => {
      const user = users.find((item) => item.login === name);
      if (!user || !/^\d+$/.test(user.id)) throw new Error();
      return { login: name, id: user.id };
    });
    if (new Set(testers.map((user) => user.id)).size !== 2) throw new Error();
    phase = "allowlist validation";
    const enabled = env.AERIUM_LOGIN_ENABLED === "true";
    if (enabled && testers.map((user) => user.id).sort().join(",") !==
        (env.AERIUM_ALLOWED_TWITCH_IDS ?? "").split(",").map((id) => id.trim()).sort().join(",")) {
      throw new Error();
    }
    return { twitch_verified: true, login_enabled: enabled, testers };
  } catch {
    throw new Error(`Twitch deployment check failed during ${phase}; credential values withheld.`);
  } finally {
    if (typeof token === "string" && token) {
      try {
        const response = await fetcher("https://id.twitch.tv/oauth2/revoke", {
          method: "POST", body: new URLSearchParams({ client_id: env.TWITCH_CLIENT_ID, token }),
          signal: AbortSignal.timeout(15000), redirect: "error",
        });
        if (!response.ok) throw new Error();
      } catch {
        throw new Error("Twitch deployment check could not revoke its temporary token; credential values withheld.");
      }
    }
  }
}

if (import.meta.main && process.env.AERIUM_VERIFY_TWITCH_USERS) {
  try {
    if (process.env.VERCEL_ENV !== "production") throw new Error("Run the credential check in Vercel Production only.");
    console.log(JSON.stringify(await verifyTwitch({ env: process.env })));
  } catch (error) {
    console.error(error.message);
    process.exitCode = 1;
  }
}