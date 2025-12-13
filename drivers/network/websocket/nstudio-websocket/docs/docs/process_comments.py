import logging
logging.basicConfig(level=logging.INFO, format="%(asctime)s [process_comments.py] [%(levelname)s] %(message)s")
import os
import sys
import json

# The comments parser will return a string type instead of an array if there is only one field
def field_to_array(field):
    if type(field) == str:
        return [field]
    return field

# This raw JSON can be really damn unpredictable. Let's handle that
def field_to_string(field):
    if type(field) == list:
        return field_to_string(field[0])
    elif type(field) == dict:
        return field_to_string(field['description'])
    return str(field)

# Make sure that everything we expect is there
def validate_fields(data, fields):
    for field in fields:
        if field not in data:
            logging.warning('Missing required item: {}'.format(field))
            return False
    return True

# Get the individual components of a `requestField` or `responseField` or `dataField` entry
def get_components(data):
    ret = []
    components_raw = data.split('|')
    for component in components_raw:
        ret.append(component.strip())
    return ret

# Convert all request fields from raw to final
def get_request_fields(fields):
    fields = field_to_array(fields)
    ret = []
    for field in fields:
        components = get_components(field)
        field_out = {}
        field_out['valueName'] = components[0].replace('?', '')
        field_out['valueType'] = components[1]
        field_out['valueDescription'] = components[2]

        valueOptionalOffset = 3
        # If value type is a number, restrictions are required. Else, should not be added.
        if field_out['valueType'].lower() == 'number':
            # In the case of a number, the optional component gets pushed back.
            valueOptionalOffset += 1
            field_out['valueRestrictions'] = components[3] if components[3].lower() != 'none' else None
        else:
            field_out['valueRestrictions'] = None

        field_out['valueOptional'] = components[0].startswith('?')
        if field_out['valueOptional']:
            field_out['valueOptionalBehavior'] = components[valueOptionalOffset] if len(components) > valueOptionalOffset else 'Unknown'
        else:
            field_out['valueOptionalBehavior'] = None
        ret.append(field_out)
    return ret

# Convert all response (or event data) fields from raw to final
def get_response_fields(fields):
    fields = field_to_array(fields)
    ret = []
    for field in fields:
        components = get_components(field)
        field_out = {}
        field_out['valueName'] = components[0]
        field_out['valueType'] = components[1]
        field_out['valueDescription'] = components[2]
        ret.append(field_out)
    return ret

#######################################################################################################################

# Read versions json
try:
    with open('../versions.json', 'r') as f:
        versions = json.load(f)
except IOError:
    logging.error('Failed to get global versions. Versions file not configured?')
    os.exit(1)

# Read the raw comments output file
with open('../work/comments.json', 'r') as f:
    comments_raw = json.load(f)

# Prepare output variables
enums = []
requests = []
events = []

enums_raw = {}
# Process the raw comments
for comment in comments_raw:
    # Skip unrelated comments like #include
    if 'api' not in comment:
        continue

    api = comment['api']
    if api == 'enums':
        if not validate_fields(comment, ['description', 'enumIdentifier', 'enumType', 'rpcVersion', 'initialVersion']):
            logging.warning('Failed to process enum id comment due to missing field(s):\n{}'.format(comment))
            continue

        enumType = field_to_string(comment['enumType'])

        enum = {}
        #                                     Recombines the header back into one string, allowing multi-line descriptions.
        enum['description'] = field_to_string(comment.get('lead', '')) + field_to_string(comment['description'])
        enum['enumIdentifier'] = field_to_string(comment['enumIdentifier'])
        rpcVersionRaw = field_to_string(comment['rpcVersion'])
        enum['rpcVersion'] = versions['rpcVersion'] if rpcVersionRaw == '-1' else int(rpcVersionRaw)
        enum['deprecated'] = False if rpcVersionRaw == '-1' else True
        enum['initialVersion'] = field_to_string(comment['initialVersion'])

        if 'enumValue' in comment:
            enumValue = field_to_string(comment['enumValue'])
            enum['enumValue'] = int(enumValue) if enumValue.isdigit() else enumValue
        else:
            enum['enumValue'] = enum['enumIdentifier']

        if enumType not in enums_raw:
            enums_raw[enumType] = {'enumIdentifiers': [enum]}
        else:
            enums_raw[enumType]['enumIdentifiers'].append(enum)

        logging.info('Processed enum: {}::{}'.format(enumType, enum['enumIdentifier']))
    elif api == 'requests':
        if not validate_fields(comment, ['description', 'requestType', 'complexity', 'rpcVersion', 'initialVersion', 'category']):
            logging.warning('Failed to process request comment due to missing field(s):\n{}'.format(comment))
            continue

        req = {}
        req['description'] = field_to_string(comment.get('lead', '')) + field_to_string(comment['description'])
        req['requestType'] = field_to_string(comment['requestType'])
        req['complexity'] = int(field_to_string(comment['complexity']))
        rpcVersionRaw = field_to_string(comment['rpcVersion'])
        req['rpcVersion'] = versions['rpcVersion'] if rpcVersionRaw == '-1' else int(rpcVersionRaw)
        req['deprecated'] = False if rpcVersionRaw == '-1' else True
        req['initialVersion'] = field_to_string(comment['initialVersion'])
        req['category'] = field_to_string(comment['category'])

        try:
            if 'requestField' in comment:
                req['requestFields'] = get_request_fields(comment['requestField'])
            else:
                req['requestFields'] = []
        except:
            logging.exception('Failed to process request `{}` request fields due to error:\n'.format(req['requestType']))
            continue

        try:
            if 'responseField' in comment:
                req['responseFields'] = get_response_fields(comment['responseField'])
            else:
                req['responseFields'] = []
        except:
            logging.exception('Failed to process request `{}` request fields due to error:\n'.format(req['requestType']))
            continue

        logging.info('Processed request: {}'.format(req['requestType']))

        requests.append(req)
    elif api == 'events':
        if not validate_fields(comment, ['description', 'eventType', 'eventSubscription', 'complexity', 'rpcVersion', 'initialVersion', 'category']):
            logging.warning('Failed to process event comment due to missing field(s):\n{}'.format(comment))
            continue

        eve = {}
        eve['description'] = field_to_string(comment.get('lead', '')) + field_to_string(comment['description'])
        eve['eventType'] = field_to_string(comment['eventType'])
        eve['eventSubscription'] = field_to_string(comment['eventSubscription'])
        eve['complexity'] = int(field_to_string(comment['complexity']))
        rpcVersionRaw = field_to_string(comment['rpcVersion'])
        eve['rpcVersion'] = versions['rpcVersion'] if rpcVersionRaw == '-1' else int(rpcVersionRaw)
        eve['deprecated'] = False if rpcVersionRaw == '-1' else True
        eve['initialVersion'] = field_to_string(comment['initialVersion'])
        eve['category'] = field_to_string(comment['category'])

        try:
            if 'dataField' in comment:
                eve['dataFields'] = get_response_fields(comment['dataField'])
            else:
                eve['dataFields'] = []
        except:
            logging.exception('Failed to process event `{}` data fields due to error:\n'.format(req['eventType']))
            continue

        logging.info('Processed event: {}'.format(eve['eventType']))

        events.append(eve)
    else:
        logging.warning('Comment with unknown api: {}'.format(api))

# Reconfigure enums to match the correct structure
for enumType in enums_raw.keys():
    enum = enums_raw[enumType]
    enums.append({'enumType': enumType, 'enumIdentifiers': enum['enumIdentifiers']})

finalObject = {'enums': enums, 'requests': requests, 'events': events}

with open('../generated/protocol.json', 'w') as f:
    json.dump(finalObject, f, indent=2)
