import json
import socket
import ssl
import os
import time
import requests
import sys
import zipfile

from io import BytesIO
from random import randbytes
from urllib.parse import urlparse
from collections import defaultdict

MINIMUM_PURGE_AGE = 9.75 * 24 * 60 * 60  # slightly less than 10 days
TIMEOUT = 10
SKIPPED_SERVICES = {"YouNow", "SHOWROOM", "Dacast"}
SERVICES_FILE = "plugins/rtmp-services/data/services.json"
PACKAGE_FILE = "plugins/rtmp-services/data/package.json"
CACHE_FILE = "other/timestamps.json"
GITHUB_OUTPUT_FILE = os.environ.get("GITHUB_OUTPUT", None)

DO_NOT_PING = {"jp9000"}
PR_MESSAGE = """This is an automatically created pull request to remove unresponsive servers and services.

| Service | Action Taken | Author(s) |
| ------- | ------------ | --------- |
{table}

If you are not responsible for an affected service and want to be excluded from future pings please let us know.

Created by workflow run: https://github.com/{repository}/actions/runs/{run_id}"""

# GQL is great isn't it
GQL_QUERY = """{
  repositoryOwner(login: "obsproject") {
    repository(name: "obs-studio") {
      object(expression: "master") {
        ... on Commit {
          blame(path: "plugins/rtmp-services/data/services.json") {
            ranges {
              startingLine
              endingLine
              commit {
                author {
                  user {
                    login
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}"""

context = ssl.create_default_context()


def check_ftl_server(hostname) -> bool:
    """Check if hostname resolves to a valid address - FTL handshake not implemented"""
    try:
        socket.getaddrinfo(hostname, 8084, proto=socket.IPPROTO_UDP)
    except socket.gaierror as e:
        print(f"⚠️ Could not resolve hostname for server: {hostname} (Exception: {e})")
        return False
    else:
        return True


def check_hls_server(uri) -> bool:
    """Check if URL responds with status code < 500 and not 404, indicating that at least there's *something* there"""
    try:
        r = requests.post(uri, timeout=TIMEOUT)
        if r.status_code >= 500 or r.status_code == 404:
            raise Exception(f"Server responded with {r.status_code}")
    except Exception as e:
        print(f"⚠️ Could not connect to HLS server: {uri} (Exception: {e})")
        return False
    else:
        return True


def check_rtmp_server(uri) -> bool:
    """Try connecting and sending a RTMP handshake (with SSL if necessary)"""
    parsed = urlparse(uri)
    hostname, port = parsed.netloc.partition(":")[::2]

    if port:
        port = int(port)
    elif parsed.scheme == "rtmps":
        port = 443
    else:
        port = 1935

    try:
        recv = b""
        with socket.create_connection((hostname, port), timeout=TIMEOUT) as sock:
            # RTMP handshake is \x03 + 4 bytes time (can be 0) + 4 zero bytes + 1528 bytes random
            handshake = b"\x03\x00\x00\x00\x00\x00\x00\x00\x00" + randbytes(1528)
            if parsed.scheme == "rtmps":
                with context.wrap_socket(sock, server_hostname=hostname) as ssock:
                    ssock.sendall(handshake)
                    while True:
                        _tmp = ssock.recv(4096)
                        recv += _tmp
                        if len(recv) >= 1536 or not _tmp:
                            break
            else:
                sock.sendall(handshake)
                while True:
                    _tmp = sock.recv(4096)
                    recv += _tmp
                    if len(recv) >= 1536 or not _tmp:
                        break

        if len(recv) < 1536 or recv[0] != 3:
            raise ValueError("Invalid RTMP handshake received from server")
    except Exception as e:
        print(f"⚠️ Connection to server failed: {uri} (Exception: {e})")
        return False
    else:
        return True


def get_last_artifact():
    s = requests.session()
    s.headers["Authorization"] = f'Bearer {os.environ["GITHUB_TOKEN"]}'

    run_id = os.environ["WORKFLOW_RUN_ID"]
    repo = os.environ["REPOSITORY"]

    # fetch run first, get workflow id from there to get workflow runs
    r = s.get(f"https://api.github.com/repos/{repo}/actions/runs/{run_id}")
    r.raise_for_status()
    workflow_id = r.json()["workflow_id"]

    r = s.get(
        f"https://api.github.com/repos/{repo}/actions/workflows/{workflow_id}/runs",
        params=dict(
            per_page=1,
            status="completed",
            branch="master",
            conclusion="success",
            event="schedule",
        ),
    )
    r.raise_for_status()
    runs = r.json()
    if not runs["workflow_runs"]:
        raise ValueError("No completed workflow runs found")

    r = s.get(runs["workflow_runs"][0]["artifacts_url"])
    r.raise_for_status()

    for artifact in r.json()["artifacts"]:
        if artifact["name"] == "timestamps":
            artifact_url = artifact["archive_download_url"]
            break
    else:
        raise ValueError("No previous artifact found.")

    r = s.get(artifact_url)
    r.raise_for_status()
    zip_data = BytesIO()
    zip_data.write(r.content)

    with zipfile.ZipFile(zip_data) as zip_ref:
        for info in zip_ref.infolist():
            if info.filename == "timestamps.json":
                return json.loads(zip_ref.read(info.filename))


def find_people_to_blame(raw_services: str, servers: list[tuple[str, str]]) -> dict:
    if not servers:
        return dict()

    # Fetch Blame data from github
    s = requests.session()
    s.headers["Authorization"] = f'Bearer {os.environ["GITHUB_TOKEN"]}'

    r = s.post(
        "https://api.github.com/graphql", json=dict(query=GQL_QUERY, variables=dict())
    )
    r.raise_for_status()
    j = r.json()

    # The file is only ~2600 lines so this isn't too crazy and makes the lookup very easy
    line_author = dict()
    for blame in j["data"]["repositoryOwner"]["repository"]["object"]["blame"][
        "ranges"
    ]:
        for i in range(blame["startingLine"] - 1, blame["endingLine"]):
            if user := blame["commit"]["author"]["user"]:
                line_author[i] = user["login"]

    service_authors = defaultdict(set)
    for i, line in enumerate(raw_services.splitlines()):
        if '"url":' not in line:
            continue
        for server, service in servers:
            if server in line and (author := line_author.get(i)):
                if author not in DO_NOT_PING:
                    service_authors[service].add(author)

    return service_authors


def set_output(name, value):
    if not GITHUB_OUTPUT_FILE:
        return

    try:
        with open(GITHUB_OUTPUT_FILE, "a", encoding="utf-8", newline="\n") as f:
            f.write(f"{name}={value}\n")
    except Exception as e:
        print(f"Writing to github output files failed: {e!r}")


def main():
    try:
        with open(SERVICES_FILE, encoding="utf-8") as services_file:
            raw_services = services_file.read()
            services = json.loads(raw_services)
        with open(PACKAGE_FILE, encoding="utf-8") as package_file:
            package = json.load(package_file)
    except OSError as e:
        print(f"❌ Could not open services/package file: {e}")
        return 1

    # attempt to load last check result cache
    try:
        with open(CACHE_FILE, encoding="utf-8") as check_file:
            fail_timestamps = json.load(check_file)
    except OSError as e:
        # cache might be evicted or not exist yet, so this is non-fatal
        print(
            f"⚠️ Could not read cache file, trying to get last artifact (Exception: {e})"
        )

        try:
            fail_timestamps = get_last_artifact()
        except Exception as e:
            print(f"⚠️ Could not fetch cache file, starting fresh. (Exception: {e})")
            fail_timestamps = dict()
        else:
            print("Fetched cache file from last run artifact.")
    else:
        print("Successfully loaded cache file:", CACHE_FILE)

    start_time = int(time.time())
    affected_services = dict()
    removed_servers = list()

    # create temporary new list
    new_services = services.copy()
    new_services["services"] = []

    for service in services["services"]:
        # skip services that do custom stuff that we can't easily check
        if service["name"] in SKIPPED_SERVICES:
            new_services["services"].append(service)
            continue

        service_type = service.get("recommended", {}).get("output", "rtmp_output")
        if service_type not in {"rtmp_output", "ffmpeg_hls_muxer", "ftl_output"}:
            print("Unknown service type:", service_type)
            new_services["services"].append(service)
            continue

        # create a copy to mess with
        new_service = service.copy()
        new_service["servers"] = []

        # run checks for all the servers, and store results in timestamp cache
        for server in service["servers"]:
            if service_type == "ftl_output":
                is_ok = check_ftl_server(server["url"])
            elif service_type == "ffmpeg_hls_muxer":
                is_ok = check_hls_server(server["url"])
            else:  # rtmp
                is_ok = check_rtmp_server(server["url"])

            if not is_ok:
                if ts := fail_timestamps.get(server["url"], None):
                    if (delta := start_time - ts) >= MINIMUM_PURGE_AGE:
                        print(
                            f'🗑️ Purging server "{server["url"]}", it has been '
                            f"unresponsive for {round(delta/60/60/24)} days."
                        )
                        removed_servers.append((server["url"], service["name"]))
                        # continuing here means not adding it to the new list, thus dropping it
                        continue
                else:
                    fail_timestamps[server["url"]] = start_time
            elif is_ok and server["url"] in fail_timestamps:
                # remove timestamp of failed check if server is back
                delta = start_time - fail_timestamps[server["url"]]
                print(
                    f'💡 Server "{server["url"]}" is back after {round(delta/60/60/24)} days!'
                )
                del fail_timestamps[server["url"]]

            new_service["servers"].append(server)

        if (diff := len(service["servers"]) - len(new_service["servers"])) > 0:
            print(f'ℹ️ Removed {diff} server(s) from {service["name"]}')
            affected_services[service["name"]] = f"{diff} servers removed"

        # remove services with no valid servers
        if not new_service["servers"]:
            print(f'💀 Service "{service["name"]}" has no valid servers left, removing!')
            affected_services[service["name"]] = f"Service removed"
            continue

        new_services["services"].append(new_service)

    # write cache file
    try:
        os.makedirs("other", exist_ok=True)
        with open(CACHE_FILE, "w", encoding="utf-8") as cache_file:
            json.dump(fail_timestamps, cache_file)
    except OSError as e:
        print(f"❌ Could not write cache file: {e}")
        return 1
    else:
        print("Successfully wrote cache file:", CACHE_FILE)

    if removed_servers:
        # increment package version and save that as well
        package["version"] += 1
        package["files"][0]["version"] += 1

        try:
            with open(SERVICES_FILE, "w", encoding="utf-8") as services_file:
                json.dump(new_services, services_file, indent=4, ensure_ascii=False)
                services_file.write("\n")

            with open(PACKAGE_FILE, "w", encoding="utf-8") as package_file:
                json.dump(package, package_file, indent=4)
                package_file.write("\n")
        except OSError as e:
            print(f"❌ Could not write services/package file: {e}")
            return 1
        else:
            print(
                f"Successfully wrote services/package files:\n- {SERVICES_FILE}\n- {PACKAGE_FILE}"
            )

        # try to find authors to ping, this is optional and is allowed to fail
        try:
            service_authors = find_people_to_blame(raw_services, removed_servers)
        except Exception as e:
            print(f"⚠ Could not fetch blame for some reason: {e}")
            service_authors = dict()

        # set GitHub outputs
        set_output("make_pr", "true")
        msg = PR_MESSAGE.format(
            repository=os.environ["REPOSITORY"],
            run_id=os.environ["WORKFLOW_RUN_ID"],
            table="\n".join(
                "| {name} | {action} | {authors} |".format(
                    name=name.replace("|", "\\|"),
                    action=action,
                    authors=", ".join(
                        f"@{author}" for author in sorted(service_authors.get(name, []))
                    ),
                )
                for name, action in sorted(affected_services.items())
            ),
        )
        set_output("pr_message", json.dumps(msg))
    else:
        set_output("make_pr", "false")


if __name__ == "__main__":
    sys.exit(main())
