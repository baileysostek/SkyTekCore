# SkyTek
The SkyTek framework enables edge devices to communicate with a frontend over various modalities. The goal of this project is to easily enable communication between some kind of web frontend and some edge device in the field.

## Installation

## Getting Started

## Supported Modalities
As this project matures more modalities will be added. Currently the following modalities are supported:
1. USB Serial

# Messages

## Message Types
The message transfer protocol within the SkyTek framework is pretty streightforward 
### Query

### Publish

## Built In Messages
The framework itself defines the following messages. <br>NOTE: These built in messages do not count towards the limit of `128` user-defined messages.  

| Message Type      | API                                   | Signature              |
|-------------------|---------------------------------------|------------------------|
| [`Query`](#query) |[`skytek`](#skytek-message)            | `/{uuid}:skytek`       |
| [`Query`](#query) |[`capabilities`](#capabilities-message)| `/{uuid}:capabilities` |
| [`Query`](#query) |[`help`](#query-message)               | `/{uuid}:help`         |

### SkyTek Message

### Capabilities Message

### Help Message

## Defining Custom Messages

# Capabilities
Now that we have a well defined way to talk to our edge devices, we need a way to represent that data on the front end. Capabilities enable an edge device to prescribe which frontend elements they intend for their messages to interact with. SkyTek ships with several capabilities already built out. 

# Devices
Anything that can communicate over one of the [supported modalities](#supported-modalities)
## Connection Handshake
When a SkyTek device is connected over USB to a host computer running the SkyTek frontend the following handshake occurs. 
1. The host computer sends over a request for the edge device to identify itself. The host computer issues the following message `/{uuid}:skytek`
<br>The [message type](#message-types) issued here is a [query](#query) therefore there is a callback stored within the frontend waiting for a message to be emitted from the edge-device containing this `{uuid}`. When an 