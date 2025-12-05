# Adapted from https://github.com/BlankSpruce/gersemi/blob/2b5f154fb069e0b97f481f4ad0c282d52fcee89e/extension-example/as_module/gersemi_extension_example/__init__.py

from gersemi.builtin_commands import builtin_commands

target_sources = builtin_commands["target_sources"]
target_sources["keyword_preprocessors"] = {
    key: "sort+unique" for key in target_sources["multi_value_keywords"]
}

command_definitions = {
    "target_sources": target_sources,
}
