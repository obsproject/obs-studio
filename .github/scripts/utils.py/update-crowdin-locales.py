import os
import logging
import json
import argparse
import sys

from crowdin_api import CrowdinClient
from fnmatch import fnmatch


def get_dir_id(name: str) -> int:
    crowdin_api.source_files.list_directories(filter=name)["data"][0]["data"]["id"]


def add_to_crowdin_storage(file_path: str) -> int:
    return crowdin_api.storages.add_storage(open(file_path))["data"]["id"]


def update_crowdin_file(file_id: int, file_path: str) -> None:
    logger = logging.getLogger()

    storage_id = add_to_crowdin_storage(file_path)
    crowdin_api.source_files.update_file(file_id, storage_id)

    logger.info(f"{file_path} updated on Crowdin.")


def create_crowdin_file(file_path: str, name: str, directory_name: str, export_pattern: str):
    logger = logging.getLogger()

    storage_id = add_to_crowdin_storage(file_path)

    crowdin_api.source_files.add_file(storage_id,
                                      name,
                                      directoryId=get_dir_id(directory_name),
                                      exportOptions={"exportPattern": export_pattern})

    logger.info(f"{file_path} created on Crowdin in {directory_name} as {name}.")


def upload(updated_locales: list[str]) -> None:
    DEFAULT_LOCALE = "en-US"

    logger = logging.getLogger()
    export_paths_map = dict()

    for source_file_data in crowdin_api.source_files.list_files()["data"]:
        source_file = source_file_data["data"]

        if "exportOptions" not in source_file:
            continue

        export_path: str = source_file["exportOptions"]["exportPattern"]
        export_path = export_path[1:]
        export_path = export_path.replace("%file_name%", os.path.basename(source_file["name"]).split(".")[0])
        export_path = export_path.replace("%locale%", DEFAULT_LOCALE)

        export_paths_map[export_path] = source_file["id"]

    for locale_path in updated_locales:
        if not os.path.exists(locale_path):
            logger.warning(f"Unable to find {locale_path} in working directory.")
            continue

        path_parts = locale_path.split("/")

        if locale_path in export_paths_map:
            crowdin_file_id = export_paths_map[locale_path]
            update_crowdin_file(crowdin_file_id, locale_path)
        elif fnmatch(locale_path, f"plugins/*/data/locale/{DEFAULT_LOCALE}.ini"):
            create_crowdin_file(locale_path, f"{path_parts[1]}.ini",
                                path_parts[0], "/plugins/%file_name%/data/locale/%locale%.ini")
        elif fnmatch(locale_path, f"UI/frontend-plugins/*/data/locale/{DEFAULT_LOCALE}.ini"):
            create_crowdin_file(locale_path, f"{path_parts[2]}.ini",
                                path_parts[1], "/UI/frontend-plugins/%file_name%/data/locale/%locale%.ini")
        else:
            logger.error(f"Unable to create {locale_path} on Crowdin due to its unexpected location.")


def main() -> int:
    parser = argparse.ArgumentParser(description="Update Crowdin source files based on provided list of updated locales")
    parser.add_argument("crowdin_secret", type=str, help="Crowdin API Token with manager access")
    parser.add_argument("updated_locales", type=str, help="JSON array of updated locales")
    parser.add_argument("--project_id", type=int, default=51028, required=False)
    parser.add_argument("--loglevel", type=str, default="INFO", required=False)
    arguments = parser.parse_args()

    logging.basicConfig(level=arguments.loglevel)
    logger = logging.getLogger()

    global crowdin_api
    crowdin_api = CrowdinClient(token=arguments.crowdin_secret, project_id=arguments.project_id, page_size=500)

    try:
        updated_locales = json.loads(arguments.updated_locales)
    except json.JSONDecodeError as e:
        logger.error(f"Failed to parse {e.doc}: {e}")
        return 1

    if len(updated_locales) != 0:
        upload(updated_locales)
    else:
        logger.error("List of updated locales is empty.")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
