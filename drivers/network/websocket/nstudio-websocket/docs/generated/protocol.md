<!-- This file was automatically generated. Do not edit directly! -->
<!-- markdownlint-disable no-bare-urls -->

# obs-websocket 5.x.x Protocol

## Main Table of Contents

- [General Intro](#general-intro)
  - [Design Goals](#design-goals)
- [Connecting to obs-websocket](#connecting-to-obs-websocket)
  - [Connection steps](#connection-steps)
    - [Connection Notes](#connection-notes)
  - [Creating an authentication string](#creating-an-authentication-string)
- [Message Types (OpCodes)](#message-types-opcodes)
  - [Hello (OpCode 0)](#hello-opcode-0)
  - [Identify (OpCode 1)](#identify-opcode-1)
  - [Identified (OpCode 2)](#identified-opcode-2)
  - [Reidentify (OpCode 3)](#reidentify-opcode-3)
  - [Event (OpCode 5)](#event-opcode-5)
  - [Request (OpCode 6)](#request-opcode-6)
  - [RequestResponse (OpCode 7)](#requestresponse-opcode-7)
  - [RequestBatch (OpCode 8)](#requestbatch-opcode-8)
  - [RequestBatchResponse (OpCode 9)](#requestbatchresponse-opcode-9)
- [Enumerations](#enums)
- [Events](#events)
- [Requests](#requests)

## General Intro

obs-websocket provides a feature-rich RPC communication protocol, giving access to much of OBS's feature set. This document contains everything you should know in order to make a connection and use obs-websocket's functionality to the fullest.

### Design Goals

- Abstraction of identification, events, requests, and batch requests into dedicated message types
- Conformity of request naming using similar terms like `Get`, `Set`, `Get[x]List`, `Start[x]`, `Toggle[x]`
- Conformity of OBS data field names like `sourceName`, `sourceKind`, `sourceType`, `sceneName`, `sceneItemName`
- Error code response system - integer corrosponds to type of error, with optional comment
- Possible support for multiple message encoding options: JSON and MessagePack
- PubSub system - Allow clients to specify which events they do or don't want to receive from OBS
- RPC versioning - Client and server negotiate the latest version of the obs-websocket protocol to communicate with.

## Connecting to obs-websocket

Here's info on how to connect to obs-websocket

---

### Connection steps

These steps should be followed precisely. Failure to connect to the server as instructed will likely result in your client being treated in an undefined way.

- Initial HTTP request made to the obs-websocket server.
  - The `Sec-WebSocket-Protocol` header can be used to tell obs-websocket which kind of message encoding to use. By default, obs-websocket uses JSON over text. Available subprotocols:
    - `obswebsocket.json` - JSON over text frames
    - `obswebsocket.msgpack` - MsgPack over binary frames

- Once the connection is upgraded, the websocket server will immediately send an [OpCode 0 `Hello`](#hello-opcode-0) message to the client.

- The client listens for the `Hello` and responds with an [OpCode 1 `Identify`](#identify-opcode-1) containing all appropriate session parameters.
  - If there is an `authentication` field in the `messageData` object, the server requires authentication, and the steps in [Creating an authentication string](#creating-an-authentication-string) should be followed.
  - If there is no `authentication` field, the resulting `Identify` object sent to the server does not require an `authentication` string.
  - The client determines if the server's `rpcVersion` is supported, and if not it provides its closest supported version in `Identify`.

- The server receives and processes the `Identify` sent by the client.
  - If authentication is required and the `Identify` message data does not contain an `authentication` string, or the string is not correct, the connection is closed with `WebSocketCloseCode::AuthenticationFailed`
  - If the client has requested an `rpcVersion` which the server cannot use, the connection is closed with `WebSocketCloseCode::UnsupportedRpcVersion`. This system allows both the server and client to have seamless backwards compatibility.
  - If any other parameters are malformed (invalid type, etc), the connection is closed with an appropriate close code.

- Once identification is processed on the server, the server responds to the client with an [OpCode 2 `Identified`](#identified-opcode-2).

- The client will begin receiving events from obs-websocket and may now make requests to obs-websocket.

- At any time after a client has been identified, it may send an [OpCode 3 `Reidentify`](#reidentify-opcode-3) message to update certain allowed session parameters. The server will respond in the same way it does during initial identification.

#### Connection Notes

- If a binary frame is received when using the `obswebsocket.json` (default) subprotocol, or a text frame is received while using the `obswebsocket.msgpack` subprotocol, the connection is closed with `WebSocketCloseCode::MessageDecodeError`.
- The obs-websocket server listens for any messages containing a `request-type` field in the first level JSON from unidentified clients. If a message matches, the connection is closed with `WebSocketCloseCode::UnsupportedRpcVersion` and a warning is logged.
- If a message with a `messageType` is not recognized to the obs-websocket server, the connection is closed with `WebSocketCloseCode::UnknownOpCode`.
- At no point may the client send any message other than a single `Identify` before it has received an `Identified`. Doing so will result in the connection being closed with `WebSocketCloseCode::NotIdentified`.

---

### Creating an authentication string

obs-websocket uses SHA256 to transmit authentication credentials. The server starts by sending an object in the `authentication` field of its `Hello` message data. The client processes the authentication challenge and responds via the `authentication` string in the `Identify` message data.

For this guide, we'll be using `supersecretpassword` as the password.

The `authentication` object in `Hello` looks like this (example):

```json
{
    "challenge": "+IxH4CnCiqpX1rM9scsNynZzbOe4KhDeYcTNS3PDaeY=",
    "salt": "lM1GncleQOaCu9lT1yeUZhFYnqhsLLP1G5lAGo3ixaI="
}
```

To generate the authentication string, follow these steps:

- Concatenate the websocket password with the `salt` provided by the server (`password + salt`)
- Generate an SHA256 binary hash of the result and base64 encode it, known as a base64 secret.
- Concatenate the base64 secret with the `challenge` sent by the server (`base64_secret + challenge`)
- Generate a binary SHA256 hash of that result and base64 encode it. You now have your `authentication` string.

For real-world examples of the `authentication` string creation, refer to the obs-websocket client libraries listed on the [README](../../README.md).

## Message Types (OpCodes)

The following message types are the low-level message types which may be sent to and from obs-websocket.

Messages sent from the obs-websocket server or client may contain these first-level fields, known as the base object:

```txt
{
  "op": number,
  "d": object
}
```

- `op` is a `WebSocketOpCode` OpCode.
- `d` is an object of the data fields associated with the operation.

---

### Hello (OpCode 0)

- Sent from: obs-websocket
- Sent to: Freshly connected websocket client
- Description: First message sent from the server immediately on client connection. Contains authentication information if auth is required. Also contains RPC version for version negotiation.

**Data Keys:**

```txt
{
  "obsStudioVersion": string,
  "obsWebSocketVersion": string,
  "rpcVersion": number,
  "authentication": object(optional)
}
```

- `rpcVersion` is a version number which gets incremented on each **breaking change** to the obs-websocket protocol. Its usage in this context is to provide the current rpc version that the server would like to use.
- `obsWebSocketVersion` may be used as a soft feature level hint. For example, a new WebSocket request may only be available in a specific obs-websocket version or newer, but the rpcVersion will not be increased, as no
  breaking changes have occured. Be aware, that no guarantees will be made on these assumptions, and you should still verify that the requests you desire to use are available in obs-websocket via the `GetVersion` request.

**Example Messages:**
Authentication is required

```json
{
  "op": 0,
  "d": {
    "obsStudioVersion": "30.2.2",
    "obsWebSocketVersion": "5.5.2",
    "rpcVersion": 1,
    "authentication": {
      "challenge": "+IxH4CnCiqpX1rM9scsNynZzbOe4KhDeYcTNS3PDaeY=",
      "salt": "lM1GncleQOaCu9lT1yeUZhFYnqhsLLP1G5lAGo3ixaI="
    }
  }
}
```

Authentication is not required

```json
{
  "op": 0,
  "d": {
    "obsStudioVersion": "30.2.2",
    "obsWebSocketVersion": "5.5.2",
    "rpcVersion": 1
  }
}
```

---

### Identify (OpCode 1)

- Sent from: Freshly connected websocket client
- Sent to: obs-websocket
- Description: Response to `Hello` message, should contain authentication string if authentication is required, along with PubSub subscriptions and other session parameters.

**Data Keys:**

```txt
{
  "rpcVersion": number,
  "authentication": string(optional),
  "eventSubscriptions": number(optional) = (EventSubscription::All)
}
```

- `rpcVersion` is the version number that the client would like the obs-websocket server to use.
- `eventSubscriptions` is a bitmask of `EventSubscriptions` items to subscribe to events and event categories at will. By default, all event categories are subscribed, except for events marked as high volume. High volume events must be explicitly subscribed to.

**Example Message:**

```json
{
  "op": 1,
  "d": {
    "rpcVersion": 1,
    "authentication": "Dj6cLS+jrNA0HpCArRg0Z/Fc+YHdt2FQfAvgD1mip6Y=",
    "eventSubscriptions": 33
  }
}
```

---

### Identified (OpCode 2)

- Sent from: obs-websocket
- Sent to: Freshly identified client
- Description: The identify request was received and validated, and the connection is now ready for normal operation.

**Data Keys:**

```txt
{
  "negotiatedRpcVersion": number
}
```

- If rpc version negotiation succeeds, the server determines the RPC version to be used and gives it to the client as `negotiatedRpcVersion`

**Example Message:**

```json
{
  "op": 2,
  "d": {
    "negotiatedRpcVersion": 1
  }
}
```

---

### Reidentify (OpCode 3)

- Sent from: Identified client
- Sent to: obs-websocket
- Description: Sent at any time after initial identification to update the provided session parameters.

**Data Keys:**

```txt
{
  "eventSubscriptions": number(optional) = (EventSubscription::All)
}
```

- Only the listed parameters may be changed after initial identification. To change a parameter not listed, you must reconnect to the obs-websocket server.

---

### Event (OpCode 5)

- Sent from: obs-websocket
- Sent to: All subscribed and identified clients
- Description: An event coming from OBS has occured. Eg scene switched, source muted.

**Data Keys:**

```txt
{
  "eventType": string,
  "eventIntent": number,
  "eventData": object(optional)
}
```

- `eventIntent` is the original intent required to be subscribed to in order to receive the event.

**Example Message:**

```json
{
  "op": 5,
  "d": {
    "eventType": "StudioModeStateChanged",
    "eventIntent": 1,
    "eventData": {
      "studioModeEnabled": true
    }
  }
}
```

---

### Request (OpCode 6)

- Sent from: Identified client
- Sent to: obs-websocket
- Description: Client is making a request to obs-websocket. Eg get current scene, create source.

**Data Keys:**

```txt
{
  "requestType": string,
  "requestId": string,
  "requestData": object(optional),

}
```

**Example Message:**

```json
{
  "op": 6,
  "d": {
    "requestType": "SetCurrentProgramScene",
    "requestId": "f819dcf0-89cc-11eb-8f0e-382c4ac93b9c",
    "requestData": {
      "sceneName": "Scene 12"
    }
  }
}
```

---

### RequestResponse (OpCode 7)

- Sent from: obs-websocket
- Sent to: Identified client which made the request
- Description: obs-websocket is responding to a request coming from a client.

**Data Keys:**

```txt
{
  "requestType": string,
  "requestId": string,
  "requestStatus": object,
  "responseData": object(optional)
}
```

- The `requestType` and `requestId` are simply mirrors of what was sent by the client.

`requestStatus` object:

```txt
{
  "result": bool,
  "code": number,
  "comment": string(optional)
}
```

- `result` is `true` if the request resulted in `RequestStatus::Success`. False if otherwise.
- `code` is a `RequestStatus` code.
- `comment` may be provided by the server on errors to offer further details on why a request failed.

**Example Messages:**
Successful Response

```json
{
  "op": 7,
  "d": {
    "requestType": "SetCurrentProgramScene",
    "requestId": "f819dcf0-89cc-11eb-8f0e-382c4ac93b9c",
    "requestStatus": {
      "result": true,
      "code": 100
    }
  }
}
```

Failure Response

```json
{
  "op": 7,
  "d": {
    "requestType": "SetCurrentProgramScene",
    "requestId": "f819dcf0-89cc-11eb-8f0e-382c4ac93b9c",
    "requestStatus": {
      "result": false,
      "code": 608,
      "comment": "Parameter: sceneName"
    }
  }
}
```

---

### RequestBatch (OpCode 8)

- Sent from: Identified client
- Sent to: obs-websocket
- Description: Client is making a batch of requests for obs-websocket. Requests are processed serially (in order) by the server.

**Data Keys:**

```txt
{
  "requestId": string,
  "haltOnFailure": bool(optional) = false,
  "executionType": number(optional) = RequestBatchExecutionType::SerialRealtime
  "requests": array<object>
}
```

- When `haltOnFailure` is `true`, the processing of requests will be halted on first failure. Returns only the processed requests in [`RequestBatchResponse`](#requestbatchresponse-opcode-9).
- Requests in the `requests` array follow the same structure as the `Request` payload data format, however `requestId` is an optional field.

---

### RequestBatchResponse (OpCode 9)

- Sent from: obs-websocket
- Sent to: Identified client which made the request
- Description: obs-websocket is responding to a request batch coming from the client.

**Data Keys:**

```txt
{
  "requestId": string,
  "results": array<object>
}
```

# Enums

These are enumeration declarations, which are referenced throughout obs-websocket's protocol.

## Enumerations Table of Contents

- [WebSocketOpCode](#websocketopcode)
  - [WebSocketOpCode::Hello](#websocketopcodehello)
  - [WebSocketOpCode::Identify](#websocketopcodeidentify)
  - [WebSocketOpCode::Identified](#websocketopcodeidentified)
  - [WebSocketOpCode::Reidentify](#websocketopcodereidentify)
  - [WebSocketOpCode::Event](#websocketopcodeevent)
  - [WebSocketOpCode::Request](#websocketopcoderequest)
  - [WebSocketOpCode::RequestResponse](#websocketopcoderequestresponse)
  - [WebSocketOpCode::RequestBatch](#websocketopcoderequestbatch)
  - [WebSocketOpCode::RequestBatchResponse](#websocketopcoderequestbatchresponse)
- [WebSocketCloseCode](#websocketclosecode)
  - [WebSocketCloseCode::DontClose](#websocketclosecodedontclose)
  - [WebSocketCloseCode::UnknownReason](#websocketclosecodeunknownreason)
  - [WebSocketCloseCode::MessageDecodeError](#websocketclosecodemessagedecodeerror)
  - [WebSocketCloseCode::MissingDataField](#websocketclosecodemissingdatafield)
  - [WebSocketCloseCode::InvalidDataFieldType](#websocketclosecodeinvaliddatafieldtype)
  - [WebSocketCloseCode::InvalidDataFieldValue](#websocketclosecodeinvaliddatafieldvalue)
  - [WebSocketCloseCode::UnknownOpCode](#websocketclosecodeunknownopcode)
  - [WebSocketCloseCode::NotIdentified](#websocketclosecodenotidentified)
  - [WebSocketCloseCode::AlreadyIdentified](#websocketclosecodealreadyidentified)
  - [WebSocketCloseCode::AuthenticationFailed](#websocketclosecodeauthenticationfailed)
  - [WebSocketCloseCode::UnsupportedRpcVersion](#websocketclosecodeunsupportedrpcversion)
  - [WebSocketCloseCode::SessionInvalidated](#websocketclosecodesessioninvalidated)
  - [WebSocketCloseCode::UnsupportedFeature](#websocketclosecodeunsupportedfeature)
- [RequestBatchExecutionType](#requestbatchexecutiontype)
  - [RequestBatchExecutionType::None](#requestbatchexecutiontypenone)
  - [RequestBatchExecutionType::SerialRealtime](#requestbatchexecutiontypeserialrealtime)
  - [RequestBatchExecutionType::SerialFrame](#requestbatchexecutiontypeserialframe)
  - [RequestBatchExecutionType::Parallel](#requestbatchexecutiontypeparallel)
- [RequestStatus](#requeststatus)
  - [RequestStatus::Unknown](#requeststatusunknown)
  - [RequestStatus::NoError](#requeststatusnoerror)
  - [RequestStatus::Success](#requeststatussuccess)
  - [RequestStatus::MissingRequestType](#requeststatusmissingrequesttype)
  - [RequestStatus::UnknownRequestType](#requeststatusunknownrequesttype)
  - [RequestStatus::GenericError](#requeststatusgenericerror)
  - [RequestStatus::UnsupportedRequestBatchExecutionType](#requeststatusunsupportedrequestbatchexecutiontype)
  - [RequestStatus::NotReady](#requeststatusnotready)
  - [RequestStatus::MissingRequestField](#requeststatusmissingrequestfield)
  - [RequestStatus::MissingRequestData](#requeststatusmissingrequestdata)
  - [RequestStatus::InvalidRequestField](#requeststatusinvalidrequestfield)
  - [RequestStatus::InvalidRequestFieldType](#requeststatusinvalidrequestfieldtype)
  - [RequestStatus::RequestFieldOutOfRange](#requeststatusrequestfieldoutofrange)
  - [RequestStatus::RequestFieldEmpty](#requeststatusrequestfieldempty)
  - [RequestStatus::TooManyRequestFields](#requeststatustoomanyrequestfields)
  - [RequestStatus::OutputRunning](#requeststatusoutputrunning)
  - [RequestStatus::OutputNotRunning](#requeststatusoutputnotrunning)
  - [RequestStatus::OutputPaused](#requeststatusoutputpaused)
  - [RequestStatus::OutputNotPaused](#requeststatusoutputnotpaused)
  - [RequestStatus::OutputDisabled](#requeststatusoutputdisabled)
  - [RequestStatus::StudioModeActive](#requeststatusstudiomodeactive)
  - [RequestStatus::StudioModeNotActive](#requeststatusstudiomodenotactive)
  - [RequestStatus::ResourceNotFound](#requeststatusresourcenotfound)
  - [RequestStatus::ResourceAlreadyExists](#requeststatusresourcealreadyexists)
  - [RequestStatus::InvalidResourceType](#requeststatusinvalidresourcetype)
  - [RequestStatus::NotEnoughResources](#requeststatusnotenoughresources)
  - [RequestStatus::InvalidResourceState](#requeststatusinvalidresourcestate)
  - [RequestStatus::InvalidInputKind](#requeststatusinvalidinputkind)
  - [RequestStatus::ResourceNotConfigurable](#requeststatusresourcenotconfigurable)
  - [RequestStatus::InvalidFilterKind](#requeststatusinvalidfilterkind)
  - [RequestStatus::ResourceCreationFailed](#requeststatusresourcecreationfailed)
  - [RequestStatus::ResourceActionFailed](#requeststatusresourceactionfailed)
  - [RequestStatus::RequestProcessingFailed](#requeststatusrequestprocessingfailed)
  - [RequestStatus::CannotAct](#requeststatuscannotact)
- [EventSubscription](#eventsubscription)
  - [EventSubscription::None](#eventsubscriptionnone)
  - [EventSubscription::General](#eventsubscriptiongeneral)
  - [EventSubscription::Config](#eventsubscriptionconfig)
  - [EventSubscription::Scenes](#eventsubscriptionscenes)
  - [EventSubscription::Inputs](#eventsubscriptioninputs)
  - [EventSubscription::Transitions](#eventsubscriptiontransitions)
  - [EventSubscription::Filters](#eventsubscriptionfilters)
  - [EventSubscription::Outputs](#eventsubscriptionoutputs)
  - [EventSubscription::SceneItems](#eventsubscriptionsceneitems)
  - [EventSubscription::MediaInputs](#eventsubscriptionmediainputs)
  - [EventSubscription::Vendors](#eventsubscriptionvendors)
  - [EventSubscription::Ui](#eventsubscriptionui)
  - [EventSubscription::All](#eventsubscriptionall)
  - [EventSubscription::InputVolumeMeters](#eventsubscriptioninputvolumemeters)
  - [EventSubscription::InputActiveStateChanged](#eventsubscriptioninputactivestatechanged)
  - [EventSubscription::InputShowStateChanged](#eventsubscriptioninputshowstatechanged)
  - [EventSubscription::SceneItemTransformChanged](#eventsubscriptionsceneitemtransformchanged)
- [ObsMediaInputAction](#obsmediainputaction)
  - [ObsMediaInputAction::OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NONE](#obsmediainputactionobs_websocket_media_input_action_none)
  - [ObsMediaInputAction::OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PLAY](#obsmediainputactionobs_websocket_media_input_action_play)
  - [ObsMediaInputAction::OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PAUSE](#obsmediainputactionobs_websocket_media_input_action_pause)
  - [ObsMediaInputAction::OBS_WEBSOCKET_MEDIA_INPUT_ACTION_STOP](#obsmediainputactionobs_websocket_media_input_action_stop)
  - [ObsMediaInputAction::OBS_WEBSOCKET_MEDIA_INPUT_ACTION_RESTART](#obsmediainputactionobs_websocket_media_input_action_restart)
  - [ObsMediaInputAction::OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NEXT](#obsmediainputactionobs_websocket_media_input_action_next)
  - [ObsMediaInputAction::OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PREVIOUS](#obsmediainputactionobs_websocket_media_input_action_previous)
- [ObsOutputState](#obsoutputstate)
  - [ObsOutputState::OBS_WEBSOCKET_OUTPUT_UNKNOWN](#obsoutputstateobs_websocket_output_unknown)
  - [ObsOutputState::OBS_WEBSOCKET_OUTPUT_STARTING](#obsoutputstateobs_websocket_output_starting)
  - [ObsOutputState::OBS_WEBSOCKET_OUTPUT_STARTED](#obsoutputstateobs_websocket_output_started)
  - [ObsOutputState::OBS_WEBSOCKET_OUTPUT_STOPPING](#obsoutputstateobs_websocket_output_stopping)
  - [ObsOutputState::OBS_WEBSOCKET_OUTPUT_STOPPED](#obsoutputstateobs_websocket_output_stopped)
  - [ObsOutputState::OBS_WEBSOCKET_OUTPUT_RECONNECTING](#obsoutputstateobs_websocket_output_reconnecting)
  - [ObsOutputState::OBS_WEBSOCKET_OUTPUT_RECONNECTED](#obsoutputstateobs_websocket_output_reconnected)
  - [ObsOutputState::OBS_WEBSOCKET_OUTPUT_PAUSED](#obsoutputstateobs_websocket_output_paused)
  - [ObsOutputState::OBS_WEBSOCKET_OUTPUT_RESUMED](#obsoutputstateobs_websocket_output_resumed)

## WebSocketOpCode

### WebSocketOpCode::Hello

The initial message sent by obs-websocket to newly connected clients.

- Identifier Value: `0`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketOpCode::Identify

The message sent by a newly connected client to obs-websocket in response to a `Hello`.

- Identifier Value: `1`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketOpCode::Identified

The response sent by obs-websocket to a client after it has successfully identified with obs-websocket.

- Identifier Value: `2`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketOpCode::Reidentify

The message sent by an already-identified client to update identification parameters.

- Identifier Value: `3`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketOpCode::Event

The message sent by obs-websocket containing an event payload.

- Identifier Value: `5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketOpCode::Request

The message sent by a client to obs-websocket to perform a request.

- Identifier Value: `6`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketOpCode::RequestResponse

The message sent by obs-websocket in response to a particular request from a client.

- Identifier Value: `7`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketOpCode::RequestBatch

The message sent by a client to obs-websocket to perform a batch of requests.

- Identifier Value: `8`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketOpCode::RequestBatchResponse

The message sent by obs-websocket in response to a particular batch of requests from a client.

- Identifier Value: `9`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

## WebSocketCloseCode

### WebSocketCloseCode::DontClose

For internal use only to tell the request handler not to perform any close action.

- Identifier Value: `0`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketCloseCode::UnknownReason

Unknown reason, should never be used.

- Identifier Value: `4000`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketCloseCode::MessageDecodeError

The server was unable to decode the incoming websocket message.

- Identifier Value: `4002`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketCloseCode::MissingDataField

A data field is required but missing from the payload.

- Identifier Value: `4003`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketCloseCode::InvalidDataFieldType

A data field's value type is invalid.

- Identifier Value: `4004`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketCloseCode::InvalidDataFieldValue

A data field's value is invalid.

- Identifier Value: `4005`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketCloseCode::UnknownOpCode

The specified `op` was invalid or missing.

- Identifier Value: `4006`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketCloseCode::NotIdentified

The client sent a websocket message without first sending `Identify` message.

- Identifier Value: `4007`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketCloseCode::AlreadyIdentified

The client sent an `Identify` message while already identified.

Note: Once a client has identified, only `Reidentify` may be used to change session parameters.

- Identifier Value: `4008`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketCloseCode::AuthenticationFailed

The authentication attempt (via `Identify`) failed.

- Identifier Value: `4009`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketCloseCode::UnsupportedRpcVersion

The server detected the usage of an old version of the obs-websocket RPC protocol.

- Identifier Value: `4010`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketCloseCode::SessionInvalidated

The websocket session has been invalidated by the obs-websocket server.

Note: This is the code used by the `Kick` button in the UI Session List. If you receive this code, you must not automatically reconnect.

- Identifier Value: `4011`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### WebSocketCloseCode::UnsupportedFeature

A requested feature is not supported due to hardware/software limitations.

- Identifier Value: `4012`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

## RequestBatchExecutionType

### RequestBatchExecutionType::None

Not a request batch.

- Identifier Value: `-1`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestBatchExecutionType::SerialRealtime

A request batch which processes all requests serially, as fast as possible.

Note: To introduce artificial delay, use the `Sleep` request and the `sleepMillis` request field.

- Identifier Value: `0`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestBatchExecutionType::SerialFrame

A request batch type which processes all requests serially, in sync with the graphics thread. Designed to provide high accuracy for animations.

Note: To introduce artificial delay, use the `Sleep` request and the `sleepFrames` request field.

- Identifier Value: `1`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestBatchExecutionType::Parallel

A request batch type which processes all requests using all available threads in the thread pool.

Note: This is mainly experimental, and only really shows its colors during requests which require lots of
active processing, like `GetSourceScreenshot`.

- Identifier Value: `2`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

## RequestStatus

### RequestStatus::Unknown

Unknown status, should never be used.

- Identifier Value: `0`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::NoError

For internal use to signify a successful field check.

- Identifier Value: `10`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::Success

The request has succeeded.

- Identifier Value: `100`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::MissingRequestType

The `requestType` field is missing from the request data.

- Identifier Value: `203`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::UnknownRequestType

The request type is invalid or does not exist.

- Identifier Value: `204`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::GenericError

Generic error code.

Note: A comment is required to be provided by obs-websocket.

- Identifier Value: `205`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::UnsupportedRequestBatchExecutionType

The request batch execution type is not supported.

- Identifier Value: `206`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::NotReady

The server is not ready to handle the request.

Note: This usually occurs during OBS scene collection change or exit. Requests may be tried again after a delay if this code is given.

- Identifier Value: `207`
- Latest Supported RPC Version: `1`
- Added in v5.3.0

---

### RequestStatus::MissingRequestField

A required request field is missing.

- Identifier Value: `300`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::MissingRequestData

The request does not have a valid requestData object.

- Identifier Value: `301`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::InvalidRequestField

Generic invalid request field message.

Note: A comment is required to be provided by obs-websocket.

- Identifier Value: `400`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::InvalidRequestFieldType

A request field has the wrong data type.

- Identifier Value: `401`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::RequestFieldOutOfRange

A request field (number) is outside of the allowed range.

- Identifier Value: `402`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::RequestFieldEmpty

A request field (string or array) is empty and cannot be.

- Identifier Value: `403`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::TooManyRequestFields

There are too many request fields (eg. a request takes two optionals, where only one is allowed at a time).

- Identifier Value: `404`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::OutputRunning

An output is running and cannot be in order to perform the request.

- Identifier Value: `500`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::OutputNotRunning

An output is not running and should be.

- Identifier Value: `501`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::OutputPaused

An output is paused and should not be.

- Identifier Value: `502`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::OutputNotPaused

An output is not paused and should be.

- Identifier Value: `503`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::OutputDisabled

An output is disabled and should not be.

- Identifier Value: `504`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::StudioModeActive

Studio mode is active and cannot be.

- Identifier Value: `505`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::StudioModeNotActive

Studio mode is not active and should be.

- Identifier Value: `506`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::ResourceNotFound

The resource was not found.

Note: Resources are any kind of object in obs-websocket, like inputs, profiles, outputs, etc.

- Identifier Value: `600`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::ResourceAlreadyExists

The resource already exists.

- Identifier Value: `601`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::InvalidResourceType

The type of resource found is invalid.

- Identifier Value: `602`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::NotEnoughResources

There are not enough instances of the resource in order to perform the request.

- Identifier Value: `603`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::InvalidResourceState

The state of the resource is invalid. For example, if the resource is blocked from being accessed.

- Identifier Value: `604`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::InvalidInputKind

The specified input (obs_source_t-OBS_SOURCE_TYPE_INPUT) had the wrong kind.

- Identifier Value: `605`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::ResourceNotConfigurable

The resource does not support being configured.

This is particularly relevant to transitions, where they do not always have changeable settings.

- Identifier Value: `606`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::InvalidFilterKind

The specified filter (obs_source_t-OBS_SOURCE_TYPE_FILTER) had the wrong kind.

- Identifier Value: `607`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::ResourceCreationFailed

Creating the resource failed.

- Identifier Value: `700`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::ResourceActionFailed

Performing an action on the resource failed.

- Identifier Value: `701`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::RequestProcessingFailed

Processing the request failed unexpectedly.

Note: A comment is required to be provided by obs-websocket.

- Identifier Value: `702`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### RequestStatus::CannotAct

The combination of request fields cannot be used to perform an action.

- Identifier Value: `703`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

## EventSubscription

### EventSubscription::None

Subcription value used to disable all events.

- Identifier Value: `0`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### EventSubscription::General

Subscription value to receive events in the `General` category.

- Identifier Value: `(1 << 0)`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### EventSubscription::Config

Subscription value to receive events in the `Config` category.

- Identifier Value: `(1 << 1)`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### EventSubscription::Scenes

Subscription value to receive events in the `Scenes` category.

- Identifier Value: `(1 << 2)`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### EventSubscription::Inputs

Subscription value to receive events in the `Inputs` category.

- Identifier Value: `(1 << 3)`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### EventSubscription::Transitions

Subscription value to receive events in the `Transitions` category.

- Identifier Value: `(1 << 4)`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### EventSubscription::Filters

Subscription value to receive events in the `Filters` category.

- Identifier Value: `(1 << 5)`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### EventSubscription::Outputs

Subscription value to receive events in the `Outputs` category.

- Identifier Value: `(1 << 6)`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### EventSubscription::SceneItems

Subscription value to receive events in the `SceneItems` category.

- Identifier Value: `(1 << 7)`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### EventSubscription::MediaInputs

Subscription value to receive events in the `MediaInputs` category.

- Identifier Value: `(1 << 8)`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### EventSubscription::Vendors

Subscription value to receive the `VendorEvent` event.

- Identifier Value: `(1 << 9)`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### EventSubscription::Ui

Subscription value to receive events in the `Ui` category.

- Identifier Value: `(1 << 10)`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### EventSubscription::All

Helper to receive all non-high-volume events.

- Identifier Value: `(General | Config | Scenes | Inputs | Transitions | Filters | Outputs | SceneItems | MediaInputs | Vendors | Ui)`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### EventSubscription::InputVolumeMeters

Subscription value to receive the `InputVolumeMeters` high-volume event.

- Identifier Value: `(1 << 16)`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### EventSubscription::InputActiveStateChanged

Subscription value to receive the `InputActiveStateChanged` high-volume event.

- Identifier Value: `(1 << 17)`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### EventSubscription::InputShowStateChanged

Subscription value to receive the `InputShowStateChanged` high-volume event.

- Identifier Value: `(1 << 18)`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### EventSubscription::SceneItemTransformChanged

Subscription value to receive the `SceneItemTransformChanged` high-volume event.

- Identifier Value: `(1 << 19)`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

## ObsMediaInputAction

### ObsMediaInputAction::OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NONE

No action.

- Identifier Value: `OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NONE`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### ObsMediaInputAction::OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PLAY

Play the media input.

- Identifier Value: `OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PLAY`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### ObsMediaInputAction::OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PAUSE

Pause the media input.

- Identifier Value: `OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PAUSE`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### ObsMediaInputAction::OBS_WEBSOCKET_MEDIA_INPUT_ACTION_STOP

Stop the media input.

- Identifier Value: `OBS_WEBSOCKET_MEDIA_INPUT_ACTION_STOP`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### ObsMediaInputAction::OBS_WEBSOCKET_MEDIA_INPUT_ACTION_RESTART

Restart the media input.

- Identifier Value: `OBS_WEBSOCKET_MEDIA_INPUT_ACTION_RESTART`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### ObsMediaInputAction::OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NEXT

Go to the next playlist item.

- Identifier Value: `OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NEXT`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### ObsMediaInputAction::OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PREVIOUS

Go to the previous playlist item.

- Identifier Value: `OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PREVIOUS`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

## ObsOutputState

### ObsOutputState::OBS_WEBSOCKET_OUTPUT_UNKNOWN

Unknown state.

- Identifier Value: `OBS_WEBSOCKET_OUTPUT_UNKNOWN`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### ObsOutputState::OBS_WEBSOCKET_OUTPUT_STARTING

The output is starting.

- Identifier Value: `OBS_WEBSOCKET_OUTPUT_STARTING`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### ObsOutputState::OBS_WEBSOCKET_OUTPUT_STARTED

The input has started.

- Identifier Value: `OBS_WEBSOCKET_OUTPUT_STARTED`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### ObsOutputState::OBS_WEBSOCKET_OUTPUT_STOPPING

The output is stopping.

- Identifier Value: `OBS_WEBSOCKET_OUTPUT_STOPPING`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### ObsOutputState::OBS_WEBSOCKET_OUTPUT_STOPPED

The output has stopped.

- Identifier Value: `OBS_WEBSOCKET_OUTPUT_STOPPED`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### ObsOutputState::OBS_WEBSOCKET_OUTPUT_RECONNECTING

The output has disconnected and is reconnecting.

- Identifier Value: `OBS_WEBSOCKET_OUTPUT_RECONNECTING`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### ObsOutputState::OBS_WEBSOCKET_OUTPUT_RECONNECTED

The output has reconnected successfully.

- Identifier Value: `OBS_WEBSOCKET_OUTPUT_RECONNECTED`
- Latest Supported RPC Version: `1`
- Added in v5.1.0

---

### ObsOutputState::OBS_WEBSOCKET_OUTPUT_PAUSED

The output is now paused.

- Identifier Value: `OBS_WEBSOCKET_OUTPUT_PAUSED`
- Latest Supported RPC Version: `1`
- Added in v5.1.0

---

### ObsOutputState::OBS_WEBSOCKET_OUTPUT_RESUMED

The output has been resumed (unpaused).

- Identifier Value: `OBS_WEBSOCKET_OUTPUT_RESUMED`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

# Events

## Events Table of Contents

- [General Events](#general-events)
  - [ExitStarted](#exitstarted)
  - [VendorEvent](#vendorevent)
  - [CustomEvent](#customevent)
- [Config Events](#config-events)
  - [CurrentSceneCollectionChanging](#currentscenecollectionchanging)
  - [CurrentSceneCollectionChanged](#currentscenecollectionchanged)
  - [SceneCollectionListChanged](#scenecollectionlistchanged)
  - [CurrentProfileChanging](#currentprofilechanging)
  - [CurrentProfileChanged](#currentprofilechanged)
  - [ProfileListChanged](#profilelistchanged)
- [Scenes Events](#scenes-events)
  - [SceneCreated](#scenecreated)
  - [SceneRemoved](#sceneremoved)
  - [SceneNameChanged](#scenenamechanged)
  - [CurrentProgramSceneChanged](#currentprogramscenechanged)
  - [CurrentPreviewSceneChanged](#currentpreviewscenechanged)
  - [SceneListChanged](#scenelistchanged)
- [Inputs Events](#inputs-events)
  - [InputCreated](#inputcreated)
  - [InputRemoved](#inputremoved)
  - [InputNameChanged](#inputnamechanged)
  - [InputSettingsChanged](#inputsettingschanged)
  - [InputActiveStateChanged](#inputactivestatechanged)
  - [InputShowStateChanged](#inputshowstatechanged)
  - [InputMuteStateChanged](#inputmutestatechanged)
  - [InputVolumeChanged](#inputvolumechanged)
  - [InputAudioBalanceChanged](#inputaudiobalancechanged)
  - [InputAudioSyncOffsetChanged](#inputaudiosyncoffsetchanged)
  - [InputAudioTracksChanged](#inputaudiotrackschanged)
  - [InputAudioMonitorTypeChanged](#inputaudiomonitortypechanged)
  - [InputVolumeMeters](#inputvolumemeters)
- [Transitions Events](#transitions-events)
  - [CurrentSceneTransitionChanged](#currentscenetransitionchanged)
  - [CurrentSceneTransitionDurationChanged](#currentscenetransitiondurationchanged)
  - [SceneTransitionStarted](#scenetransitionstarted)
  - [SceneTransitionEnded](#scenetransitionended)
  - [SceneTransitionVideoEnded](#scenetransitionvideoended)
- [Filters Events](#filters-events)
  - [SourceFilterListReindexed](#sourcefilterlistreindexed)
  - [SourceFilterCreated](#sourcefiltercreated)
  - [SourceFilterRemoved](#sourcefilterremoved)
  - [SourceFilterNameChanged](#sourcefilternamechanged)
  - [SourceFilterSettingsChanged](#sourcefiltersettingschanged)
  - [SourceFilterEnableStateChanged](#sourcefilterenablestatechanged)
- [Scene Items Events](#scene-items-events)
  - [SceneItemCreated](#sceneitemcreated)
  - [SceneItemRemoved](#sceneitemremoved)
  - [SceneItemListReindexed](#sceneitemlistreindexed)
  - [SceneItemEnableStateChanged](#sceneitemenablestatechanged)
  - [SceneItemLockStateChanged](#sceneitemlockstatechanged)
  - [SceneItemSelected](#sceneitemselected)
  - [SceneItemTransformChanged](#sceneitemtransformchanged)
- [Outputs Events](#outputs-events)
  - [StreamStateChanged](#streamstatechanged)
  - [RecordStateChanged](#recordstatechanged)
  - [RecordFileChanged](#recordfilechanged)
  - [ReplayBufferStateChanged](#replaybufferstatechanged)
  - [VirtualcamStateChanged](#virtualcamstatechanged)
  - [ReplayBufferSaved](#replaybuffersaved)
- [Media Inputs Events](#media-inputs-events)
  - [MediaInputPlaybackStarted](#mediainputplaybackstarted)
  - [MediaInputPlaybackEnded](#mediainputplaybackended)
  - [MediaInputActionTriggered](#mediainputactiontriggered)
- [Ui Events](#ui-events)
  - [StudioModeStateChanged](#studiomodestatechanged)
  - [ScreenshotSaved](#screenshotsaved)

## General Events

### ExitStarted

OBS has begun the shutdown process.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### VendorEvent

An event has been emitted from a vendor.

A vendor is a unique name registered by a third-party plugin or script, which allows for custom requests and events to be added to obs-websocket.
If a plugin or script implements vendor requests or events, documentation is expected to be provided with them.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| vendorName | String | Name of the vendor emitting the event |
| eventType | String | Vendor-provided event typedef |
| eventData | Object | Vendor-provided event data. {} if event does not provide any data |

---

### CustomEvent

Custom event emitted by `BroadcastCustomEvent`.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| eventData | Object | Custom event data |

## Config Events

### CurrentSceneCollectionChanging

The current scene collection has begun changing.

Note: We recommend using this event to trigger a pause of all polling requests, as performing any requests during a
scene collection change is considered undefined behavior and can cause crashes!

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneCollectionName | String | Name of the current scene collection |

---

### CurrentSceneCollectionChanged

The current scene collection has changed.

Note: If polling has been paused during `CurrentSceneCollectionChanging`, this is the que to restart polling.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneCollectionName | String | Name of the new scene collection |

---

### SceneCollectionListChanged

The scene collection list has changed.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneCollections | Array&lt;String&gt; | Updated list of scene collections |

---

### CurrentProfileChanging

The current profile has begun changing.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| profileName | String | Name of the current profile |

---

### CurrentProfileChanged

The current profile has changed.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| profileName | String | Name of the new profile |

---

### ProfileListChanged

The profile list has changed.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| profiles | Array&lt;String&gt; | Updated list of profiles |

## Scenes Events

### SceneCreated

A new scene has been created.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneName | String | Name of the new scene |
| sceneUuid | String | UUID of the new scene |
| isGroup | Boolean | Whether the new scene is a group |

---

### SceneRemoved

A scene has been removed.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneName | String | Name of the removed scene |
| sceneUuid | String | UUID of the removed scene |
| isGroup | Boolean | Whether the scene was a group |

---

### SceneNameChanged

The name of a scene has changed.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneUuid | String | UUID of the scene |
| oldSceneName | String | Old name of the scene |
| sceneName | String | New name of the scene |

---

### CurrentProgramSceneChanged

The current program scene has changed.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneName | String | Name of the scene that was switched to |
| sceneUuid | String | UUID of the scene that was switched to |

---

### CurrentPreviewSceneChanged

The current preview scene has changed.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneName | String | Name of the scene that was switched to |
| sceneUuid | String | UUID of the scene that was switched to |

---

### SceneListChanged

The list of scenes has changed.

TODO: Make OBS fire this event when scenes are reordered.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| scenes | Array&lt;Object&gt; | Updated array of scenes |

## Inputs Events

### InputCreated

An input has been created.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputName | String | Name of the input |
| inputUuid | String | UUID of the input |
| inputKind | String | The kind of the input |
| unversionedInputKind | String | The unversioned kind of input (aka no `_v2` stuff) |
| inputKindCaps | Number | Bitflag value for the caps that an input supports. See obs_source_info.output_flags in the libobs docs |
| inputSettings | Object | The settings configured to the input when it was created |
| defaultInputSettings | Object | The default settings for the input |

---

### InputRemoved

An input has been removed.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputName | String | Name of the input |
| inputUuid | String | UUID of the input |

---

### InputNameChanged

The name of an input has changed.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputUuid | String | UUID of the input |
| oldInputName | String | Old name of the input |
| inputName | String | New name of the input |

---

### InputSettingsChanged

An input's settings have changed (been updated).

Note: On some inputs, changing values in the properties dialog will cause an immediate update. Pressing the "Cancel" button will revert the settings, resulting in another event being fired.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.4.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputName | String | Name of the input |
| inputUuid | String | UUID of the input |
| inputSettings | Object | New settings object of the input |

---

### InputActiveStateChanged

An input's active state has changed.

When an input is active, it means it's being shown by the program feed.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputName | String | Name of the input |
| inputUuid | String | UUID of the input |
| videoActive | Boolean | Whether the input is active |

---

### InputShowStateChanged

An input's show state has changed.

When an input is showing, it means it's being shown by the preview or a dialog.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputName | String | Name of the input |
| inputUuid | String | UUID of the input |
| videoShowing | Boolean | Whether the input is showing |

---

### InputMuteStateChanged

An input's mute state has changed.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputName | String | Name of the input |
| inputUuid | String | UUID of the input |
| inputMuted | Boolean | Whether the input is muted |

---

### InputVolumeChanged

An input's volume level has changed.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputName | String | Name of the input |
| inputUuid | String | UUID of the input |
| inputVolumeMul | Number | New volume level multiplier |
| inputVolumeDb | Number | New volume level in dB |

---

### InputAudioBalanceChanged

The audio balance value of an input has changed.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputName | String | Name of the input |
| inputUuid | String | UUID of the input |
| inputAudioBalance | Number | New audio balance value of the input |

---

### InputAudioSyncOffsetChanged

The sync offset of an input has changed.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputName | String | Name of the input |
| inputUuid | String | UUID of the input |
| inputAudioSyncOffset | Number | New sync offset in milliseconds |

---

### InputAudioTracksChanged

The audio tracks of an input have changed.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputName | String | Name of the input |
| inputUuid | String | UUID of the input |
| inputAudioTracks | Object | Object of audio tracks along with their associated enable states |

---

### InputAudioMonitorTypeChanged

The monitor type of an input has changed.

Available types are:

- `OBS_MONITORING_TYPE_NONE`
- `OBS_MONITORING_TYPE_MONITOR_ONLY`
- `OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT`

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputName | String | Name of the input |
| inputUuid | String | UUID of the input |
| monitorType | String | New monitor type of the input |

---

### InputVolumeMeters

A high-volume event providing volume levels of all active inputs every 50 milliseconds.

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputs | Array&lt;Object&gt; | Array of active inputs with their associated volume levels |

## Transitions Events

### CurrentSceneTransitionChanged

The current scene transition has changed.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| transitionName | String | Name of the new transition |
| transitionUuid | String | UUID of the new transition |

---

### CurrentSceneTransitionDurationChanged

The current scene transition duration has changed.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| transitionDuration | Number | Transition duration in milliseconds |

---

### SceneTransitionStarted

A scene transition has started.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| transitionName | String | Scene transition name |
| transitionUuid | String | Scene transition UUID |

---

### SceneTransitionEnded

A scene transition has completed fully.

Note: Does not appear to trigger when the transition is interrupted by the user.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| transitionName | String | Scene transition name |
| transitionUuid | String | Scene transition UUID |

---

### SceneTransitionVideoEnded

A scene transition's video has completed fully.

Useful for stinger transitions to tell when the video *actually* ends.
`SceneTransitionEnded` only signifies the cut point, not the completion of transition playback.

Note: Appears to be called by every transition, regardless of relevance.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| transitionName | String | Scene transition name |
| transitionUuid | String | Scene transition UUID |

## Filters Events

### SourceFilterListReindexed

A source's filter list has been reindexed.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sourceName | String | Name of the source |
| filters | Array&lt;Object&gt; | Array of filter objects |

---

### SourceFilterCreated

A filter has been added to a source.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sourceName | String | Name of the source the filter was added to |
| filterName | String | Name of the filter |
| filterKind | String | The kind of the filter |
| filterIndex | Number | Index position of the filter |
| filterSettings | Object | The settings configured to the filter when it was created |
| defaultFilterSettings | Object | The default settings for the filter |

---

### SourceFilterRemoved

A filter has been removed from a source.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sourceName | String | Name of the source the filter was on |
| filterName | String | Name of the filter |

---

### SourceFilterNameChanged

The name of a source filter has changed.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sourceName | String | The source the filter is on |
| oldFilterName | String | Old name of the filter |
| filterName | String | New name of the filter |

---

### SourceFilterSettingsChanged

An source filter's settings have changed (been updated).

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.4.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sourceName | String | Name of the source the filter is on |
| filterName | String | Name of the filter |
| filterSettings | Object | New settings object of the filter |

---

### SourceFilterEnableStateChanged

A source filter's enable state has changed.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sourceName | String | Name of the source the filter is on |
| filterName | String | Name of the filter |
| filterEnabled | Boolean | Whether the filter is enabled |

## Scene Items Events

### SceneItemCreated

A scene item has been created.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneName | String | Name of the scene the item was added to |
| sceneUuid | String | UUID of the scene the item was added to |
| sourceName | String | Name of the underlying source (input/scene) |
| sourceUuid | String | UUID of the underlying source (input/scene) |
| sceneItemId | Number | Numeric ID of the scene item |
| sceneItemIndex | Number | Index position of the item |

---

### SceneItemRemoved

A scene item has been removed.

This event is not emitted when the scene the item is in is removed.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneName | String | Name of the scene the item was removed from |
| sceneUuid | String | UUID of the scene the item was removed from |
| sourceName | String | Name of the underlying source (input/scene) |
| sourceUuid | String | UUID of the underlying source (input/scene) |
| sceneItemId | Number | Numeric ID of the scene item |

---

### SceneItemListReindexed

A scene's item list has been reindexed.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneName | String | Name of the scene |
| sceneUuid | String | UUID of the scene |
| sceneItems | Array&lt;Object&gt; | Array of scene item objects |

---

### SceneItemEnableStateChanged

A scene item's enable state has changed.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneName | String | Name of the scene the item is in |
| sceneUuid | String | UUID of the scene the item is in |
| sceneItemId | Number | Numeric ID of the scene item |
| sceneItemEnabled | Boolean | Whether the scene item is enabled (visible) |

---

### SceneItemLockStateChanged

A scene item's lock state has changed.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneName | String | Name of the scene the item is in |
| sceneUuid | String | UUID of the scene the item is in |
| sceneItemId | Number | Numeric ID of the scene item |
| sceneItemLocked | Boolean | Whether the scene item is locked |

---

### SceneItemSelected

A scene item has been selected in the Ui.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneName | String | Name of the scene the item is in |
| sceneUuid | String | UUID of the scene the item is in |
| sceneItemId | Number | Numeric ID of the scene item |

---

### SceneItemTransformChanged

The transform/crop of a scene item has changed.

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneName | String | The name of the scene the item is in |
| sceneUuid | String | The UUID of the scene the item is in |
| sceneItemId | Number | Numeric ID of the scene item |
| sceneItemTransform | Object | New transform/crop info of the scene item |

## Outputs Events

### StreamStateChanged

The state of the stream output has changed.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| outputActive | Boolean | Whether the output is active |
| outputState | String | The specific state of the output |

---

### RecordStateChanged

The state of the record output has changed.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| outputActive | Boolean | Whether the output is active |
| outputState | String | The specific state of the output |
| outputPath | String | File name for the saved recording, if record stopped. `null` otherwise |

---

### RecordFileChanged

The record output has started writing to a new file. For example, when a file split happens.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.5.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| newOutputPath | String | File name that the output has begun writing to |

---

### ReplayBufferStateChanged

The state of the replay buffer output has changed.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| outputActive | Boolean | Whether the output is active |
| outputState | String | The specific state of the output |

---

### VirtualcamStateChanged

The state of the virtualcam output has changed.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| outputActive | Boolean | Whether the output is active |
| outputState | String | The specific state of the output |

---

### ReplayBufferSaved

The replay buffer has been saved.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| savedReplayPath | String | Path of the saved replay file |

## Media Inputs Events

### MediaInputPlaybackStarted

A media input has started playing.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputName | String | Name of the input |
| inputUuid | String | UUID of the input |

---

### MediaInputPlaybackEnded

A media input has finished playing.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputName | String | Name of the input |
| inputUuid | String | UUID of the input |

---

### MediaInputActionTriggered

An action has been performed on an input.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputName | String | Name of the input |
| inputUuid | String | UUID of the input |
| mediaAction | String | Action performed on the input. See `ObsMediaInputAction` enum |

## Ui Events

### StudioModeStateChanged

Studio mode has been enabled or disabled.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| studioModeEnabled | Boolean | True == Enabled, False == Disabled |

---

### ScreenshotSaved

A screenshot has been saved.

Note: Triggered for the screenshot feature available in `Settings -> Hotkeys -> Screenshot Output` ONLY.
Applications using `Get/SaveSourceScreenshot` should implement a `CustomEvent` if this kind of inter-client
communication is desired.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.1.0

**Data Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| savedScreenshotPath | String | Path of the saved image file |

# Requests

## Requests Table of Contents

- [General Requests](#general-1-requests)
  - [GetVersion](#getversion)
  - [GetStats](#getstats)
  - [BroadcastCustomEvent](#broadcastcustomevent)
  - [CallVendorRequest](#callvendorrequest)
  - [GetHotkeyList](#gethotkeylist)
  - [TriggerHotkeyByName](#triggerhotkeybyname)
  - [TriggerHotkeyByKeySequence](#triggerhotkeybykeysequence)
  - [Sleep](#sleep)
- [Config Requests](#config-1-requests)
  - [GetPersistentData](#getpersistentdata)
  - [SetPersistentData](#setpersistentdata)
  - [GetSceneCollectionList](#getscenecollectionlist)
  - [SetCurrentSceneCollection](#setcurrentscenecollection)
  - [CreateSceneCollection](#createscenecollection)
  - [GetProfileList](#getprofilelist)
  - [SetCurrentProfile](#setcurrentprofile)
  - [CreateProfile](#createprofile)
  - [RemoveProfile](#removeprofile)
  - [GetProfileParameter](#getprofileparameter)
  - [SetProfileParameter](#setprofileparameter)
  - [GetVideoSettings](#getvideosettings)
  - [SetVideoSettings](#setvideosettings)
  - [GetStreamServiceSettings](#getstreamservicesettings)
  - [SetStreamServiceSettings](#setstreamservicesettings)
  - [GetRecordDirectory](#getrecorddirectory)
  - [SetRecordDirectory](#setrecorddirectory)
- [Sources Requests](#sources-requests)
  - [GetSourceActive](#getsourceactive)
  - [GetSourceScreenshot](#getsourcescreenshot)
  - [SaveSourceScreenshot](#savesourcescreenshot)
- [Scenes Requests](#scenes-1-requests)
  - [GetSceneList](#getscenelist)
  - [GetGroupList](#getgrouplist)
  - [GetCurrentProgramScene](#getcurrentprogramscene)
  - [SetCurrentProgramScene](#setcurrentprogramscene)
  - [GetCurrentPreviewScene](#getcurrentpreviewscene)
  - [SetCurrentPreviewScene](#setcurrentpreviewscene)
  - [CreateScene](#createscene)
  - [RemoveScene](#removescene)
  - [SetSceneName](#setscenename)
  - [GetSceneSceneTransitionOverride](#getscenescenetransitionoverride)
  - [SetSceneSceneTransitionOverride](#setscenescenetransitionoverride)
- [Inputs Requests](#inputs-1-requests)
  - [GetInputList](#getinputlist)
  - [GetInputKindList](#getinputkindlist)
  - [GetSpecialInputs](#getspecialinputs)
  - [CreateInput](#createinput)
  - [RemoveInput](#removeinput)
  - [SetInputName](#setinputname)
  - [GetInputDefaultSettings](#getinputdefaultsettings)
  - [GetInputSettings](#getinputsettings)
  - [SetInputSettings](#setinputsettings)
  - [GetInputMute](#getinputmute)
  - [SetInputMute](#setinputmute)
  - [ToggleInputMute](#toggleinputmute)
  - [GetInputVolume](#getinputvolume)
  - [SetInputVolume](#setinputvolume)
  - [GetInputAudioBalance](#getinputaudiobalance)
  - [SetInputAudioBalance](#setinputaudiobalance)
  - [GetInputAudioSyncOffset](#getinputaudiosyncoffset)
  - [SetInputAudioSyncOffset](#setinputaudiosyncoffset)
  - [GetInputAudioMonitorType](#getinputaudiomonitortype)
  - [SetInputAudioMonitorType](#setinputaudiomonitortype)
  - [GetInputAudioTracks](#getinputaudiotracks)
  - [SetInputAudioTracks](#setinputaudiotracks)
  - [GetInputDeinterlaceMode](#getinputdeinterlacemode)
  - [SetInputDeinterlaceMode](#setinputdeinterlacemode)
  - [GetInputDeinterlaceFieldOrder](#getinputdeinterlacefieldorder)
  - [SetInputDeinterlaceFieldOrder](#setinputdeinterlacefieldorder)
  - [GetInputPropertiesListPropertyItems](#getinputpropertieslistpropertyitems)
  - [PressInputPropertiesButton](#pressinputpropertiesbutton)
- [Transitions Requests](#transitions-1-requests)
  - [GetTransitionKindList](#gettransitionkindlist)
  - [GetSceneTransitionList](#getscenetransitionlist)
  - [GetCurrentSceneTransition](#getcurrentscenetransition)
  - [SetCurrentSceneTransition](#setcurrentscenetransition)
  - [SetCurrentSceneTransitionDuration](#setcurrentscenetransitionduration)
  - [SetCurrentSceneTransitionSettings](#setcurrentscenetransitionsettings)
  - [GetCurrentSceneTransitionCursor](#getcurrentscenetransitioncursor)
  - [TriggerStudioModeTransition](#triggerstudiomodetransition)
  - [SetTBarPosition](#settbarposition)
- [Filters Requests](#filters-1-requests)
  - [GetSourceFilterKindList](#getsourcefilterkindlist)
  - [GetSourceFilterList](#getsourcefilterlist)
  - [GetSourceFilterDefaultSettings](#getsourcefilterdefaultsettings)
  - [CreateSourceFilter](#createsourcefilter)
  - [RemoveSourceFilter](#removesourcefilter)
  - [SetSourceFilterName](#setsourcefiltername)
  - [GetSourceFilter](#getsourcefilter)
  - [SetSourceFilterIndex](#setsourcefilterindex)
  - [SetSourceFilterSettings](#setsourcefiltersettings)
  - [SetSourceFilterEnabled](#setsourcefilterenabled)
- [Scene Items Requests](#scene-items-1-requests)
  - [GetSceneItemList](#getsceneitemlist)
  - [GetGroupSceneItemList](#getgroupsceneitemlist)
  - [GetSceneItemId](#getsceneitemid)
  - [GetSceneItemSource](#getsceneitemsource)
  - [CreateSceneItem](#createsceneitem)
  - [RemoveSceneItem](#removesceneitem)
  - [DuplicateSceneItem](#duplicatesceneitem)
  - [GetSceneItemTransform](#getsceneitemtransform)
  - [SetSceneItemTransform](#setsceneitemtransform)
  - [GetSceneItemEnabled](#getsceneitemenabled)
  - [SetSceneItemEnabled](#setsceneitemenabled)
  - [GetSceneItemLocked](#getsceneitemlocked)
  - [SetSceneItemLocked](#setsceneitemlocked)
  - [GetSceneItemIndex](#getsceneitemindex)
  - [SetSceneItemIndex](#setsceneitemindex)
  - [GetSceneItemBlendMode](#getsceneitemblendmode)
  - [SetSceneItemBlendMode](#setsceneitemblendmode)
- [Outputs Requests](#outputs-1-requests)
  - [GetVirtualCamStatus](#getvirtualcamstatus)
  - [ToggleVirtualCam](#togglevirtualcam)
  - [StartVirtualCam](#startvirtualcam)
  - [StopVirtualCam](#stopvirtualcam)
  - [GetReplayBufferStatus](#getreplaybufferstatus)
  - [ToggleReplayBuffer](#togglereplaybuffer)
  - [StartReplayBuffer](#startreplaybuffer)
  - [StopReplayBuffer](#stopreplaybuffer)
  - [SaveReplayBuffer](#savereplaybuffer)
  - [GetLastReplayBufferReplay](#getlastreplaybufferreplay)
  - [GetOutputList](#getoutputlist)
  - [GetOutputStatus](#getoutputstatus)
  - [ToggleOutput](#toggleoutput)
  - [StartOutput](#startoutput)
  - [StopOutput](#stopoutput)
  - [GetOutputSettings](#getoutputsettings)
  - [SetOutputSettings](#setoutputsettings)
- [Stream Requests](#stream-requests)
  - [GetStreamStatus](#getstreamstatus)
  - [ToggleStream](#togglestream)
  - [StartStream](#startstream)
  - [StopStream](#stopstream)
  - [SendStreamCaption](#sendstreamcaption)
- [Record Requests](#record-requests)
  - [GetRecordStatus](#getrecordstatus)
  - [ToggleRecord](#togglerecord)
  - [StartRecord](#startrecord)
  - [StopRecord](#stoprecord)
  - [ToggleRecordPause](#togglerecordpause)
  - [PauseRecord](#pauserecord)
  - [ResumeRecord](#resumerecord)
  - [SplitRecordFile](#splitrecordfile)
  - [CreateRecordChapter](#createrecordchapter)
- [Media Inputs Requests](#media-inputs-1-requests)
  - [GetMediaInputStatus](#getmediainputstatus)
  - [SetMediaInputCursor](#setmediainputcursor)
  - [OffsetMediaInputCursor](#offsetmediainputcursor)
  - [TriggerMediaInputAction](#triggermediainputaction)
- [Ui Requests](#ui-1-requests)
  - [GetStudioModeEnabled](#getstudiomodeenabled)
  - [SetStudioModeEnabled](#setstudiomodeenabled)
  - [OpenInputPropertiesDialog](#openinputpropertiesdialog)
  - [OpenInputFiltersDialog](#openinputfiltersdialog)
  - [OpenInputInteractDialog](#openinputinteractdialog)
  - [GetMonitorList](#getmonitorlist)
  - [OpenVideoMixProjector](#openvideomixprojector)
  - [OpenSourceProjector](#opensourceprojector)

## General Requests

### GetVersion

Gets data about the current plugin and RPC version.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| obsVersion | String | Current OBS Studio version |
| obsWebSocketVersion | String | Current obs-websocket version |
| rpcVersion | Number | Current latest obs-websocket RPC version |
| availableRequests | Array&lt;String&gt; | Array of available RPC requests for the currently negotiated RPC version |
| supportedImageFormats | Array&lt;String&gt; | Image formats available in `GetSourceScreenshot` and `SaveSourceScreenshot` requests. |
| platform | String | Name of the platform. Usually `windows`, `macos`, or `ubuntu` (linux flavor). Not guaranteed to be any of those |
| platformDescription | String | Description of the platform, like `Windows 10 (10.0)` |

---

### GetStats

Gets statistics about OBS, obs-websocket, and the current session.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| cpuUsage | Number | Current CPU usage in percent |
| memoryUsage | Number | Amount of memory in MB currently being used by OBS |
| availableDiskSpace | Number | Available disk space on the device being used for recording storage |
| activeFps | Number | Current FPS being rendered |
| averageFrameRenderTime | Number | Average time in milliseconds that OBS is taking to render a frame |
| renderSkippedFrames | Number | Number of frames skipped by OBS in the render thread |
| renderTotalFrames | Number | Total number of frames outputted by the render thread |
| outputSkippedFrames | Number | Number of frames skipped by OBS in the output thread |
| outputTotalFrames | Number | Total number of frames outputted by the output thread |
| webSocketSessionIncomingMessages | Number | Total number of messages received by obs-websocket from the client |
| webSocketSessionOutgoingMessages | Number | Total number of messages sent by obs-websocket to the client |

---

### BroadcastCustomEvent

Broadcasts a `CustomEvent` to all WebSocket clients. Receivers are clients which are identified and subscribed.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| eventData | Object | Data payload to emit to all receivers | None | N/A |

---

### CallVendorRequest

Call a request registered to a vendor.

A vendor is a unique name registered by a third-party plugin or script, which allows for custom requests and events to be added to obs-websocket.
If a plugin or script implements vendor requests or events, documentation is expected to be provided with them.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| vendorName | String | Name of the vendor to use | None | N/A |
| requestType | String | The request type to call | None | N/A |
| ?requestData | Object | Object containing appropriate request data | None | {} |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| vendorName | String | Echoed of `vendorName` |
| requestType | String | Echoed of `requestType` |
| responseData | Object | Object containing appropriate response data. {} if request does not provide any response data |

---

### GetHotkeyList

Gets an array of all hotkey names in OBS.

Note: Hotkey functionality in obs-websocket comes as-is, and we do not guarantee support if things are broken. In 9/10 usages of hotkey requests, there exists a better, more reliable method via other requests.

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| hotkeys | Array&lt;String&gt; | Array of hotkey names |

---

### TriggerHotkeyByName

Triggers a hotkey using its name. See `GetHotkeyList`.

Note: Hotkey functionality in obs-websocket comes as-is, and we do not guarantee support if things are broken. In 9/10 usages of hotkey requests, there exists a better, more reliable method via other requests.

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| hotkeyName | String | Name of the hotkey to trigger | None | N/A |
| ?contextName | String | Name of context of the hotkey to trigger | None | Unknown |

---

### TriggerHotkeyByKeySequence

Triggers a hotkey using a sequence of keys.

Note: Hotkey functionality in obs-websocket comes as-is, and we do not guarantee support if things are broken. In 9/10 usages of hotkey requests, there exists a better, more reliable method via other requests.

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?keyId | String | The OBS key ID to use. See https://github.com/obsproject/obs-studio/blob/master/libobs/obs-hotkeys.h | None | Not pressed |
| ?keyModifiers | Object | Object containing key modifiers to apply | None | Ignored |
| ?keyModifiers.shift | Boolean | Press Shift | None | Not pressed |
| ?keyModifiers.control | Boolean | Press CTRL | None | Not pressed |
| ?keyModifiers.alt | Boolean | Press ALT | None | Not pressed |
| ?keyModifiers.command | Boolean | Press CMD (Mac) | None | Not pressed |

---

### Sleep

Sleeps for a time duration or number of frames. Only available in request batches with types `SERIAL_REALTIME` or `SERIAL_FRAME`.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sleepMillis | Number | Number of milliseconds to sleep for (if `SERIAL_REALTIME` mode) | >= 0, <= 50000 | Unknown |
| ?sleepFrames | Number | Number of frames to sleep for (if `SERIAL_FRAME` mode) | >= 0, <= 10000 | Unknown |

## Config Requests

### GetPersistentData

Gets the value of a "slot" from the selected persistent data realm.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| realm | String | The data realm to select. `OBS_WEBSOCKET_DATA_REALM_GLOBAL` or `OBS_WEBSOCKET_DATA_REALM_PROFILE` | None | N/A |
| slotName | String | The name of the slot to retrieve data from | None | N/A |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| slotValue | Any | Value associated with the slot. `null` if not set |

---

### SetPersistentData

Sets the value of a "slot" from the selected persistent data realm.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| realm | String | The data realm to select. `OBS_WEBSOCKET_DATA_REALM_GLOBAL` or `OBS_WEBSOCKET_DATA_REALM_PROFILE` | None | N/A |
| slotName | String | The name of the slot to retrieve data from | None | N/A |
| slotValue | Any | The value to apply to the slot | None | N/A |

---

### GetSceneCollectionList

Gets an array of all scene collections

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| currentSceneCollectionName | String | The name of the current scene collection |
| sceneCollections | Array&lt;String&gt; | Array of all available scene collections |

---

### SetCurrentSceneCollection

Switches to a scene collection.

Note: This will block until the collection has finished changing.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| sceneCollectionName | String | Name of the scene collection to switch to | None | N/A |

---

### CreateSceneCollection

Creates a new scene collection, switching to it in the process.

Note: This will block until the collection has finished changing.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| sceneCollectionName | String | Name for the new scene collection | None | N/A |

---

### GetProfileList

Gets an array of all profiles

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| currentProfileName | String | The name of the current profile |
| profiles | Array&lt;String&gt; | Array of all available profiles |

---

### SetCurrentProfile

Switches to a profile.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| profileName | String | Name of the profile to switch to | None | N/A |

---

### CreateProfile

Creates a new profile, switching to it in the process

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| profileName | String | Name for the new profile | None | N/A |

---

### RemoveProfile

Removes a profile. If the current profile is chosen, it will change to a different profile first.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| profileName | String | Name of the profile to remove | None | N/A |

---

### GetProfileParameter

Gets a parameter from the current profile's configuration.

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| parameterCategory | String | Category of the parameter to get | None | N/A |
| parameterName | String | Name of the parameter to get | None | N/A |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| parameterValue | String | Value associated with the parameter. `null` if not set and no default |
| defaultParameterValue | String | Default value associated with the parameter. `null` if no default |

---

### SetProfileParameter

Sets the value of a parameter in the current profile's configuration.

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| parameterCategory | String | Category of the parameter to set | None | N/A |
| parameterName | String | Name of the parameter to set | None | N/A |
| parameterValue | String | Value of the parameter to set. Use `null` to delete | None | N/A |

---

### GetVideoSettings

Gets the current video settings.

Note: To get the true FPS value, divide the FPS numerator by the FPS denominator. Example: `60000/1001`

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| fpsNumerator | Number | Numerator of the fractional FPS value |
| fpsDenominator | Number | Denominator of the fractional FPS value |
| baseWidth | Number | Width of the base (canvas) resolution in pixels |
| baseHeight | Number | Height of the base (canvas) resolution in pixels |
| outputWidth | Number | Width of the output resolution in pixels |
| outputHeight | Number | Height of the output resolution in pixels |

---

### SetVideoSettings

Sets the current video settings.

Note: Fields must be specified in pairs. For example, you cannot set only `baseWidth` without needing to specify `baseHeight`.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?fpsNumerator | Number | Numerator of the fractional FPS value | >= 1 | Not changed |
| ?fpsDenominator | Number | Denominator of the fractional FPS value | >= 1 | Not changed |
| ?baseWidth | Number | Width of the base (canvas) resolution in pixels | >= 1, <= 4096 | Not changed |
| ?baseHeight | Number | Height of the base (canvas) resolution in pixels | >= 1, <= 4096 | Not changed |
| ?outputWidth | Number | Width of the output resolution in pixels | >= 1, <= 4096 | Not changed |
| ?outputHeight | Number | Height of the output resolution in pixels | >= 1, <= 4096 | Not changed |

---

### GetStreamServiceSettings

Gets the current stream service settings (stream destination).

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| streamServiceType | String | Stream service type, like `rtmp_custom` or `rtmp_common` |
| streamServiceSettings | Object | Stream service settings |

---

### SetStreamServiceSettings

Sets the current stream service settings (stream destination).

Note: Simple RTMP settings can be set with type `rtmp_custom` and the settings fields `server` and `key`.

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| streamServiceType | String | Type of stream service to apply. Example: `rtmp_common` or `rtmp_custom` | None | N/A |
| streamServiceSettings | Object | Settings to apply to the service | None | N/A |

---

### GetRecordDirectory

Gets the current directory that the record output is set to.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| recordDirectory | String | Output directory |

---

### SetRecordDirectory

Sets the current directory that the record output writes files to.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.3.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| recordDirectory | String | Output directory | None | N/A |

## Sources Requests

### GetSourceActive

Gets the active and show state of a source.

**Compatible with inputs and scenes.**

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sourceName | String | Name of the source to get the active state of | None | Unknown |
| ?sourceUuid | String | UUID of the source to get the active state of | None | Unknown |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| videoActive | Boolean | Whether the source is showing in Program |
| videoShowing | Boolean | Whether the source is showing in the UI (Preview, Projector, Properties) |

---

### GetSourceScreenshot

Gets a Base64-encoded screenshot of a source.

The `imageWidth` and `imageHeight` parameters are treated as "scale to inner", meaning the smallest ratio will be used and the aspect ratio of the original resolution is kept.
If `imageWidth` and `imageHeight` are not specified, the compressed image will use the full resolution of the source.

**Compatible with inputs and scenes.**

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sourceName | String | Name of the source to take a screenshot of | None | Unknown |
| ?sourceUuid | String | UUID of the source to take a screenshot of | None | Unknown |
| imageFormat | String | Image compression format to use. Use `GetVersion` to get compatible image formats | None | N/A |
| ?imageWidth | Number | Width to scale the screenshot to | >= 8, <= 4096 | Source value is used |
| ?imageHeight | Number | Height to scale the screenshot to | >= 8, <= 4096 | Source value is used |
| ?imageCompressionQuality | Number | Compression quality to use. 0 for high compression, 100 for uncompressed. -1 to use "default" (whatever that means, idk) | >= -1, <= 100 | -1 |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| imageData | String | Base64-encoded screenshot |

---

### SaveSourceScreenshot

Saves a screenshot of a source to the filesystem.

The `imageWidth` and `imageHeight` parameters are treated as "scale to inner", meaning the smallest ratio will be used and the aspect ratio of the original resolution is kept.
If `imageWidth` and `imageHeight` are not specified, the compressed image will use the full resolution of the source.

**Compatible with inputs and scenes.**

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sourceName | String | Name of the source to take a screenshot of | None | Unknown |
| ?sourceUuid | String | UUID of the source to take a screenshot of | None | Unknown |
| imageFormat | String | Image compression format to use. Use `GetVersion` to get compatible image formats | None | N/A |
| imageFilePath | String | Path to save the screenshot file to. Eg. `C:\Users\user\Desktop\screenshot.png` | None | N/A |
| ?imageWidth | Number | Width to scale the screenshot to | >= 8, <= 4096 | Source value is used |
| ?imageHeight | Number | Height to scale the screenshot to | >= 8, <= 4096 | Source value is used |
| ?imageCompressionQuality | Number | Compression quality to use. 0 for high compression, 100 for uncompressed. -1 to use "default" (whatever that means, idk) | >= -1, <= 100 | -1 |

## Scenes Requests

### GetSceneList

Gets an array of all scenes in OBS.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| currentProgramSceneName | String | Current program scene name. Can be `null` if internal state desync |
| currentProgramSceneUuid | String | Current program scene UUID. Can be `null` if internal state desync |
| currentPreviewSceneName | String | Current preview scene name. `null` if not in studio mode |
| currentPreviewSceneUuid | String | Current preview scene UUID. `null` if not in studio mode |
| scenes | Array&lt;Object&gt; | Array of scenes |

---

### GetGroupList

Gets an array of all groups in OBS.

Groups in OBS are actually scenes, but renamed and modified. In obs-websocket, we treat them as scenes where we can.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| groups | Array&lt;String&gt; | Array of group names |

---

### GetCurrentProgramScene

Gets the current program scene.

Note: This request is slated to have the `currentProgram`-prefixed fields removed from in an upcoming RPC version.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneName | String | Current program scene name |
| sceneUuid | String | Current program scene UUID |
| currentProgramSceneName | String | Current program scene name (Deprecated) |
| currentProgramSceneUuid | String | Current program scene UUID (Deprecated) |

---

### SetCurrentProgramScene

Sets the current program scene.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Scene name to set as the current program scene | None | Unknown |
| ?sceneUuid | String | Scene UUID to set as the current program scene | None | Unknown |

---

### GetCurrentPreviewScene

Gets the current preview scene.

Only available when studio mode is enabled.

Note: This request is slated to have the `currentPreview`-prefixed fields removed from in an upcoming RPC version.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneName | String | Current preview scene name |
| sceneUuid | String | Current preview scene UUID |
| currentPreviewSceneName | String | Current preview scene name |
| currentPreviewSceneUuid | String | Current preview scene UUID |

---

### SetCurrentPreviewScene

Sets the current preview scene.

Only available when studio mode is enabled.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Scene name to set as the current preview scene | None | Unknown |
| ?sceneUuid | String | Scene UUID to set as the current preview scene | None | Unknown |

---

### CreateScene

Creates a new scene in OBS.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| sceneName | String | Name for the new scene | None | N/A |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneUuid | String | UUID of the created scene |

---

### RemoveScene

Removes a scene from OBS.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene to remove | None | Unknown |
| ?sceneUuid | String | UUID of the scene to remove | None | Unknown |

---

### SetSceneName

Sets the name of a scene (rename).

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene to be renamed | None | Unknown |
| ?sceneUuid | String | UUID of the scene to be renamed | None | Unknown |
| newSceneName | String | New name for the scene | None | N/A |

---

### GetSceneSceneTransitionOverride

Gets the scene transition overridden for a scene.

Note: A transition UUID response field is not currently able to be implemented as of 2024-1-18.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene | None | Unknown |
| ?sceneUuid | String | UUID of the scene | None | Unknown |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| transitionName | String | Name of the overridden scene transition, else `null` |
| transitionDuration | Number | Duration of the overridden scene transition, else `null` |

---

### SetSceneSceneTransitionOverride

Sets the scene transition overridden for a scene.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene | None | Unknown |
| ?sceneUuid | String | UUID of the scene | None | Unknown |
| ?transitionName | String | Name of the scene transition to use as override. Specify `null` to remove | None | Unchanged |
| ?transitionDuration | Number | Duration to use for any overridden transition. Specify `null` to remove | >= 50, <= 20000 | Unchanged |

## Inputs Requests

### GetInputList

Gets an array of all inputs in OBS.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputKind | String | Restrict the array to only inputs of the specified kind | None | All kinds included |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputs | Array&lt;Object&gt; | Array of inputs |

---

### GetInputKindList

Gets an array of all available input kinds in OBS.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?unversioned | Boolean | True == Return all kinds as unversioned, False == Return with version suffixes (if available) | None | false |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputKinds | Array&lt;String&gt; | Array of input kinds |

---

### GetSpecialInputs

Gets the names of all special inputs.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| desktop1 | String | Name of the Desktop Audio input |
| desktop2 | String | Name of the Desktop Audio 2 input |
| mic1 | String | Name of the Mic/Auxiliary Audio input |
| mic2 | String | Name of the Mic/Auxiliary Audio 2 input |
| mic3 | String | Name of the Mic/Auxiliary Audio 3 input |
| mic4 | String | Name of the Mic/Auxiliary Audio 4 input |

---

### CreateInput

Creates a new input, adding it as a scene item to the specified scene.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene to add the input to as a scene item | None | Unknown |
| ?sceneUuid | String | UUID of the scene to add the input to as a scene item | None | Unknown |
| inputName | String | Name of the new input to created | None | N/A |
| inputKind | String | The kind of input to be created | None | N/A |
| ?inputSettings | Object | Settings object to initialize the input with | None | Default settings used |
| ?sceneItemEnabled | Boolean | Whether to set the created scene item to enabled or disabled | None | True |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputUuid | String | UUID of the newly created input |
| sceneItemId | Number | ID of the newly created scene item |

---

### RemoveInput

Removes an existing input.

Note: Will immediately remove all associated scene items.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input to remove | None | Unknown |
| ?inputUuid | String | UUID of the input to remove | None | Unknown |

---

### SetInputName

Sets the name of an input (rename).

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Current input name | None | Unknown |
| ?inputUuid | String | Current input UUID | None | Unknown |
| newInputName | String | New name for the input | None | N/A |

---

### GetInputDefaultSettings

Gets the default settings for an input kind.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| inputKind | String | Input kind to get the default settings for | None | N/A |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| defaultInputSettings | Object | Object of default settings for the input kind |

---

### GetInputSettings

Gets the settings of an input.

Note: Does not include defaults. To create the entire settings object, overlay `inputSettings` over the `defaultInputSettings` provided by `GetInputDefaultSettings`.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input to get the settings of | None | Unknown |
| ?inputUuid | String | UUID of the input to get the settings of | None | Unknown |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputSettings | Object | Object of settings for the input |
| inputKind | String | The kind of the input |

---

### SetInputSettings

Sets the settings of an input.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input to set the settings of | None | Unknown |
| ?inputUuid | String | UUID of the input to set the settings of | None | Unknown |
| inputSettings | Object | Object of settings to apply | None | N/A |
| ?overlay | Boolean | True == apply the settings on top of existing ones, False == reset the input to its defaults, then apply settings. | None | true |

---

### GetInputMute

Gets the audio mute state of an input.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of input to get the mute state of | None | Unknown |
| ?inputUuid | String | UUID of input to get the mute state of | None | Unknown |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputMuted | Boolean | Whether the input is muted |

---

### SetInputMute

Sets the audio mute state of an input.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input to set the mute state of | None | Unknown |
| ?inputUuid | String | UUID of the input to set the mute state of | None | Unknown |
| inputMuted | Boolean | Whether to mute the input or not | None | N/A |

---

### ToggleInputMute

Toggles the audio mute state of an input.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input to toggle the mute state of | None | Unknown |
| ?inputUuid | String | UUID of the input to toggle the mute state of | None | Unknown |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputMuted | Boolean | Whether the input has been muted or unmuted |

---

### GetInputVolume

Gets the current volume setting of an input.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input to get the volume of | None | Unknown |
| ?inputUuid | String | UUID of the input to get the volume of | None | Unknown |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputVolumeMul | Number | Volume setting in mul |
| inputVolumeDb | Number | Volume setting in dB |

---

### SetInputVolume

Sets the volume setting of an input.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input to set the volume of | None | Unknown |
| ?inputUuid | String | UUID of the input to set the volume of | None | Unknown |
| ?inputVolumeMul | Number | Volume setting in mul | >= 0, <= 20 | `inputVolumeDb` should be specified |
| ?inputVolumeDb | Number | Volume setting in dB | >= -100, <= 26 | `inputVolumeMul` should be specified |

---

### GetInputAudioBalance

Gets the audio balance of an input.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input to get the audio balance of | None | Unknown |
| ?inputUuid | String | UUID of the input to get the audio balance of | None | Unknown |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputAudioBalance | Number | Audio balance value from 0.0-1.0 |

---

### SetInputAudioBalance

Sets the audio balance of an input.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input to set the audio balance of | None | Unknown |
| ?inputUuid | String | UUID of the input to set the audio balance of | None | Unknown |
| inputAudioBalance | Number | New audio balance value | >= 0.0, <= 1.0 | N/A |

---

### GetInputAudioSyncOffset

Gets the audio sync offset of an input.

Note: The audio sync offset can be negative too!

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input to get the audio sync offset of | None | Unknown |
| ?inputUuid | String | UUID of the input to get the audio sync offset of | None | Unknown |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputAudioSyncOffset | Number | Audio sync offset in milliseconds |

---

### SetInputAudioSyncOffset

Sets the audio sync offset of an input.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input to set the audio sync offset of | None | Unknown |
| ?inputUuid | String | UUID of the input to set the audio sync offset of | None | Unknown |
| inputAudioSyncOffset | Number | New audio sync offset in milliseconds | >= -950, <= 20000 | N/A |

---

### GetInputAudioMonitorType

Gets the audio monitor type of an input.

The available audio monitor types are:

- `OBS_MONITORING_TYPE_NONE`
- `OBS_MONITORING_TYPE_MONITOR_ONLY`
- `OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT`

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input to get the audio monitor type of | None | Unknown |
| ?inputUuid | String | UUID of the input to get the audio monitor type of | None | Unknown |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| monitorType | String | Audio monitor type |

---

### SetInputAudioMonitorType

Sets the audio monitor type of an input.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input to set the audio monitor type of | None | Unknown |
| ?inputUuid | String | UUID of the input to set the audio monitor type of | None | Unknown |
| monitorType | String | Audio monitor type | None | N/A |

---

### GetInputAudioTracks

Gets the enable state of all audio tracks of an input.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input | None | Unknown |
| ?inputUuid | String | UUID of the input | None | Unknown |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputAudioTracks | Object | Object of audio tracks and associated enable states |

---

### SetInputAudioTracks

Sets the enable state of audio tracks of an input.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input | None | Unknown |
| ?inputUuid | String | UUID of the input | None | Unknown |
| inputAudioTracks | Object | Track settings to apply | None | N/A |

---

### GetInputDeinterlaceMode

Gets the deinterlace mode of an input.

Deinterlace Modes:

- `OBS_DEINTERLACE_MODE_DISABLE`
- `OBS_DEINTERLACE_MODE_DISCARD`
- `OBS_DEINTERLACE_MODE_RETRO`
- `OBS_DEINTERLACE_MODE_BLEND`
- `OBS_DEINTERLACE_MODE_BLEND_2X`
- `OBS_DEINTERLACE_MODE_LINEAR`
- `OBS_DEINTERLACE_MODE_LINEAR_2X`
- `OBS_DEINTERLACE_MODE_YADIF`
- `OBS_DEINTERLACE_MODE_YADIF_2X`

Note: Deinterlacing functionality is restricted to async inputs only.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.6.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input | None | Unknown |
| ?inputUuid | String | UUID of the input | None | Unknown |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputDeinterlaceMode | String | Deinterlace mode of the input |

---

### SetInputDeinterlaceMode

Sets the deinterlace mode of an input.

Note: Deinterlacing functionality is restricted to async inputs only.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.6.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input | None | Unknown |
| ?inputUuid | String | UUID of the input | None | Unknown |
| inputDeinterlaceMode | String | Deinterlace mode for the input | None | N/A |

---

### GetInputDeinterlaceFieldOrder

Gets the deinterlace field order of an input.

Deinterlace Field Orders:

- `OBS_DEINTERLACE_FIELD_ORDER_TOP`
- `OBS_DEINTERLACE_FIELD_ORDER_BOTTOM`

Note: Deinterlacing functionality is restricted to async inputs only.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.6.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input | None | Unknown |
| ?inputUuid | String | UUID of the input | None | Unknown |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| inputDeinterlaceFieldOrder | String | Deinterlace field order of the input |

---

### SetInputDeinterlaceFieldOrder

Sets the deinterlace field order of an input.

Note: Deinterlacing functionality is restricted to async inputs only.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.6.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input | None | Unknown |
| ?inputUuid | String | UUID of the input | None | Unknown |
| inputDeinterlaceFieldOrder | String | Deinterlace field order for the input | None | N/A |

---

### GetInputPropertiesListPropertyItems

Gets the items of a list property from an input's properties.

Note: Use this in cases where an input provides a dynamic, selectable list of items. For example, display capture, where it provides a list of available displays.

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input | None | Unknown |
| ?inputUuid | String | UUID of the input | None | Unknown |
| propertyName | String | Name of the list property to get the items of | None | N/A |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| propertyItems | Array&lt;Object&gt; | Array of items in the list property |

---

### PressInputPropertiesButton

Presses a button in the properties of an input.

Some known `propertyName` values are:

- `refreshnocache` - Browser source reload button

Note: Use this in cases where there is a button in the properties of an input that cannot be accessed in any other way. For example, browser sources, where there is a refresh button.

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input | None | Unknown |
| ?inputUuid | String | UUID of the input | None | Unknown |
| propertyName | String | Name of the button property to press | None | N/A |

## Transitions Requests

### GetTransitionKindList

Gets an array of all available transition kinds.

Similar to `GetInputKindList`

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| transitionKinds | Array&lt;String&gt; | Array of transition kinds |

---

### GetSceneTransitionList

Gets an array of all scene transitions in OBS.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| currentSceneTransitionName | String | Name of the current scene transition. Can be null |
| currentSceneTransitionUuid | String | UUID of the current scene transition. Can be null |
| currentSceneTransitionKind | String | Kind of the current scene transition. Can be null |
| transitions | Array&lt;Object&gt; | Array of transitions |

---

### GetCurrentSceneTransition

Gets information about the current scene transition.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| transitionName | String | Name of the transition |
| transitionUuid | String | UUID of the transition |
| transitionKind | String | Kind of the transition |
| transitionFixed | Boolean | Whether the transition uses a fixed (unconfigurable) duration |
| transitionDuration | Number | Configured transition duration in milliseconds. `null` if transition is fixed |
| transitionConfigurable | Boolean | Whether the transition supports being configured |
| transitionSettings | Object | Object of settings for the transition. `null` if transition is not configurable |

---

### SetCurrentSceneTransition

Sets the current scene transition.

Small note: While the namespace of scene transitions is generally unique, that uniqueness is not a guarantee as it is with other resources like inputs.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| transitionName | String | Name of the transition to make active | None | N/A |

---

### SetCurrentSceneTransitionDuration

Sets the duration of the current scene transition, if it is not fixed.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| transitionDuration | Number | Duration in milliseconds | >= 50, <= 20000 | N/A |

---

### SetCurrentSceneTransitionSettings

Sets the settings of the current scene transition.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| transitionSettings | Object | Settings object to apply to the transition. Can be `{}` | None | N/A |
| ?overlay | Boolean | Whether to overlay over the current settings or replace them | None | true |

---

### GetCurrentSceneTransitionCursor

Gets the cursor position of the current scene transition.

Note: `transitionCursor` will return 1.0 when the transition is inactive.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| transitionCursor | Number | Cursor position, between 0.0 and 1.0 |

---

### TriggerStudioModeTransition

Triggers the current scene transition. Same functionality as the `Transition` button in studio mode.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### SetTBarPosition

Sets the position of the TBar.

**Very important note**: This will be deprecated and replaced in a future version of obs-websocket.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| position | Number | New position | >= 0.0, <= 1.0 | N/A |
| ?release | Boolean | Whether to release the TBar. Only set `false` if you know that you will be sending another position update | None | `true` |

## Filters Requests

### GetSourceFilterKindList

Gets an array of all available source filter kinds.

Similar to `GetInputKindList`

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.4.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sourceFilterKinds | Array&lt;String&gt; | Array of source filter kinds |

---

### GetSourceFilterList

Gets an array of all of a source's filters.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sourceName | String | Name of the source | None | Unknown |
| ?sourceUuid | String | UUID of the source | None | Unknown |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| filters | Array&lt;Object&gt; | Array of filters |

---

### GetSourceFilterDefaultSettings

Gets the default settings for a filter kind.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| filterKind | String | Filter kind to get the default settings for | None | N/A |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| defaultFilterSettings | Object | Object of default settings for the filter kind |

---

### CreateSourceFilter

Creates a new filter, adding it to the specified source.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sourceName | String | Name of the source to add the filter to | None | Unknown |
| ?sourceUuid | String | UUID of the source to add the filter to | None | Unknown |
| filterName | String | Name of the new filter to be created | None | N/A |
| filterKind | String | The kind of filter to be created | None | N/A |
| ?filterSettings | Object | Settings object to initialize the filter with | None | Default settings used |

---

### RemoveSourceFilter

Removes a filter from a source.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sourceName | String | Name of the source the filter is on | None | Unknown |
| ?sourceUuid | String | UUID of the source the filter is on | None | Unknown |
| filterName | String | Name of the filter to remove | None | N/A |

---

### SetSourceFilterName

Sets the name of a source filter (rename).

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sourceName | String | Name of the source the filter is on | None | Unknown |
| ?sourceUuid | String | UUID of the source the filter is on | None | Unknown |
| filterName | String | Current name of the filter | None | N/A |
| newFilterName | String | New name for the filter | None | N/A |

---

### GetSourceFilter

Gets the info for a specific source filter.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sourceName | String | Name of the source | None | Unknown |
| ?sourceUuid | String | UUID of the source | None | Unknown |
| filterName | String | Name of the filter | None | N/A |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| filterEnabled | Boolean | Whether the filter is enabled |
| filterIndex | Number | Index of the filter in the list, beginning at 0 |
| filterKind | String | The kind of filter |
| filterSettings | Object | Settings object associated with the filter |

---

### SetSourceFilterIndex

Sets the index position of a filter on a source.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sourceName | String | Name of the source the filter is on | None | Unknown |
| ?sourceUuid | String | UUID of the source the filter is on | None | Unknown |
| filterName | String | Name of the filter | None | N/A |
| filterIndex | Number | New index position of the filter | >= 0 | N/A |

---

### SetSourceFilterSettings

Sets the settings of a source filter.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sourceName | String | Name of the source the filter is on | None | Unknown |
| ?sourceUuid | String | UUID of the source the filter is on | None | Unknown |
| filterName | String | Name of the filter to set the settings of | None | N/A |
| filterSettings | Object | Object of settings to apply | None | N/A |
| ?overlay | Boolean | True == apply the settings on top of existing ones, False == reset the input to its defaults, then apply settings. | None | true |

---

### SetSourceFilterEnabled

Sets the enable state of a source filter.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sourceName | String | Name of the source the filter is on | None | Unknown |
| ?sourceUuid | String | UUID of the source the filter is on | None | Unknown |
| filterName | String | Name of the filter | None | N/A |
| filterEnabled | Boolean | New enable state of the filter | None | N/A |

## Scene Items Requests

### GetSceneItemList

Gets a list of all scene items in a scene.

Scenes only

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene to get the items of | None | Unknown |
| ?sceneUuid | String | UUID of the scene to get the items of | None | Unknown |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneItems | Array&lt;Object&gt; | Array of scene items in the scene |

---

### GetGroupSceneItemList

Basically GetSceneItemList, but for groups.

Using groups at all in OBS is discouraged, as they are very broken under the hood. Please use nested scenes instead.

Groups only

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the group to get the items of | None | Unknown |
| ?sceneUuid | String | UUID of the group to get the items of | None | Unknown |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneItems | Array&lt;Object&gt; | Array of scene items in the group |

---

### GetSceneItemId

Searches a scene for a source, and returns its id.

Scenes and Groups

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene or group to search in | None | Unknown |
| ?sceneUuid | String | UUID of the scene or group to search in | None | Unknown |
| sourceName | String | Name of the source to find | None | N/A |
| ?searchOffset | Number | Number of matches to skip during search. >= 0 means first forward. -1 means last (top) item | >= -1 | 0 |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneItemId | Number | Numeric ID of the scene item |

---

### GetSceneItemSource

Gets the source associated with a scene item.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.4.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene the item is in | None | Unknown |
| ?sceneUuid | String | UUID of the scene the item is in | None | Unknown |
| sceneItemId | Number | Numeric ID of the scene item | >= 0 | N/A |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sourceName | String | Name of the source associated with the scene item |
| sourceUuid | String | UUID of the source associated with the scene item |

---

### CreateSceneItem

Creates a new scene item using a source.

Scenes only

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene to create the new item in | None | Unknown |
| ?sceneUuid | String | UUID of the scene to create the new item in | None | Unknown |
| ?sourceName | String | Name of the source to add to the scene | None | Unknown |
| ?sourceUuid | String | UUID of the source to add to the scene | None | Unknown |
| ?sceneItemEnabled | Boolean | Enable state to apply to the scene item on creation | None | True |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneItemId | Number | Numeric ID of the scene item |

---

### RemoveSceneItem

Removes a scene item from a scene.

Scenes only

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene the item is in | None | Unknown |
| ?sceneUuid | String | UUID of the scene the item is in | None | Unknown |
| sceneItemId | Number | Numeric ID of the scene item | >= 0 | N/A |

---

### DuplicateSceneItem

Duplicates a scene item, copying all transform and crop info.

Scenes only

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene the item is in | None | Unknown |
| ?sceneUuid | String | UUID of the scene the item is in | None | Unknown |
| sceneItemId | Number | Numeric ID of the scene item | >= 0 | N/A |
| ?destinationSceneName | String | Name of the scene to create the duplicated item in | None | From scene is assumed |
| ?destinationSceneUuid | String | UUID of the scene to create the duplicated item in | None | From scene is assumed |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneItemId | Number | Numeric ID of the duplicated scene item |

---

### GetSceneItemTransform

Gets the transform and crop info of a scene item.

Scenes and Groups

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene the item is in | None | Unknown |
| ?sceneUuid | String | UUID of the scene the item is in | None | Unknown |
| sceneItemId | Number | Numeric ID of the scene item | >= 0 | N/A |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneItemTransform | Object | Object containing scene item transform info |

---

### SetSceneItemTransform

Sets the transform and crop info of a scene item.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene the item is in | None | Unknown |
| ?sceneUuid | String | UUID of the scene the item is in | None | Unknown |
| sceneItemId | Number | Numeric ID of the scene item | >= 0 | N/A |
| sceneItemTransform | Object | Object containing scene item transform info to update | None | N/A |

---

### GetSceneItemEnabled

Gets the enable state of a scene item.

Scenes and Groups

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene the item is in | None | Unknown |
| ?sceneUuid | String | UUID of the scene the item is in | None | Unknown |
| sceneItemId | Number | Numeric ID of the scene item | >= 0 | N/A |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneItemEnabled | Boolean | Whether the scene item is enabled. `true` for enabled, `false` for disabled |

---

### SetSceneItemEnabled

Sets the enable state of a scene item.

Scenes and Groups

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene the item is in | None | Unknown |
| ?sceneUuid | String | UUID of the scene the item is in | None | Unknown |
| sceneItemId | Number | Numeric ID of the scene item | >= 0 | N/A |
| sceneItemEnabled | Boolean | New enable state of the scene item | None | N/A |

---

### GetSceneItemLocked

Gets the lock state of a scene item.

Scenes and Groups

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene the item is in | None | Unknown |
| ?sceneUuid | String | UUID of the scene the item is in | None | Unknown |
| sceneItemId | Number | Numeric ID of the scene item | >= 0 | N/A |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneItemLocked | Boolean | Whether the scene item is locked. `true` for locked, `false` for unlocked |

---

### SetSceneItemLocked

Sets the lock state of a scene item.

Scenes and Group

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene the item is in | None | Unknown |
| ?sceneUuid | String | UUID of the scene the item is in | None | Unknown |
| sceneItemId | Number | Numeric ID of the scene item | >= 0 | N/A |
| sceneItemLocked | Boolean | New lock state of the scene item | None | N/A |

---

### GetSceneItemIndex

Gets the index position of a scene item in a scene.

An index of 0 is at the bottom of the source list in the UI.

Scenes and Groups

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene the item is in | None | Unknown |
| ?sceneUuid | String | UUID of the scene the item is in | None | Unknown |
| sceneItemId | Number | Numeric ID of the scene item | >= 0 | N/A |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneItemIndex | Number | Index position of the scene item |

---

### SetSceneItemIndex

Sets the index position of a scene item in a scene.

Scenes and Groups

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene the item is in | None | Unknown |
| ?sceneUuid | String | UUID of the scene the item is in | None | Unknown |
| sceneItemId | Number | Numeric ID of the scene item | >= 0 | N/A |
| sceneItemIndex | Number | New index position of the scene item | >= 0 | N/A |

---

### GetSceneItemBlendMode

Gets the blend mode of a scene item.

Blend modes:

- `OBS_BLEND_NORMAL`
- `OBS_BLEND_ADDITIVE`
- `OBS_BLEND_SUBTRACT`
- `OBS_BLEND_SCREEN`
- `OBS_BLEND_MULTIPLY`
- `OBS_BLEND_LIGHTEN`
- `OBS_BLEND_DARKEN`

Scenes and Groups

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene the item is in | None | Unknown |
| ?sceneUuid | String | UUID of the scene the item is in | None | Unknown |
| sceneItemId | Number | Numeric ID of the scene item | >= 0 | N/A |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| sceneItemBlendMode | String | Current blend mode |

---

### SetSceneItemBlendMode

Sets the blend mode of a scene item.

Scenes and Groups

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sceneName | String | Name of the scene the item is in | None | Unknown |
| ?sceneUuid | String | UUID of the scene the item is in | None | Unknown |
| sceneItemId | Number | Numeric ID of the scene item | >= 0 | N/A |
| sceneItemBlendMode | String | New blend mode | None | N/A |

## Outputs Requests

### GetVirtualCamStatus

Gets the status of the virtualcam output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| outputActive | Boolean | Whether the output is active |

---

### ToggleVirtualCam

Toggles the state of the virtualcam output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| outputActive | Boolean | Whether the output is active |

---

### StartVirtualCam

Starts the virtualcam output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### StopVirtualCam

Stops the virtualcam output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### GetReplayBufferStatus

Gets the status of the replay buffer output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| outputActive | Boolean | Whether the output is active |

---

### ToggleReplayBuffer

Toggles the state of the replay buffer output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| outputActive | Boolean | Whether the output is active |

---

### StartReplayBuffer

Starts the replay buffer output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### StopReplayBuffer

Stops the replay buffer output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### SaveReplayBuffer

Saves the contents of the replay buffer output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### GetLastReplayBufferReplay

Gets the filename of the last replay buffer save file.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| savedReplayPath | String | File path |

---

### GetOutputList

Gets the list of available outputs.

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| outputs | Array&lt;Object&gt; | Array of outputs |

---

### GetOutputStatus

Gets the status of an output.

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| outputName | String | Output name | None | N/A |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| outputActive | Boolean | Whether the output is active |
| outputReconnecting | Boolean | Whether the output is reconnecting |
| outputTimecode | String | Current formatted timecode string for the output |
| outputDuration | Number | Current duration in milliseconds for the output |
| outputCongestion | Number | Congestion of the output |
| outputBytes | Number | Number of bytes sent by the output |
| outputSkippedFrames | Number | Number of frames skipped by the output's process |
| outputTotalFrames | Number | Total number of frames delivered by the output's process |

---

### ToggleOutput

Toggles the status of an output.

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| outputName | String | Output name | None | N/A |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| outputActive | Boolean | Whether the output is active |

---

### StartOutput

Starts an output.

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| outputName | String | Output name | None | N/A |

---

### StopOutput

Stops an output.

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| outputName | String | Output name | None | N/A |

---

### GetOutputSettings

Gets the settings of an output.

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| outputName | String | Output name | None | N/A |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| outputSettings | Object | Output settings |

---

### SetOutputSettings

Sets the settings of an output.

- Complexity Rating: `4/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| outputName | String | Output name | None | N/A |
| outputSettings | Object | Output settings | None | N/A |

## Stream Requests

### GetStreamStatus

Gets the status of the stream output.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| outputActive | Boolean | Whether the output is active |
| outputReconnecting | Boolean | Whether the output is currently reconnecting |
| outputTimecode | String | Current formatted timecode string for the output |
| outputDuration | Number | Current duration in milliseconds for the output |
| outputCongestion | Number | Congestion of the output |
| outputBytes | Number | Number of bytes sent by the output |
| outputSkippedFrames | Number | Number of frames skipped by the output's process |
| outputTotalFrames | Number | Total number of frames delivered by the output's process |

---

### ToggleStream

Toggles the status of the stream output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| outputActive | Boolean | New state of the stream output |

---

### StartStream

Starts the stream output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### StopStream

Stops the stream output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### SendStreamCaption

Sends CEA-608 caption text over the stream output.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| captionText | String | Caption text | None | N/A |

## Record Requests

### GetRecordStatus

Gets the status of the record output.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| outputActive | Boolean | Whether the output is active |
| outputPaused | Boolean | Whether the output is paused |
| outputTimecode | String | Current formatted timecode string for the output |
| outputDuration | Number | Current duration in milliseconds for the output |
| outputBytes | Number | Number of bytes sent by the output |

---

### ToggleRecord

Toggles the status of the record output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| outputActive | Boolean | The new active state of the output |

---

### StartRecord

Starts the record output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### StopRecord

Stops the record output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| outputPath | String | File name for the saved recording |

---

### ToggleRecordPause

Toggles pause on the record output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### PauseRecord

Pauses the record output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### ResumeRecord

Resumes the record output.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

---

### SplitRecordFile

Splits the current file being recorded into a new file.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.5.0

---

### CreateRecordChapter

Adds a new chapter marker to the file currently being recorded.

Note: As of OBS 30.2.0, the only file format supporting this feature is Hybrid MP4.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.5.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?chapterName | String | Name of the new chapter | None | Unknown |

## Media Inputs Requests

### GetMediaInputStatus

Gets the status of a media input.

Media States:

- `OBS_MEDIA_STATE_NONE`
- `OBS_MEDIA_STATE_PLAYING`
- `OBS_MEDIA_STATE_OPENING`
- `OBS_MEDIA_STATE_BUFFERING`
- `OBS_MEDIA_STATE_PAUSED`
- `OBS_MEDIA_STATE_STOPPED`
- `OBS_MEDIA_STATE_ENDED`
- `OBS_MEDIA_STATE_ERROR`

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the media input | None | Unknown |
| ?inputUuid | String | UUID of the media input | None | Unknown |

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| mediaState | String | State of the media input |
| mediaDuration | Number | Total duration of the playing media in milliseconds. `null` if not playing |
| mediaCursor | Number | Position of the cursor in milliseconds. `null` if not playing |

---

### SetMediaInputCursor

Sets the cursor position of a media input.

This request does not perform bounds checking of the cursor position.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the media input | None | Unknown |
| ?inputUuid | String | UUID of the media input | None | Unknown |
| mediaCursor | Number | New cursor position to set | >= 0 | N/A |

---

### OffsetMediaInputCursor

Offsets the current cursor position of a media input by the specified value.

This request does not perform bounds checking of the cursor position.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the media input | None | Unknown |
| ?inputUuid | String | UUID of the media input | None | Unknown |
| mediaCursorOffset | Number | Value to offset the current cursor position by | None | N/A |

---

### TriggerMediaInputAction

Triggers an action on a media input.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the media input | None | Unknown |
| ?inputUuid | String | UUID of the media input | None | Unknown |
| mediaAction | String | Identifier of the `ObsMediaInputAction` enum | None | N/A |

## Ui Requests

### GetStudioModeEnabled

Gets whether studio is enabled.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| studioModeEnabled | Boolean | Whether studio mode is enabled |

---

### SetStudioModeEnabled

Enables or disables studio mode

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| studioModeEnabled | Boolean | True == Enabled, False == Disabled | None | N/A |

---

### OpenInputPropertiesDialog

Opens the properties dialog of an input.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input to open the dialog of | None | Unknown |
| ?inputUuid | String | UUID of the input to open the dialog of | None | Unknown |

---

### OpenInputFiltersDialog

Opens the filters dialog of an input.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input to open the dialog of | None | Unknown |
| ?inputUuid | String | UUID of the input to open the dialog of | None | Unknown |

---

### OpenInputInteractDialog

Opens the interact dialog of an input.

- Complexity Rating: `1/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?inputName | String | Name of the input to open the dialog of | None | Unknown |
| ?inputUuid | String | UUID of the input to open the dialog of | None | Unknown |

---

### GetMonitorList

Gets a list of connected monitors and information about them.

- Complexity Rating: `2/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Response Fields:**

| Name | Type  | Description |
| ---- | :---: | ----------- |
| monitors | Array&lt;Object&gt; | a list of detected monitors with some information |

---

### OpenVideoMixProjector

Opens a projector for a specific output video mix.

Mix types:

- `OBS_WEBSOCKET_VIDEO_MIX_TYPE_PREVIEW`
- `OBS_WEBSOCKET_VIDEO_MIX_TYPE_PROGRAM`
- `OBS_WEBSOCKET_VIDEO_MIX_TYPE_MULTIVIEW`

Note: This request serves to provide feature parity with 4.x. It is very likely to be changed/deprecated in a future release.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| videoMixType | String | Type of mix to open | None | N/A |
| ?monitorIndex | Number | Monitor index, use `GetMonitorList` to obtain index | None | -1: Opens projector in windowed mode |
| ?projectorGeometry | String | Size/Position data for a windowed projector, in Qt Base64 encoded format. Mutually exclusive with `monitorIndex` | None | N/A |

---

### OpenSourceProjector

Opens a projector for a source.

Note: This request serves to provide feature parity with 4.x. It is very likely to be changed/deprecated in a future release.

- Complexity Rating: `3/5`
- Latest Supported RPC Version: `1`
- Added in v5.0.0

**Request Fields:**

| Name | Type  | Description | Value Restrictions | ?Default Behavior |
| ---- | :---: | ----------- | :----------------: | ----------------- |
| ?sourceName | String | Name of the source to open a projector for | None | Unknown |
| ?sourceUuid | String | UUID of the source to open a projector for | None | Unknown |
| ?monitorIndex | Number | Monitor index, use `GetMonitorList` to obtain index | None | -1: Opens projector in windowed mode |
| ?projectorGeometry | String | Size/Position data for a windowed projector, in Qt Base64 encoded format. Mutually exclusive with `monitorIndex` | None | N/A |
