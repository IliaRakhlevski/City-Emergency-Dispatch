# City Emergency Dispatch

## [Real Time College](https://rt-ed.co.il/) - a multi-disciplinary Real-Time O.S. and Embedded Software Solutions Center, providing consulting, development, integration, training and support solutions.<br/>

## Real-Time Embedded Concepts Course Project

Course project developed as part of the Real-Time Embedded Concepts course.
The project simulates a city-wide emergency dispatch system where a central server generates emergency events and distributes them to specialized response departments. 
Each emergency vehicle operates as an independent FreeRTOS task and processes events according to their priority level.
The system demonstrates practical implementation of real-time operating system concepts including task scheduling, inter-task communication, synchronization mechanisms, 
fault recovery, resource management and UDP-based client-server communication.


### Development Environment

Operating System
- Ubuntu 24.04.4 LTS (Noble Numbat)

Development Tools
- Visual Studio Code 1.123.0
- GCC 13.3.0
- GNU Make 4.3

Libraries and Frameworks
- FreeRTOS POSIX
- SQLite3 3.45.1
- BSD/POSIX Socket API (UDP)

## Overview

The project simulates a city-wide emergency dispatch system built on top of FreeRTOS POSIX.
A central server continuously generates emergency events, stores them in an SQLite database and distributes them to specialized departments using UDP communication. 
On the client side, events are dispatched to priority-based queues and processed by independent vehicle tasks representing emergency response units.
The system includes Shift Manager and Fault Manager components responsible for resource management, event retry handling and dynamic vehicle availability control.

The application demonstrates the use of:

- FreeRTOS Tasks
- Queues
- Mutexes
- Event Groups
- Priority-Based Scheduling
- UDP Client-Server Communication
- SQLite Persistence
- Fault Recovery and Retry Mechanisms
- Dynamic Resource Management

## System Components

### Server

- Generates random city events
- Stores events in SQLite
- Sends events to the client via UDP
- Receives acknowledgements and completion reports
- Maintains runtime statistics

### Client

- Receives events from the server
- Dispatches events according to department and priority
- Processes events using vehicle tasks
- Manages vehicle availability through Shift Manager
- Retries interrupted events through Fault Manager
- Reports completion results to the server

## Departments

- Ambulance
- Police
- Fire Department
- Maintenance
- Waste Collection
- Electric Services

## Task Architecture

### Server Tasks

| Task | Description |
|--------|--------|
| ServerEventGeneratorTask | Generates city events |
| ServerUdpTxTask | Sends events to the client |
| ServerUdpRxTask | Receives acknowledgements and completion reports |

### Client Tasks

| Task | Description |
|--------|--------|
| ClientUdpRxTask | Receives UDP messages |
| ClientDispatcherTask | Routes events to department queues |
| VehicleTask | Processes assigned events |
| ShiftManagerTask | Manages vehicle availability |
| FaultManagerTask | Handles retries for failed events |

## Event Lifecycle

```text
Server
  ↓
Generate Event
  ↓
SQLite
  ↓
UDP
  ↓
Client
  ↓
Dispatcher
  ↓
Department Queue
  ↓
Vehicle Task
  ↓
Completion Report
  ↓
Server
  ↓
SQLite Update
```

## Features

- FreeRTOS POSIX simulation
- UDP client/server architecture
- SQLite persistence
- Priority-based scheduling
- Automatic retry mechanism
- Event cancellation support
- Dynamic resource management
- Runtime statistics collection

## Build

```bash
make
```

## Run Server

```bash
./build/city_emergency_dispatch -server
```

## Run Client

```bash
./build/city_emergency_dispatch -client
```

## Author

Ilia Rakhlevski
