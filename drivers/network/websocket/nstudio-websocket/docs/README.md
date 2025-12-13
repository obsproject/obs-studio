# obs-websocket documentation

## If you're looking for the documentation page, it's [here](generated/protocol.md)

This is the documentation for obs-websocket. Run `build_docs.sh` to auto generate the latest docs from the `src` directory. There are 3 components to the docs generation:

- `comments/comments.js`: Generates the `work/comments.json` file from the code comments in the src directory.
- `docs/process_comments.py`: Processes `work/comments.json` to create `generated/protocol.json`, which is a machine-readable documentation format that can be used to create obs-websocket client libraries.
- `docs/generate_md.py`: Processes `generated/protocol.json` to create `generated/protocol.md`, which is the actual human-readable documentation.

Some notes about documenting:

- The `complexity` comment line is a suggestion to the user about how much knowledge about OBS's inner workings is required to safely use the associated feature. `1` for easy, `5` for megadeath-expert.
- The `rpcVersion` comment line is used to specify the latest available version that the feature is available in. If a feature is deprecated, then the placeholder value of `-1` should be replaced with the last RPC version that the feature will be available in. Manually specifying an RPC version automatically adds the `Deprecated` line to the entry in `generated/protocol.md`.
- The description can be multiple lines, but must be contained above the first documentation property (the lines starting with `@`).
- Value types are in reference to JSON values. The only ones you should use are `Any`, `String`, `Boolean`, `Number`, `Array`, `Object`.
  - `Array` types follow this format: `Array<subtype>`, for example `Array<String>` to specify an array of strings.

Formatting notes:

- Fields should have their columns aligned. So in a request, the columns of all `@requestField`s should be aligned.
- We suggest looking at how other enums/events/requests have been documented before documenting one of your own, to get a general feel of how things have been formatted.

## Creating enum documentation

Enums follow this code comment format:

```js
/**
* [description]
*
* @enumIdentifier [identifier]
* @enumValue [value]
* @enumType [type]
* @rpcVersion [latest available RPC version, use `-1` unless deprecated.]
* @initialVersion [first obs-websocket version this is found in]
* @api enums
*/
```

Example code comment:

```js
/**
* The initial message sent by obs-websocket to newly connected clients.
*
* @enumIdentifier Hello
* @enumValue 0
* @enumType WebSocketOpCode
* @rpcVersion -1
* @initialVersion 5.0.0
* @api enums
*/
```

- This is the documentation for the `WebSocketOpCode::Hello` enum identifier.

## Creating event documentation

Events follow this code comment format:

```js
/**
 * [description]
 *
 * @dataField [field name] | [value type] | [field description]
 * [... more @dataField entries ...]
 *
 * @eventType [type]
 * @eventSubscription [EventSubscription requirement]
 * @complexity [complexity rating, 1-5]
 * @rpcVersion [latest available RPC version, use `-1` unless deprecated.]
 * @initialVersion [first obs-websocket version this is found in]
 * @category [event category]
 * @api events
 */
```

Example code comment:

```js
/**
 * Studio mode has been enabled or disabled.
 *
 * @dataField studioModeEnabled | Boolean | True == Enabled, False == Disabled
 *
 * @eventType StudioModeStateChanged
 * @eventSubscription General
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category general
 * @api events
 */
```

## Creating request documentation

Requests follow this code comment format:

```js
/**
 * [description]
 *
 * @requestField [optional flag][field name] | [value type] | [field description] | [value restrictions (only include if the value type is `Number`)] | [default behavior (only include if optional flag is set)]
 * [... more @requestField entries ...]
 *
 * @responseField [field name] | [value type] | [field description]
 * [... more @responseField entries ...]
 *
 * @requestType [type]
 * @complexity [complexity rating, 1-5]
 * @rpcVersion [latest available RPC version, use `-1` unless deprecated.]
 * @initialVersion [first obs-websocket version this is found in]
 * @category [request category]
 * @api requests
 */
```

- The optional flag is a `?` that prefixes the field name, telling the docs processor that the field is optionally specified.

Example code comment:

```js
/**
 * Gets the value of a "slot" from the selected persistent data realm.
 *
 * @requestField realm    | String | The data realm to select. `OBS_WEBSOCKET_DATA_REALM_GLOBAL` or `OBS_WEBSOCKET_DATA_REALM_PROFILE`
 * @requestField slotName | String | The name of the slot to retrieve data from
 *
 * @responseField slotValue | String | Value associated with the slot. `null` if not set
 *
 * @requestType GetPersistentData
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api requests
 */
```
