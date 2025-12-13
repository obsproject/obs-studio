import logging
logging.basicConfig(level=logging.INFO, format="%(asctime)s [generate_md.py] [%(levelname)s] %(message)s")
import os
import sys
import json

enumTypeOrder = [
    'WebSocketOpCode',
    'WebSocketCloseCode',
    'RequestBatchExecutionType',
    'RequestStatus',
    'EventSubscription',
    'ObsMediaInputAction',
    'ObsOutputState'
]

categoryOrder = [
    'General',
    'Config',
    'Sources',
    'Scenes',
    'Inputs',
    'Transitions',
    'Filters',
    'Scene Items',
    'Outputs',
    'Stream',
    'Record',
    'Media Inputs',
    'Ui',
    'High-Volume'
]

requestFieldHeader = """
**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
"""

responseFieldHeader = """
**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
"""

dataFieldHeader = """
**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
"""

fragments = []

# Utils
#######################################################################################################################

def read_file(fileName):
    with open(fileName, 'r') as f:
        return f.read()

def get_fragment(name, register = True):
    global fragments
    testFragmentName = name.replace(' ', '-').replace(':', '').lower()
    if testFragmentName in fragments:
        testFragmentName += '-1'
        increment = 1
        while testFragmentName in fragments:
            increment += 1
            testFragmentName[:-1] = str(increment)
    if register:
        fragments.append(testFragmentName)
    return testFragmentName

def get_category_items(items, category):
    ret = []
    for item in requests:
        if item['category'] != category:
            continue
        ret.append(item)
    return ret

def get_enums_toc(enums):
    ret = ''
    for enumType in enumTypeOrder:
        enum = None
        for enumIt in enums:
            if enumIt['enumType'] == enumType:
                enum = enumIt
                break
        if not enum:
            continue
        typeFragment = get_fragment(enumType, False)
        ret += '- [{}](#{})\n'.format(enumType, typeFragment)
        for enumIdentifier in enum['enumIdentifiers']:
            enumIdentifier = enumIdentifier['enumIdentifier']
            enumIdentifierHeader = '{}::{}'.format(enumType, enumIdentifier)
            enumIdentifierFragment = get_fragment(enumIdentifierHeader, False)
            ret += '  - [{}](#{})\n'.format(enumIdentifierHeader, enumIdentifierFragment)
    return ret

def get_enums(enums):
    ret = ''
    for enumType in enumTypeOrder:
        enum = None
        for enumIt in enums:
            if enumIt['enumType'] == enumType:
                enum = enumIt
                break
        if not enum:
            continue
        typeFragment = get_fragment(enumType)
        ret += '## {}\n\n'.format(enumType)
        for enumIdentifier in enum['enumIdentifiers']:
            enumIdentifierString = enumIdentifier['enumIdentifier']
            enumIdentifierHeader = '{}::{}'.format(enumType, enumIdentifierString)
            enumIdentifierFragment = get_fragment(enumIdentifierHeader, False)
            ret += '### {}\n\n'.format(enumIdentifierHeader)
            ret += '{}\n\n'.format(enumIdentifier['description'])
            ret += '- Identifier Value: `{}`\n'.format(enumIdentifier['enumValue'])
            ret += '- Latest Supported RPC Version: `{}`\n'.format(enumIdentifier['rpcVersion'])
            if enumIdentifier['deprecated']:
                ret += '- **⚠️ Deprecated. ⚠️**\n'
            if enumIdentifier['initialVersion'].lower() == 'unreleased':
                ret += '- Unreleased\n'
            else:
                ret += '- Added in v{}\n'.format(enumIdentifier['initialVersion'])
            if enumIdentifier != enum['enumIdentifiers'][-1]:
                ret += '\n---\n\n'
            else:
                ret += '\n'
    return ret

def get_requests_toc(requests):
    ret = ''
    for category in categoryOrder:
        requestsOut = []
        for request in requests:
            if request['category'] != category.lower():
                continue
            requestsOut.append(request)
        if not len(requestsOut):
            continue
        categoryFragment = get_fragment(category, False)
        ret += '- [{} Requests](#{}-requests)\n'.format(category, categoryFragment)
        for request in requestsOut:
            requestType = request['requestType']
            requestTypeFragment = get_fragment(requestType, False)
            ret += '  - [{}](#{})\n'.format(requestType, requestTypeFragment)
    return ret

def get_requests(requests):
    ret = ''
    for category in categoryOrder:
        requestsOut = []
        for request in requests:
            if request['category'] != category.lower():
                continue
            requestsOut.append(request)
        if not len(requestsOut):
            continue
        categoryFragment = get_fragment(category)
        ret += '## {} Requests\n\n'.format(category)
        for request in requestsOut:
            requestType = request['requestType']
            requestTypeFragment = get_fragment(requestType)
            ret += '### {}\n\n'.format(requestType)
            ret += '{}\n\n'.format(request['description'])
            ret += '- Complexity Rating: `{}/5`\n'.format(request['complexity'])
            ret += '- Latest Supported RPC Version: `{}`\n'.format(request['rpcVersion'])
            if request['deprecated']:
                ret += '- **⚠️ Deprecated. ⚠️**\n'
            if request['initialVersion'].lower() == 'unreleased':
                ret += '- Unreleased\n'
            else:
                ret += '- Added in v{}\n'.format(request['initialVersion'])

            if request['requestFields']:
                ret += requestFieldHeader
            for requestField in request['requestFields']:
                valueType = requestField['valueType'].replace('<', "&lt;").replace('>', "&gt;")
                valueRestrictions = requestField['valueRestrictions'] if requestField['valueRestrictions'] else 'None'
                valueOptional = '?' if requestField['valueOptional'] else ''
                valueOptionalBehavior = requestField['valueOptionalBehavior'] if requestField['valueOptional'] and requestField['valueOptionalBehavior'] else 'N/A'
                ret += '| {}{} | {} | {} | {} | {} |\n'.format(valueOptional, requestField['valueName'], valueType, requestField['valueDescription'], valueRestrictions, valueOptionalBehavior)

            if request['responseFields']:
                ret += responseFieldHeader
            for responseField in request['responseFields']:
                valueType = responseField['valueType'].replace('<', "&lt;").replace('>', "&gt;")
                ret += '| {} | {} | {} |\n'.format(responseField['valueName'], valueType, responseField['valueDescription'])

            if request != requestsOut[-1]:
                ret += '\n---\n\n'
            else:
                ret += '\n'
    return ret

def get_events_toc(events):
    ret = ''
    for category in categoryOrder:
        eventsOut = []
        for event in events:
            if event['category'] != category.lower():
                continue
            eventsOut.append(event)
        if not len(eventsOut):
            continue
        categoryFragment = get_fragment(category, False)
        ret += '- [{} Events](#{}-events)\n'.format(category, categoryFragment)
        for event in eventsOut:
            eventType = event['eventType']
            eventTypeFragment = get_fragment(eventType, False)
            ret += '  - [{}](#{})\n'.format(eventType, eventTypeFragment)
    return ret

def get_events(events):
    ret = ''
    for category in categoryOrder:
        eventsOut = []
        for event in events:
            if event['category'] != category.lower():
                continue
            eventsOut.append(event)
        if not len(eventsOut):
            continue
        categoryFragment = get_fragment(category)
        ret += '## {} Events\n\n'.format(category)
        for event in eventsOut:
            eventType = event['eventType']
            eventTypeFragment = get_fragment(eventType)
            ret += '### {}\n\n'.format(eventType)
            ret += '{}\n\n'.format(event['description'])
            ret += '- Complexity Rating: `{}/5`\n'.format(event['complexity'])
            ret += '- Latest Supported RPC Version: `{}`\n'.format(event['rpcVersion'])
            if event['deprecated']:
                ret += '- **⚠️ Deprecated. ⚠️**\n'
            if event['initialVersion'].lower() == 'unreleased':
                ret += '- Unreleased\n'
            else:
                ret += '- Added in v{}\n'.format(event['initialVersion'])

            if event['dataFields']:
                ret += dataFieldHeader
            for dataField in event['dataFields']:
                valueType = dataField['valueType'].replace('<', "&lt;").replace('>', "&gt;")
                ret += '| {} | {} | {} |\n'.format(dataField['valueName'], valueType, dataField['valueDescription'])

            if event != eventsOut[-1]:
                ret += '\n---\n\n'
            else:
                ret += '\n'
    return ret

# Actual code
#######################################################################################################################

# Read versions json
try:
    with open('../versions.json', 'r') as f:
        versions = json.load(f)
except IOError:
    logging.error('Failed to get global versions. Versions file not configured?')
    os.exit(1)

# Read protocol json
with open('../generated/protocol.json', 'r') as f:
    protocol = json.load(f)

output = "<!-- This file was automatically generated. Do not edit directly! -->\n"
output += "<!-- markdownlint-disable no-bare-urls -->\n"

# Insert introduction partial
output += read_file('partials/introduction.md')
logging.info('Inserted introduction section.')

output += '\n'

# Generate enums MD
output += read_file('partials/enumsHeader.md')
output += '\n'
output += get_enums_toc(protocol['enums'])
output += '\n'
output += get_enums(protocol['enums'])
logging.info('Inserted enums section.')

# Generate events MD
output += read_file('partials/eventsHeader.md')
output += '\n'
output += get_events_toc(protocol['events'])
output += '\n'
output += get_events(protocol['events'])
logging.info('Inserted events section.')

# Generate requests MD
output += read_file('partials/requestsHeader.md')
output += '\n'
output += get_requests_toc(protocol['requests'])
output += '\n'
output += get_requests(protocol['requests'])
logging.info('Inserted requests section.')

if output.endswith('\n\n'):
    output = output[:-1]

# Write new protocol MD
with open('../generated/protocol.md', 'w') as f:
    f.write(output)

logging.info('Finished generating protocol.md.')
