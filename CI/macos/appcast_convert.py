import os
from copy import deepcopy

import xmltodict


DELTA_BASE_URL = "https://cdn-fastly.obsproject.com/downloads/sparkle_deltas"


def convert_appcast(filename):
    print("Converting", filename)
    in_path = os.path.join("output/appcasts", filename)
    out_path = os.path.join("output/appcasts/stable", filename.replace("_v2", ""))
    with open(in_path, "rb") as f:
        xml_data = f.read()
    if not xml_data:
        return

    appcast = xmltodict.parse(xml_data, force_list=("item",))
    out_appcast = deepcopy(appcast)

    # Remove anything but stable channel items.
    new_list = []
    for _item in appcast["rss"]["channel"]["item"]:
        item = deepcopy(_item)
        branch = item.pop("sparkle:channel", "stable")
        if branch != "stable":
            continue
        # Remove delta information (incompatible with Sparkle 1.x)
        item.pop("sparkle:deltas", None)
        new_list.append(item)

    out_appcast["rss"]["channel"]["item"] = new_list

    with open(out_path, "wb") as f:
        xmltodict.unparse(out_appcast, output=f, pretty=True)

    # Also create legacy appcast from x86 version.
    if "x86" in filename:
        out_path = os.path.join("output/appcasts/stable", "updates.xml")
        with open(out_path, "wb") as f:
            xmltodict.unparse(out_appcast, output=f, pretty=True)


def adjust_appcast(filename):
    print("Adjusting", filename)
    file_path = os.path.join("output/appcasts", filename)
    with open(file_path, "rb") as f:
        xml_data = f.read()
    if not xml_data:
        return

    arch = "arm64" if "arm64" in filename else "x86_64"
    appcast = xmltodict.parse(xml_data, force_list=("item", "enclosure"))

    out_appcast = deepcopy(appcast)
    out_appcast["rss"]["channel"]["title"] = "OBS Studio"
    out_appcast["rss"]["channel"]["link"] = "https://obsproject.com/"

    new_list = []
    for _item in appcast["rss"]["channel"]["item"]:
        item = deepcopy(_item)
        # Fix changelog URL
        # Sparkle doesn't allow us to specify the URL for a specific update,
        # so we set the full release notes link instead and then rewrite the
        # appcast. Yay.
        if release_notes_link := item.pop("sparkle:fullReleaseNotesLink", None):
            item["sparkle:releaseNotesLink"] = release_notes_link

        # If deltas exist, update their URLs to match server layout
        # (generate_appcast doesn't allow this).
        if deltas := item.get("sparkle:deltas", None):
            for delta_item in deltas["enclosure"]:
                delta_filename = delta_item["@url"].rpartition("/")[2]
                delta_item["@url"] = f"{DELTA_BASE_URL}/{arch}/{delta_filename}"

        new_list.append(item)

    out_appcast["rss"]["channel"]["item"] = new_list

    with open(file_path, "wb") as f:
        xmltodict.unparse(out_appcast, output=f, pretty=True)


if __name__ == "__main__":
    for ac_file in os.listdir("output/appcasts"):
        if ".xml" not in ac_file:
            continue
        if "v2" not in ac_file:
            # generate_appcast may download legacy appcast files and update them as well.
            # Those generated files are not backwards-compatible, so delete whatever v1
            # files it may have created and recreate them manually.
            os.remove(os.path.join("output/appcasts", ac_file))
            continue
        adjust_appcast(ac_file)
        convert_appcast(ac_file)
