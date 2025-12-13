
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
