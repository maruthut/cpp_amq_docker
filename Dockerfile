# Multi-stage build Dockerfile for C++ Producer-Consumer project
# Build stage
FROM gcc:latest AS builder

# Install dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    make \
    git \
    libssl-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . .

# Build argument to determine which app to build
ARG TARGET_APP

# Configure and build the application
RUN if [ "$TARGET_APP" = "producer" ]; then \
        cd producer && \
        cmake . && \
        make; \
    elif [ "$TARGET_APP" = "consumer" ]; then \
        cd consumer && \
        cmake . && \
        make; \
    else \
        echo "Invalid TARGET_APP. Must be 'producer' or 'consumer'" && exit 1; \
    fi

# Runtime stage
FROM debian:stable-slim AS runtime

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

# Build argument to determine which app to copy
ARG TARGET_APP

# Copy the built executable
COPY --from=builder --chmod=755 /build/${TARGET_APP}/${TARGET_APP} /app/app

# Set working directory
WORKDIR /app

# Run the application
CMD ["./app"]