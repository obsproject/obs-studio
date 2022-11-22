import os
import sys
import plistlib
import glob
import subprocess
import argparse

import requests
import xmltodict


def download_build(url):
    print(f'Downloading build "{url}"...')
    filename = url.rpartition("/")[2]
    r = requests.get(url)
    if r.status_code == 200:
        with open(f"artifacts/{filename}", "wb") as f:
            f.write(r.content)
    else:
        print(f"Build download failed, status code: {r.status_code}")
        sys.exit(1)


def read_appcast(url):
    print(f"Downloading feed {url} ...")
    r = requests.get(url)
    if r.status_code != 200:
        print(f"Appcast download failed, status code: {r.status_code}")
        sys.exit(1)

    filename = url.rpartition("/")[2]
    with open(f"builds/{filename}", "wb") as f:
        f.write(r.content)

    appcast = xmltodict.parse(r.content, force_list=("item",))

    dl = 0
    for item in appcast["rss"]["channel"]["item"]:
        channel = item.get("sparkle:channel", "stable")
        if channel != target_branch:
            continue

        if dl == max_old_vers:
            break
        download_build(item["enclosure"]["@url"])
        dl += 1


def get_appcast_url(artifact_dir):
    dmgs = glob.glob(artifact_dir + "/*.dmg")
    if not dmgs:
        raise ValueError("No artifacts!")
    elif len(dmgs) > 1:
        raise ValueError("Too many artifacts!")

    dmg = dmgs[0]
    print(f"Mounting {dmg} ...")
    out = subprocess.check_output(
        [
            "hdiutil",
            "attach",
            "-readonly",
            "-noverify",
            "-noautoopen",
            "-plist",
            dmg,
        ]
    )
    d = plistlib.loads(out)

    mountpoint = ""
    for item in d["system-entities"]:
        if "mount-point" not in item:
            continue
        mountpoint = item["mount-point"]
        break

    url = None
    plist_files = glob.glob(mountpoint + "/*.app/Contents/Info.plist")
    if plist_files:
        plist_file = plist_files[0]
        print(f"Reading plist {plist_file} ...")
        plist = plistlib.load(open(plist_file, "rb"))
        url = plist.get("SUFeedURL", None)
    else:
        print("No Plist file found!")

    print(f"Unmounting {mountpoint}")
    subprocess.check_call(["hdiutil", "detach", mountpoint])
    return url


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--artifacts-dir",
        dest="artifacts",
        action="store",
        default="artifacts",
        help="Folder containing artifact",
    )
    parser.add_argument(
        "--branch",
        dest="branch",
        action="store",
        default="stable",
        help="Channel/Branch",
    )
    parser.add_argument(
        "--max-old-versions",
        dest="max_old_ver",
        action="store",
        type=int,
        default=1,
        help="Maximum old versions to download",
    )
    args = parser.parse_args()

    target_branch = args.branch
    max_old_vers = args.max_old_ver
    url = get_appcast_url(args.artifacts)
    if not url:
        raise ValueError("Failed to get Sparkle URL from DMG!")

    read_appcast(url)
