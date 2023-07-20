import argparse
import glob
import json
import logging
import os
import sys
from typing import Any


def main() -> int:
    parser = argparse.ArgumentParser(description="Format Flatpak manifest")
    parser.add_argument(
        "manifest_file",
        metavar="FILE",
        type=str,
        help="Manifest file to adjust format for",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Check for necessary changes only",
        default=False,
        required=False,
    )
    parser.add_argument(
        "--loglevel", type=str, help="Set log level", default="WARNING", required=False
    )

    arguments = parser.parse_args()

    logging.basicConfig(level=arguments.loglevel, format="%(message)s")
    logger = logging.getLogger()

    manifest_file = arguments.manifest_file

    try:
        with open(manifest_file, "r+") as manifest:
            manifest_path = os.path.dirname(manifest_file)
            manifest_string = manifest.read()
            manifest_data = json.loads(manifest_string)

            module_list = manifest_data.get("modules", [])

            obs_object = module_list[-1]

            if type(obs_object) != dict:
                logger.error(
                    f"❌ Last element in modules list is not the obs-studio object"
                )
                return 2

            new_module_list = []

            for module in module_list:
                if type(module) == str:
                    if not os.path.isfile(os.path.join(manifest_path, module)):
                        logger.warning(
                            f"⚠️ Specified module {os.path.basename(module)} not found."
                        )
                        continue

                    new_module_list.append(module)

            new_module_list.sort()
            new_module_list.append(obs_object)
            manifest_data["modules"] = new_module_list

            new_manifest_string = (
                f"{json.dumps(manifest_data, indent=4, ensure_ascii=False)}\n"
            )

            if arguments.check:
                if new_module_list != module_list:
                    logger.error(f"❌ Module list failed order validation")
                    return 2
                elif new_manifest_string != manifest_string:
                    logger.error(f"❌ Manifest file is not correctly formatted")
                    return 2
                else:
                    logger.info(f"✅ Module list passed order validation")
                    return 0

            manifest.seek(0)
            manifest.truncate()
            manifest.write(new_manifest_string)

            logger.info(f"✅ Updated manifest file '{manifest_file}")
    except IOError:
        logger.error(f"❌ Unable to read manifest file '{manifest_file}'")
        return 2

    return 0


if __name__ == "__main__":
    sys.exit(main())
