# C++ Producer-Consumer Project with ActiveMQ and Docker Compose

A complete, self-contained C++ producer-consumer application using ActiveMQ Artemis broker with STOMP protocol. The entire solution is containerized using Docker Compose and designed to run on Docker Desktop (Linux containers) on a Windows host machine.

## Project Structure

```
producer-consumer-activemq/
├── producer/
│   ├── CMakeLists.txt
│   └── main.cpp (Producer Logic)
├── consumer/
│   ├── CMakeLists.txt
│   └── main.cpp (Consumer Logic)
├── Dockerfile (Multi-stage build for both applications)
├── docker-compose.yml
├── .gitignore
└── README.md
```

## Technical Specifications

| Feature | Specification | Details |
|---------|---------------|---------|
| **Messaging Broker** | ActiveMQ Artemis | Latest Docker Hub image: `apache/activemq-artemis:latest` |
| **Protocol** | STOMP | Port 61613 for internal communication |
| **C++ Standard** | C++17 | Modern C++ features for robust code |
| **Build System** | CMake | Cross-platform build management |
| **Queue** | `/queue/ProjectQueue` | Target destination for all messages |
| **Message Count** | 10 messages | Producer sends, Consumer receives exactly 10 messages |
| **Containerization** | Docker Compose | Single Dockerfile with multi-stage build |

## Key Features

### Producer Application
- Connects to ActiveMQ via STOMP protocol
- Sends 10 distinct timestamped messages to `/queue/ProjectQueue`
- Includes connection retry logic for broker startup delays
- 1-second delay between messages
- Comprehensive logging of connection status and message sending

### Consumer Application
- Connects to ActiveMQ via STOMP protocol
- Subscribes to `/queue/ProjectQueue`
- Receives and processes exactly 10 messages
- Graceful shutdown after receiving all messages
- Ready for future JSON payload deserialization (commented placeholder)
- Detailed logging of received messages and counts

### Docker Architecture
- **Multi-stage Dockerfile**: Separates build and runtime environments
- **Build Stage**: Uses `gcc:latest` with CMake and development tools
- **Runtime Stage**: Uses `debian:stable-slim` for minimal footprint
- **Build Arguments**: Single Dockerfile handles both producer and consumer via `TARGET_APP` argument

## Quick Start

### Prerequisites
- Docker Desktop installed and running on Windows
- Linux containers mode enabled in Docker Desktop

### Build and Run

Execute the following single command to build and run the entire stack:

```bash
docker-compose up --build
```

This command will:
1. Build the ActiveMQ Artemis container
2. Build the C++ producer application
3. Build the C++ consumer application
4. Start all services with proper dependencies
5. Execute the producer-consumer message flow

## Monitoring Logs

### View All Logs
```bash
docker-compose logs
```

### View Specific Service Logs
```bash
# Producer logs
docker-compose logs producer

# Consumer logs
docker-compose logs consumer

# ActiveMQ logs
docker-compose logs activemq
```

### Follow Live Logs
```bash
# Follow all logs in real-time
docker-compose logs -f

# Follow specific service logs
docker-compose logs -f producer
docker-compose logs -f consumer
```

## Expected Output

### Producer Output
```
[PRODUCER] Starting C++ Producer Application
[PRODUCER] Connection attempt 1/10
[PRODUCER] Successfully connected to ActiveMQ via STOMP
[PRODUCER] Sending 10 messages to /queue/ProjectQueue
[PRODUCER] Sending message 1/10: Hello from C++ Producer - MSG_20251007_143022_INDEX_1
[PRODUCER] Message 1 sent successfully
...
[PRODUCER] All messages sent. Disconnecting...
[PRODUCER] Producer application completed successfully
```

### Consumer Output
```
[CONSUMER] Starting C++ Consumer Application
[CONSUMER] Connection attempt 1/10
[CONSUMER] Successfully connected to ActiveMQ via STOMP
[CONSUMER] Successfully subscribed to /queue/ProjectQueue
[CONSUMER] Waiting for messages from /queue/ProjectQueue
[CONSUMER] Expected to receive 10 messages
[CONSUMER] Received message 1/10: Hello from C++ Producer - MSG_20251007_143022_INDEX_1
...
[CONSUMER] All 10 messages received successfully!
[CONSUMER] Consumer application completed successfully
```

## Network Configuration

- **Internal Communication**: Services communicate via Docker network using service names
- **ActiveMQ Service Name**: `activemq` (used in C++ connection strings)
- **External Access**: STOMP port 61613 exposed for optional host-side testing
- **Admin Console**: Available at http://localhost:8161 (admin/admin)

## Development Environment

- **Host OS**: Windows with Docker Desktop
- **Container OS**: Linux (Debian-based)
- **Build Environment**: GCC with CMake
- **Runtime Environment**: Minimal Debian slim image

## Future Extensibility

The current implementation uses simple string messages but is designed for easy extension:

- **JSON Support**: Consumer includes placeholder comments for JSON deserialization
- **Message Schema**: Can be extended to support complex data structures
- **Protocol Upgrades**: STOMP client supports versions 1.0, 1.1, and 1.2
- **Scaling**: Architecture supports multiple producer/consumer instances

## Troubleshooting

### Common Issues

1. **Connection Failed**: Ensure Docker Desktop is running and ActiveMQ container is healthy
2. **Build Errors**: Check that Docker has sufficient resources allocated
3. **Port Conflicts**: Ensure ports 61613 and 8161 are not in use by other applications

### Cleanup

```bash
# Stop and remove all containers
docker-compose down

# Remove built images (optional)
docker-compose down --rmi all

# Remove volumes (optional)
docker-compose down --volumes
```

## Project Validation

The project is confirmed to work on:
- ✅ Windows Host Machine
- ✅ Docker Desktop with Linux Containers
- ✅ STOMP Protocol Communication
- ✅ ActiveMQ Artemis Broker
- ✅ Modern C++ (C++17)
- ✅ CMake Build System
- ✅ Multi-stage Docker Build