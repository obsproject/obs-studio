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

MINIMUM_PURGE_AGE = 9.75 * 24 * 60 * 60  # slightly less than 10 days
TIMEOUT = 10
SKIPPED_SERVICES = {'YouNow', 'SHOWROOM', 'Dacast'}
SERVICES_FILE = 'plugins/rtmp-services/data/services.json'
PACKAGE_FILE = 'plugins/rtmp-services/data/package.json'
CACHE_FILE = 'other/timestamps.json'

context = ssl.create_default_context()


def check_ftl_server(hostname) -> bool:
    """Check if hostname resolves to a valid address - FTL handshake not implemented"""
    try:
        socket.getaddrinfo(hostname, 8084, proto=socket.IPPROTO_UDP)
    except socket.gaierror as e:
        print(f'⚠️ Could not resolve hostname for server: {hostname} (Exception: {e})')
        return False
    else:
        return True


def check_hls_server(uri) -> bool:
    """Check if URL responds with status code < 500 and not 404, indicating that at least there's *something* there"""
    try:
        r = requests.post(uri, timeout=TIMEOUT)
        if r.status_code >= 500 or r.status_code == 404:
            raise Exception(f'Server responded with {r.status_code}')
    except Exception as e:
        print(f'⚠️ Could not connect to HLS server: {uri} (Exception: {e})')
        return False
    else:
        return True


def check_rtmp_server(uri) -> bool:
    """Try connecting and sending a RTMP handshake (with SSL if necessary)"""
    parsed = urlparse(uri)
    hostname, port = parsed.netloc.partition(':')[::2]
    port = int(port) if port else 1935

    try:
        recv = b''
        with socket.create_connection((hostname, port), timeout=TIMEOUT) as sock:
            # RTMP handshake is \x03 + 4 bytes time (can be 0) + 4 zero bytes + 1528 bytes random
            handshake = b'\x03\x00\x00\x00\x00\x00\x00\x00\x00' + randbytes(1528)
            if parsed.scheme == 'rtmps':
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
            raise ValueError('Invalid RTMP handshake received from server')
    except Exception as e:
        print(f'⚠️ Connection to server failed: {uri} (Exception: {e})')
        return False
    else:
        return True


def get_last_artifact():
    s = requests.session()
    s.headers['Authorization'] = f'Bearer {os.environ["GITHUB_TOKEN"]}'

    run_id = os.environ['WORKFLOW_RUN_ID']
    repo = os.environ['REPOSITORY']

    # fetch run first, get workflow id from there to get workflow runs
    r = s.get(f'https://api.github.com/repos/{repo}/actions/runs/{run_id}')
    r.raise_for_status()
    workflow_id = r.json()['workflow_id']

    r = s.get(
        f'https://api.github.com/repos/{repo}/actions/workflows/{workflow_id}/runs',
        params=dict(per_page=1, status='completed', branch='master', conclusion='success', event='schedule'),
    )
    r.raise_for_status()
    runs = r.json()
    if not runs['workflow_runs']:
        raise ValueError('No completed workflow runs found')

    r = s.get(runs['workflow_runs'][0]['artifacts_url'])
    r.raise_for_status()

    for artifact in r.json()['artifacts']:
        if artifact['name'] == 'timestamps':
            artifact_url = artifact['archive_download_url']
            break
    else:
        raise ValueError('No previous artifact found.')

    r = s.get(artifact_url)
    r.raise_for_status()
    zip_data = BytesIO()
    zip_data.write(r.content)

    with zipfile.ZipFile(zip_data) as zip_ref:
        for info in zip_ref.infolist():
            if info.filename == 'timestamps.json':
                return json.loads(zip_ref.read(info.filename))


def main():
    try:
        with open(SERVICES_FILE, encoding='utf-8') as services_file:
            services = json.load(services_file)
        with open(PACKAGE_FILE, encoding='utf-8') as package_file:
            package = json.load(package_file)
    except OSError as e:
        print(f'❌ Could not open services/package file: {e}')
        return 1

    # attempt to load last check result cache
    try:
        with open(CACHE_FILE, encoding='utf-8') as check_file:
            fail_timestamps = json.load(check_file)
    except OSError as e:
        # cache might be evicted or not exist yet, so this is non-fatal
        print(f'⚠️ Could not read cache file, trying to get last artifact (Exception: {e})')

        try:
            fail_timestamps = get_last_artifact()
        except Exception as e:
            print(f'⚠️ Could not fetch cache file, starting fresh. (Exception: {e})')
            fail_timestamps = dict()
        else:
            print('Fetched cache file from last run artifact.')
    else:
        print('Successfully loaded cache file:', CACHE_FILE)

    start_time = int(time.time())
    removed_something = False

    # create temporary new list
    new_services = services.copy()
    new_services['services'] = []

    for service in services['services']:
        # skip services that do custom stuff that we can't easily check
        if service['name'] in SKIPPED_SERVICES:
            new_services['services'].append(service)
            continue

        service_type = service.get('recommended', {}).get('output', 'rtmp_output')
        if service_type not in {'rtmp_output', 'ffmpeg_hls_muxer', 'ftl_output'}:
            print('Unknown service type:', service_type)
            new_services['services'].append(service)
            continue

        # create a copy to mess with
        new_service = service.copy()
        new_service['servers'] = []

        # run checks for all the servers, and store results in timestamp cache
        for server in service['servers']:
            if service_type == 'ftl_output':
                is_ok = check_ftl_server(server['url'])
            elif service_type == 'ffmpeg_hls_muxer':
                is_ok = check_hls_server(server['url'])
            else:  # rtmp
                is_ok = check_rtmp_server(server['url'])

            if not is_ok:
                if ts := fail_timestamps.get(server['url'], None):
                    if (delta := start_time - ts) >= MINIMUM_PURGE_AGE:
                        print(
                            f'🗑️ Purging server "{server["url"]}", it has been '
                            f'unresponsive for {round(delta/60/60/24)} days.'
                        )
                        # continuing here means not adding it to the new list, thus dropping it
                        continue
                else:
                    fail_timestamps[server['url']] = start_time
            elif is_ok and server['url'] in fail_timestamps:
                # remove timestamp of failed check if server is back
                delta = start_time - fail_timestamps[server['url']]
                print(f'💡 Server "{server["url"]}" is back after {round(delta/60/60/24)} days!')
                del fail_timestamps[server['url']]

            new_service['servers'].append(server)

        if (diff := len(service['servers']) - len(new_service['servers'])) > 0:
            print(f'ℹ️ Removed {diff} server(s) from {service["name"]}')
            removed_something = True

        # remove services with no valid servers
        if not new_service['servers']:
            print(f'💀 Service "{service["name"]}" has no valid servers left, removing!')
            continue

        new_services['services'].append(new_service)

    # write cache file
    try:
        os.makedirs('other', exist_ok=True)
        with open(CACHE_FILE, 'w', encoding='utf-8') as cache_file:
            json.dump(fail_timestamps, cache_file)
    except OSError as e:
        print(f'❌ Could not write cache file: {e}')
        return 1
    else:
        print('Successfully wrote cache file:', CACHE_FILE)

    if removed_something:
        # increment package version and save that as well
        package['version'] += 1
        package['files'][0]['version'] += 1

        try:
            with open(SERVICES_FILE, 'w', encoding='utf-8') as services_file:
                json.dump(new_services, services_file, indent=4, ensure_ascii=False)
                services_file.write('\n')

            with open(PACKAGE_FILE, 'w', encoding='utf-8') as package_file:
                json.dump(package, package_file, indent=4)
                package_file.write('\n')
        except OSError as e:
            print(f'❌ Could not write services/package file: {e}')
            return 1
        else:
            print(f'Successfully wrote services/package files:\n- {SERVICES_FILE}\n- {PACKAGE_FILE}')


if __name__ == '__main__':
    sys.exit(main())
