import argparse
import glob
import json
import logging
import os
import sys
from typing import Any


class LogFilter(logging.Filter):
    def filter(self, record):
        if record.levelno == logging.ERROR or record.levelno == logging.CRITICAL:
            record.github_level = "::error::"
            record.obs_level = "✖"
        elif record.levelno == logging.WARNING:
            record.github_level = "::warning::"
            record.obs_level = "⚠"
        elif record.levelno == logging.INFO:
            record.github_level = "::notice::"
            record.obs_level = "ℹ︎"
        elif record.levelno == logging.DEBUG:
            record.github_level = "::debug::"
            record.obs_level = "⚙︎"
        else:
            record.github_level = ""

        return True


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

    ENV_CI = os.environ.get("CI", None)

    if ENV_CI is not None:
        logging.basicConfig(
            level=arguments.loglevel, format="%(github_level)s%(message)s"
        )
    else:
        logging.basicConfig(
            level=arguments.loglevel, format="%(obs_level)s %(message)s"
        )

    log_filter = LogFilter()
    logger = logging.getLogger()
    logger.addFilter(log_filter)

    manifest_file = arguments.manifest_file

    try:
        with open(manifest_file, "r+") as manifest:
            manifest_path = os.path.dirname(manifest_file)
            manifest_string = manifest.read()
            manifest_data = json.loads(manifest_string)

            new_manifest_string = (
                f"{json.dumps(manifest_data, indent=4, ensure_ascii=False)}\n"
            )

            if arguments.check:
                if new_manifest_string != manifest_string:
                    logger.error(f"Manifest file is not correctly formatted")
                    return 2
                else:
                    logger.info(f"Module list passed order validation")
                    return 0

            manifest.seek(0)
            manifest.truncate()
            manifest.write(new_manifest_string)

            logger.info(f"Updated manifest file '{manifest_file}")
    except IOError:
        logger.error(f"Unable to read manifest file '{manifest_file}'")
        return 2

    return 0


if __name__ == "__main__":
    sys.exit(main())
