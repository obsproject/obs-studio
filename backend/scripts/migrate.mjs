import { readFile } from "node:fs/promises";
import { neon } from "@neondatabase/serverless";

if (!process.env.DATABASE_URL) throw new Error("DATABASE_URL is required");
try {
	const sql = neon(process.env.DATABASE_URL);
	const schema = await readFile(new URL("../src/schema.sql", import.meta.url), "utf8");
	await sql.query(schema);
	console.log("Aerium authentication schema applied.");
} catch {
	console.error("Schema migration failed. Check database connectivity and permissions.");
	process.exitCode = 1;
}