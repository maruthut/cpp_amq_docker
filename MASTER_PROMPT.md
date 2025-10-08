# Master Prompt: C++ Producer-Consumer with ActiveMQ and Docker

## 📋 Project Overview

Create a complete, self-contained C++ producer-consumer application using ActiveMQ Artemis broker with STOMP protocol, fully containerized using Docker Compose and designed to run on Docker Desktop (Linux containers) on a Windows host machine.

## 🎯 Core Requirements

### Functional Requirements
1. **Producer Application (C++17)**
   - Connect to ActiveMQ Artemis via STOMP protocol
   - Send exactly 10 timestamped messages to `/queue/ProjectQueue`
   - Messages format: `Hello from C++ Producer - MSG_YYYYMMDD_HHMMSS_INDEX_N`
   - 1-second delay between messages
   - Graceful shutdown after sending all messages
   - Exit with code 0 on success

2. **Consumer Application (C++17)**
   - Connect to ActiveMQ Artemis via STOMP protocol
   - Subscribe to `/queue/ProjectQueue`
   - Receive and process exactly 10 messages
   - Display each received message with counter
   - Graceful shutdown after receiving all messages
   - Exit with code 0 on success

3. **Message Broker (ActiveMQ Artemis)**
   - Use latest Docker Hub image: `apache/activemq-artemis:latest`
   - Enable STOMP protocol on port 61613
   - Web console accessible on port 8161
   - Support anonymous connections (no authentication required)

### Technical Requirements
1. **Programming Language**: C++17 with modern features
2. **Build System**: CMake with proper threading support
3. **Protocol**: STOMP 1.0/1.1/1.2 compatibility
4. **Containerization**: Docker Compose with multi-stage builds
5. **Network**: Internal Docker bridge network with service discovery
6. **Dependencies**: Minimal external dependencies, use standard libraries

### Infrastructure Requirements
1. **Host Environment**: Windows with Docker Desktop
2. **Container Environment**: Linux containers (Debian-based)
3. **Build Environment**: gcc:latest with CMake
4. **Runtime Environment**: debian:stable-slim for minimal footprint
5. **Orchestration**: Docker Compose with health checks and dependencies

## 📁 Required Project Structure

```
cpp_amq_docker/
├── producer/
│   ├── CMakeLists.txt
│   └── main.cpp
├── consumer/
│   ├── CMakeLists.txt
│   └── main.cpp
├── Dockerfile
├── docker-compose.yml
├── .gitignore
├── README.md
└── MASTER_PROMPT.md
```

## 💻 Detailed Implementation Specifications

### Producer Application (producer/main.cpp)

**Core Features:**
- STOMP client implementation with socket programming
- Connection retry logic (10 attempts, 3-second intervals)
- Hostname resolution using `gethostbyname()`
- Message generation with timestamps
- Proper STOMP frame formatting
- Error handling and logging
- Clean disconnection

**Key Components:**
```cpp
class SimpleStompClient {
    bool connect(const std::string& host, int port);
    bool sendMessage(const std::string& destination, const std::string& message);
    void disconnect();
};

std::string generateMessageId(int index);
std::string generateTimestamp();
```

**STOMP Frame Format:**
```
CONNECT
accept-version:1.0,1.1,1.2
host:activemq
heart-beat:0,0

NULL_BYTE
```

### Consumer Application (consumer/main.cpp)

**Core Features:**
- STOMP client implementation (similar to producer)
- Queue subscription management
- Message receiving and parsing
- Message counting and validation
- Graceful shutdown after receiving target count
- Future-ready for JSON deserialization

**Key Components:**
```cpp
class SimpleStompClient {
    bool connect(const std::string& host, int port);
    bool subscribe(const std::string& destination, const std::string& subscription_id);
    std::string receiveMessage();
    void disconnect();
};
```

**STOMP Subscription Frame:**
```
SUBSCRIBE
destination:/queue/ProjectQueue
id:sub-1
ack:auto

NULL_BYTE
```

### Build Configuration (CMakeLists.txt)

**Producer CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.10)
project(Producer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(producer main.cpp)

find_package(Threads REQUIRED)
target_link_libraries(producer Threads::Threads)
```

**Consumer CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.10)
project(Consumer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(consumer main.cpp)

find_package(Threads REQUIRED)
target_link_libraries(consumer Threads::Threads)
```

### Docker Configuration

**Multi-Stage Dockerfile:**
```dockerfile
# Build stage
FROM gcc:latest as builder
RUN apt-get update && apt-get install -y cmake
WORKDIR /build
COPY . .
ARG TARGET_APP
RUN if [ "$TARGET_APP" = "producer" ]; then \
        cd producer && cmake . && make; \
    elif [ "$TARGET_APP" = "consumer" ]; then \
        cd consumer && cmake . && make; \
    fi

# Runtime stage
FROM debian:stable-slim
RUN apt-get update && apt-get install -y \
    libc6 libgcc-s1 libstdc++6 && \
    rm -rf /var/lib/apt/lists/*
ARG TARGET_APP
COPY --from=builder --chmod=755 /build/${TARGET_APP}/${TARGET_APP} /app/
WORKDIR /app
CMD ["./${TARGET_APP}"]
```

**Docker Compose Configuration:**
```yaml
version: '3.8'

services:
  activemq:
    image: apache/activemq-artemis:latest
    container_name: activemq-artemis
    ports:
      - "61613:61613"
      - "8161:8161"
    environment:
      - ARTEMIS_USERNAME=admin
      - ARTEMIS_PASSWORD=admin
      - ANONYMOUS_LOGIN=true
    healthcheck:
      test: ["CMD-SHELL", "curl -f http://localhost:8161/console/ || exit 1"]
      interval: 30s
      timeout: 10s
      retries: 5
      start_period: 30s
    networks:
      - amq-network

  producer:
    build:
      context: .
      dockerfile: Dockerfile
      args:
        TARGET_APP: producer
    container_name: cpp-producer
    depends_on:
      activemq:
        condition: service_healthy
    restart: "no"
    networks:
      - amq-network

  consumer:
    build:
      context: .
      dockerfile: Dockerfile
      args:
        TARGET_APP: consumer
    container_name: cpp-consumer
    depends_on:
      activemq:
        condition: service_healthy
    restart: "no"
    networks:
      - amq-network

networks:
  amq-network:
    driver: bridge
```

### Git Configuration (.gitignore)

```gitignore
# Compiled executables (specific paths to avoid excluding source directories)
/producer/producer
/consumer/consumer

# Build directories
**/build/
**/cmake-build-*/

# CMake generated files
CMakeCache.txt
CMakeFiles/
Makefile
cmake_install.cmake
*.cmake

# Docker
.dockerignore

# IDE files
.vscode/
.idea/
*.swp
*.swo
*~

# OS files
.DS_Store
Thumbs.db

# Logs
*.log
logs/
```

## 🔧 Implementation Guidelines

### STOMP Protocol Implementation
1. **Connection Frame**: Include accept-version, host, heart-beat headers
2. **Frame Termination**: Use proper null byte termination (`char(0)`)
3. **Error Handling**: Parse CONNECTED/ERROR responses
4. **Clean Shutdown**: Send DISCONNECT frame before closing socket

### Error Handling Strategy
1. **Connection Failures**: Retry logic with exponential backoff
2. **Network Issues**: Graceful degradation and logging
3. **STOMP Errors**: Parse error frames and respond appropriately
4. **Resource Cleanup**: Ensure sockets are properly closed

### Logging Requirements
1. **Producer Logging**:
   - Connection attempts and status
   - Message sending progress (N/10)
   - Success/failure for each message
   - Disconnection status

2. **Consumer Logging**:
   - Connection and subscription status
   - Message reception progress (N/10)
   - Total messages received
   - Shutdown status

### Performance Requirements
1. **Connection Time**: < 3 seconds per application
2. **Message Throughput**: 1 message/second (configurable)
3. **End-to-End Latency**: < 100ms per message
4. **Memory Usage**: < 500MB total for all containers
5. **Startup Time**: < 30 seconds full stack

## 🧪 Validation Criteria

### Success Indicators
1. **Build Success**: All containers build without errors
2. **Connection Success**: Both applications connect to ActiveMQ
3. **Message Flow**: Exactly 10 messages sent and received
4. **Message Integrity**: All messages received in correct format
5. **Graceful Shutdown**: All containers exit with code 0
6. **No Errors**: Clean logs without error frames

### Expected Output Patterns
**Producer Log Pattern:**
```
[PRODUCER] Starting C++ Producer Application
[PRODUCER] Connection attempt 1/10
[PRODUCER] Successfully connected to ActiveMQ via STOMP
[PRODUCER] Sending 10 messages to /queue/ProjectQueue
[PRODUCER] Sending message 1/10: Hello from C++ Producer - MSG_*_INDEX_1
[PRODUCER] Message 1 sent successfully
...
[PRODUCER] All messages sent. Disconnecting...
[PRODUCER] Producer application completed successfully
```

**Consumer Log Pattern:**
```
[CONSUMER] Starting C++ Consumer Application
[CONSUMER] Connection attempt 1/10
[CONSUMER] Successfully connected to ActiveMQ via STOMP
[CONSUMER] Successfully subscribed to /queue/ProjectQueue
[CONSUMER] Received message 1/10: Hello from C++ Producer - MSG_*_INDEX_1
...
[CONSUMER] All 10 messages received successfully!
[CONSUMER] Consumer application completed successfully
```

## 🚀 Deployment Instructions

### Prerequisites
- Windows 10/11 with Docker Desktop
- Linux containers enabled
- Ports 61613 and 8161 available
- At least 4GB RAM and 2GB disk space

### Quick Start Commands
```bash
# Clone repository
git clone <repository-url>
cd cpp_amq_docker

# Build and run everything
docker-compose up --build

# Cleanup
docker-compose down
```

### Development Workflow
```bash
# Development iteration
docker-compose up --build

# Debug specific service
docker-compose logs producer
docker-compose logs consumer
docker-compose logs activemq

# Interactive debugging
docker-compose run --rm producer /bin/bash
```

## 🔮 Extension Points

### Future Enhancements
1. **JSON Message Support**: Replace plain text with structured JSON
2. **Authentication**: Implement proper STOMP authentication
3. **SSL/TLS**: Secure communication channels
4. **Multiple Queues**: Support routing to different destinations
5. **Kubernetes**: Add K8s deployment manifests
6. **Monitoring**: Prometheus metrics and health endpoints
7. **Configuration**: External configuration management

### Architecture Evolution
1. **Microservices**: Split into independent services
2. **Load Balancing**: Multiple producer/consumer instances
3. **Message Persistence**: Durable messaging guarantees
4. **Dead Letter Queues**: Failed message handling
5. **Distributed Tracing**: OpenTelemetry integration

## 📋 Troubleshooting Reference

### Common Issues
1. **Connection Refused**: Wait for ActiveMQ health check
2. **Port Conflicts**: Check port availability with `netstat`
3. **Build Failures**: Verify Docker resources and clear cache
4. **Authentication Errors**: Ensure anonymous login enabled
5. **Network Issues**: Inspect Docker bridge network

### Diagnostic Commands
```bash
# System status
docker-compose ps
docker stats

# Network debugging
docker network inspect cpp_amq_docker_amq-network
docker-compose exec producer ping activemq

# Log analysis
docker-compose logs -f --tail=100
```

## 💡 Best Practices

### Code Quality
1. Use modern C++17 features appropriately
2. Implement proper RAII for resource management
3. Follow Google C++ Style Guide
4. Include comprehensive error handling
5. Write self-documenting code with clear variable names

### Docker Best Practices
1. Use multi-stage builds for optimal image size
2. Implement proper health checks
3. Use specific image tags in production
4. Minimize container privileges
5. Implement graceful shutdown handling

### Git Workflow
1. Use conventional commit messages
2. Create feature branches for development
3. Include comprehensive documentation updates
4. Test all changes with full integration testing
5. Use semantic versioning for releases

---

**This master prompt serves as the complete specification for recreating the C++ Producer-Consumer project with ActiveMQ and Docker Compose. Follow these requirements precisely to achieve a working, professional-grade implementation.**