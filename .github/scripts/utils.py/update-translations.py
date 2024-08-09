import time
import requests
import fnmatch
import logging
import subprocess
import os

from lxml import etree
from io import BytesIO
from crowdin_api import CrowdinClient
from zipfile import ZipFile
from typing import NewType

CrowdinBuild = NewType("CrowdinBuild", dict[str, dict[str, str]])  # file path mapped to string keys & translations

CROWDIN_PROJECT_ID = 51028
CROWDIN_API_TOKEN = os.environ["CROWDIN_PAT"]

crowdin_api = CrowdinClient(token=CROWDIN_API_TOKEN, project_id=CROWDIN_PROJECT_ID, page_size=500)

logging.basicConfig()
logger = logging.getLogger()
logger.setLevel(logging.INFO)


def generate_build() -> int:
    build_res = crowdin_api.translations.build_project_translation(dict({"skipUntranslatedStrings": True}))["data"]

    is_finished = build_res["status"] == "finished"
    while not is_finished:
        time.sleep(5.0)
        is_finished = crowdin_api.translations.check_project_build_status(build_res["id"])["data"]["status"] == "finished"

    return build_res["id"]


def download_build(build_id: int) -> CrowdinBuild:
    download_url = crowdin_api.translations.download_project_translations(build_id)["data"]["url"]
    build_download_req = requests.get(download_url)
    build: CrowdinBuild = dict()

    with ZipFile(BytesIO(build_download_req.content)) as build_zip:
        file_list = build_zip.namelist()
        file_list.sort()

        for file_name in file_list:
            if not file_name.endswith(".ini"):
                continue

            content = build_zip.read(file_name).decode()

            content = content.strip()
            if content == "":
                continue

            content = content.replace("\r\n", "\n")
            content = content.replace("\r", "\n")

            while "\n\n" in content:
                content = content.replace("\n\n", "\n")

            # Manually placed linebreaks in translations break the parsing
            fixed_linebreaks = ""
            previous_line = ""

            for line in content.splitlines():
                if '="' in line and not line.endswith('="'):
                    fixed_linebreaks += "\n"
                else:
                    logger.warning(f"{file_name} contains manually placed linebreak which should be fixed on Crowdin. Line: {previous_line}")
                fixed_linebreaks += line

                previous_line = line

            fixed_linebreaks = fixed_linebreaks.strip()

            # Parse to CrowdinBuild
            build[file_name] = {line[:line.index("=")]: line[line.index('"') + 1:-1] for line in fixed_linebreaks.splitlines()}

    return build


def get_locale_from_path(file_path: str) -> str:
    return file_path[file_path.rindex("/")+1:file_path.rindex(".")]


def filter_build(build: CrowdinBuild, patterns: list[str]) -> CrowdinBuild:
    filtered_build: CrowdinBuild = dict()

    for pattern in patterns:
        filtered_build = filtered_build | {key: build[key] for key in fnmatch.filter(list(build), pattern)}

    return filtered_build


def write_core_translations(build: CrowdinBuild) -> None:
    for file_path, translations in build.items():
        with open(file_path, "w") as file:
            output = ""
            for key, translation in translations.items():
                output += f'{key}="{translation}"\n'
            file.write(output)


def write_locale_file(build: CrowdinBuild) -> None:
    FILE_PATH = "UI/data/locale.ini"
    DEFAULT_LANGUAGE_NAME = "English (USA)"
    DEFAULT_LOCALE = "en-US"
    LANGUAGE_PROGRESS_THRESHOLD = 35  # Required percentage of language progress for a language to be added to the locale.ini

    locales_to_write: list[str] = []

    with open(FILE_PATH, encoding="utf-8") as file:
        for line in file:
            line = line.strip()
            if line.startswith("["):
                locales_to_write.append(line[1:-1])

    for progress_data in crowdin_api.translation_status.get_project_progress()["data"]:
        locale = progress_data["data"]["language"]["locale"]
        if not locale in locales_to_write:
            if progress_data["data"]["translationProgress"] >= LANGUAGE_PROGRESS_THRESHOLD:
                locales_to_write.append(locale)

    build[f"language-name/{DEFAULT_LOCALE}.ini"] = {"Language": DEFAULT_LANGUAGE_NAME}
    if not DEFAULT_LOCALE in locales_to_write:
        locales_to_write.append(DEFAULT_LOCALE)

    locales_to_write.sort()  # Sort again because we manually inserted en-US
    output = ""

    for locale in locales_to_write:
        file_path = f"language-name/{locale}.ini"
        if not file_path in build:
            logger.warning(f"Unable to add {locale} to locale.ini because it is missing the translation file.")
            continue

        translations = build[file_path]

        if not "Language" in translations:
            logger.warning(f"Unable to add {locale} to locale.ini because it is missing the language name.")
            continue

        output += f"[{locale}]\n"
        output += f'Name={translations["Language"]}\n'
        output += "\n"

    output = output[:-1]
    with open(FILE_PATH, "w", encoding="utf-8") as file:
        file.write(output)


def write_desktop_entry(build: CrowdinBuild) -> None:
    FILE_PATH = "UI/cmake/linux/com.obsproject.Studio.desktop"

    content = ""

    with open(FILE_PATH, encoding="utf-8") as file:
        # Remove previous translations
        content = file.read().split("\n\n")[0]

    content += "\n\n"

    for file_path, translations in build.items():
        locale = get_locale_from_path(file_path)

        for key, translation in translations.items():
            content += f'{key}[{locale}]={translation}\n'

    with open(FILE_PATH, "w", encoding="utf-8") as file:
        file.write(content)


def write_meta_info(build: CrowdinBuild) -> None:
    FILE_PATH = "UI/cmake/linux/com.obsproject.Studio.metainfo.xml.in"
    FILE_ID = 758  # id of the file probably called "Metadata"
    LANGUAGE_CODE_MAP = {"en_GB": "en-GB", "nb": "nb-NO", "pt_BR": "pt-BR"}  # Flathub and Crowdin don't share exactly the same language codes

    source_strings_data = crowdin_api.source_strings.list_strings(fileId=FILE_ID)["data"]
    xpath_list = [string["data"]["identifier"] for string in source_strings_data]

    read_file = ""
    with open(FILE_PATH, encoding="utf-8") as file:
        for line in file.read().splitlines():
            if ' xml:lang="' in line:
                continue

            read_file += line

    xml_tree = etree.ElementTree(etree.fromstring(read_file.encode()))

    for xpath in reversed(xpath_list):
        for file_path, translations in reversed(build.items()):
            if not xpath in translations:
                continue

            source_element = xml_tree.find(f".{xpath}")

            if source_element is None:
                logger.warning(f"Unable to find {xpath} from {file_path} in meta-info.")
                continue

            translated_element = etree.Element(source_element.tag)
            translated_element.text = translations[xpath]

            locale = get_locale_from_path(file_path)

            if locale in LANGUAGE_CODE_MAP:
                locale = LANGUAGE_CODE_MAP[locale]

            translated_element.set("xmllang", locale)
            source_element.addnext(translated_element)

    etree.indent(xml_tree)

    output = '<?xml version="1.0" encoding="UTF-8"?>\n'
    output += etree.tostring(xml_tree, encoding="utf-8").decode()
    output = output.replace(' xmllang="', ' xml:lang="')
    output += "\n"

    with open(FILE_PATH, "w", encoding="utf-8") as file:
        file.write(output)


def write_authors() -> None:
    FILE_PATH = "AUTHORS"
    EXCLUDED_COMMITERS = ["Translation Updater", "Service Checker"]

    output = "Original Author: Lain Bailey\n\nContributors are sorted by their amount of commits / translated words.\n\n"
    output += "Git Contributors:\n"

    git_output = subprocess.check_output(["git", "shortlog", "--all", "-sn", "--no-merges"]).decode()
    for line in git_output.splitlines():
        committer_name = line.split("\t")[1]
        if committer_name not in EXCLUDED_COMMITERS:
            output += f" {committer_name}\n"

    output += "\nTranslators:\n"

    languages_data = crowdin_api.projects.get_project()["data"]["targetLanguages"]
    target_languages = {language["name"]: language["id"] for language in languages_data}

    blocked_users_data = crowdin_api.users.list_project_members(role="blocked")["data"]
    blocked_users_ids = [user["data"]["id"] for user in blocked_users_data]

    for language_name in sorted(target_languages):
        generate_report_data = crowdin_api.reports.generate_top_members_report(format="json", languageId=target_languages[language_name])["data"]

        report_id = generate_report_data["identifier"]
        report_finished = generate_report_data["status"] == "finished"
        while not report_finished:
            time.sleep(1)
            report_finished = crowdin_api.reports.check_report_generation_status(report_id)["data"]["status"] == "finished"

        report_download_url = crowdin_api.reports.download_report(report_id)["data"]["url"]
        report_download_res = requests.get(report_download_url).json()

        if not "data" in report_download_res:
            continue

        output += f" {language_name}:\n"

        for user_data in report_download_res["data"]:
            user = user_data["user"]
            full_name = user["fullName"]

            if full_name == "REMOVED_USER":
                continue

            if user["id"] in blocked_users_ids:
                continue

            if user_data["translated"] == 0 and user_data["approved"] == 0:
                continue

            output += f"  {full_name}\n"

    with open(FILE_PATH, "w", encoding="utf-8") as file:
        file.write(output)


build_id = generate_build()
build = download_build(build_id)

write_core_translations(filter_build(build, ["UI/data/locale/*.ini",
                                             "plugins/*/data/locale/*.ini",
                                             "plugins/mac-virtualcam/src/obs-plugin/data/locale/*.ini",
                                             "UI/frontend-plugins/*/data/locale/*.ini"]))
write_locale_file(filter_build(build, ["language-name/*.ini"]))
write_desktop_entry(filter_build(build, ["desktop-entry/*.ini"]))
write_meta_info(filter_build(build, ["meta-info/*.ini"]))
write_authors()
