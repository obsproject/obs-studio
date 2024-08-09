import os
import subprocess
import logging

from crowdin_api import CrowdinClient
from fnmatch import fnmatch


CROWDIN_PROJECT_ID = 51028
CROWDIN_API_TOKEN = os.environ["CROWDIN_PAT"]
DEFAULT_LOCALE = "en-US"

crowdin_api = CrowdinClient(token=CROWDIN_API_TOKEN, project_id=CROWDIN_PROJECT_ID, page_size=500)

logging.basicConfig()
logger = logging.getLogger()
logger.setLevel(logging.INFO)


def get_updated_locales(cwd=".") -> list[str]:
    COMMITS_TO_CHECK = f"{os.environ['GITHUB_EVENT_BEFORE']}..{os.environ['GITHUB_SHA']}"
    updated_locales = subprocess.check_output(["git", "diff", "--name-only", COMMITS_TO_CHECK, f"**/{DEFAULT_LOCALE}.ini"], cwd=cwd)
    updated_locales = updated_locales.decode().strip().splitlines()

    return updated_locales


def get_dir_id(name: str) -> int:
    crowdin_api.source_files.list_directories(filter=name)["data"][0]["data"]["id"]


def add_to_crowdin_storage(file_path: str) -> int:
    return crowdin_api.storages.add_storage(open(file_path))["data"]["id"]


def update_crowdin_file(file_id: int, file_path: str) -> None:
    storage_id = add_to_crowdin_storage(file_path)
    crowdin_api.source_files.update_file(file_id, storage_id)

    logger.info(f"{file_path} updated on Crowdin.")


def create_crowdin_file(file_path: str, name: str, directory_name: str, export_pattern: str):
    storage_id = add_to_crowdin_storage(file_path)

    crowdin_api.source_files.add_file(storage_id,
                                      name,
                                      directoryId=get_dir_id(directory_name),
                                      exportOptions={"exportPattern": export_pattern})

    logger.info(f"{file_path} created on Crowdin in {directory_name} as {name}.")


def upload(updated_locales: list[str]) -> None:
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

    for file_path in updated_locales:
        if not os.path.exists(file_path):
            logger.warning(f"Unable to find {file_path} in working directory.")
            continue

        path_parts = file_path.split("/")

        if file_path in export_paths_map:
            crowdin_file_id = export_paths_map[file_path]
            update_crowdin_file(crowdin_file_id, file_path)
        elif fnmatch(file_path, f"plugins/*/data/locale/{DEFAULT_LOCALE}.ini"):
            create_crowdin_file(file_path, f"{path_parts[1]}.ini",
                                path_parts[0], "/plugins/%file_name%/data/locale/%locale%.ini")
        elif fnmatch(file_path, f"UI/frontend-plugins/*/data/locale/{DEFAULT_LOCALE}.ini"):
            create_crowdin_file(file_path, f"{path_parts[2]}.ini",
                                path_parts[1], "/UI/frontend-plugins/%file_name%/data/locale/%locale%.ini")
        else:
            logger.error(f"Unable to create {file_path} on Crowdin due to its unexpected location.")


updated_locales = [str]

if "SUBMODULE_NAME" in os.environ:
    updated_locales = get_updated_locales(f"plugins/{os.environ['SUBMODULE_NAME']}")
else:
    updated_locales = get_updated_locales()

if len(updated_locales) != 0:
    upload(updated_locales)
else:
    logger.error("Unable to find updated locales.")
