import argparse
import json
import logging
import os
import sys
from typing import Any

from json_source_map import calculate
from json_source_map.errors import InvalidInputError
from jsonschema import Draft7Validator


def discover_schema_file(filename: str) -> tuple[str | None, Any]:
    logger = logging.getLogger()

    with open(filename) as json_file:
        json_data = json.load(json_file)

    schema_filename = json_data.get("$schema", None)

    if not schema_filename:
        logger.info(f"ℹ️ ${filename} has no schema definition")
        return (None, None)

    schema_file = os.path.join(os.path.dirname(filename), schema_filename)

    with open(schema_file) as schema_file:
        schema_data = json.load(schema_file)

    return (str(schema_file), schema_data)


def validate_json_files(
    schema_data: dict[Any, Any], json_file_name: str
) -> list[dict[str, str]]:
    logger = logging.getLogger()

    with open(json_file_name) as json_file:
        text_data = json_file.read()

    json_data = json.loads(text_data)
    source_map = calculate(text_data)

    validator = Draft7Validator(schema_data)

    violations = []
    for violation in sorted(validator.iter_errors(json_data), key=str):
        logger.info(
            f"⚠️ Schema violation in file '{json_file_name}':\n{violation}\n----\n"
        )

        if len(violation.absolute_path):
            error_path = "/".join(
                str(path_element) for path_element in violation.absolute_path
            )
            error_entry = source_map["/{}".format(error_path)]

            violation_data = {
                "file": json_file_name,
                "title": "Validation Error",
                "message": violation.message,
                "annotation_level": "failure",
                "start_line": error_entry.value_start.line + 1,
                "end_line": error_entry.value_end.line + 1,
            }

            violations.append(violation_data)

    return violations


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate JSON files by schema definition"
    )
    parser.add_argument(
        "json_files", metavar="FILE", type=str, nargs="+", help="JSON file to validate"
    )
    parser.add_argument(
        "--loglevel", type=str, help="Set log level", default="WARNING", required=False
    )

    arguments = parser.parse_args()

    logging.basicConfig(level=arguments.loglevel, format="%(levelname)s - %(message)s")
    logger = logging.getLogger()

    schema_mappings = {}

    for json_file in arguments.json_files:
        try:
            (schema_file, schema_data) = discover_schema_file(json_file)
        except OSError as e:
            logger.error(f"❌ Failed to discover schema for file '{json_file}': {e}")
            return 2

        if schema_file and schema_file not in schema_mappings.keys():
            schema_mappings.update(
                {schema_file: {"schema_data": schema_data, "files": set()}}
            )

        schema_mappings[schema_file]["files"].add(json_file)

    validation_errors = []
    for schema_entry in schema_mappings.values():
        for json_file in schema_entry["files"]:
            try:
                new_errors = validate_json_files(schema_entry["schema_data"], json_file)
            except (InvalidInputError, OSError) as e:
                logger.error(
                    f"❌ Failed to create JSON source map for file '{json_file}': {e}"
                )
                return 2

            [validation_errors.append(error) for error in new_errors]

    if validation_errors:
        try:
            with open("validation_errors.json", "w") as results_file:
                json.dump(validation_errors, results_file)
        except OSError as e:
            logger.error(f"❌ Failed to write validation results file: {e}")
            return 2

        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
