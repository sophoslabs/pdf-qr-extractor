# QR Extractor IPC Behavior Specification

## 1. Connection Model

- The Client opens a new UNIX domain socket connection for each request.
- Connections are not reused.
- Each connection handles exactly one request-response cycle.

This avoids head-of-line blocking and simplifies concurrency.

## 2. Concurrency Model

- The Client may issue multiple concurrent requests using multiple threads.
- The extractor accepts multiple concurrent socket connections.
- Each request is stateless.
- No session state is shared between requests.

## 3. Timeout Behavior

All socket read/write operations are bounded by REQUEST_TIMEOUT_MS.

If a timeout occurs:

- The socket is closed.
- The request is considered failed.
- The daemon may retry depending on status classification.

No socket operation is allowed to block indefinitely.

## 4. Failure Handling

|        Failure         | Daemon Behavior |
|------------------------|-----------------|
| Socket connect failure |      Retry      |
| Read/write timeout     |      Retry      |
| Partial read/write     |      Retry      |
| VERSION_UNSUPPORTED    |   Do not retry  |
| LIMIT_EXCEEDED         |   Do not retry  |
| INVALID_REQUEST        |   Do not retry  |
| INTERNAL_ERROR         |      Retry      |

## 5. Shutdown Behavior

When the extractor receives SIGTERM:

- It stops accepting new connections.
- Existing connections are allowed to complete or timeout.
- The UNIX socket file is removed.
- The process exits gracefully.

## 6. Resource Ownership

- Each connection owns its socket.
- No socket descriptors are shared across threads.
- Memory buffers are request-scoped.

## 7. Backpressure

- If extractor workers are saturated, accept queue fills naturally.
- Daemon threads experience connect or read timeouts.
- No unbounded queues are used inside extractor.

## 8. Security Model

- UNIX domain socket filesystem permissions restrict access.
- Extractor validates peer credentials using SO_PEERCRED.
- Extractor runs as non-root user.

## 9. Observability

Extractor logs:

- Protocol validation failures
- Timeout occurrences
- Connection failures
- Shutdown events

Client logs:

- IPC failures
- Retry decisions
- Protocol errors

## 10. Design Intent

This IPC behavior model ensures:

- High throughput
- No deadlocks
- Predictable failure handling
- Horizontal scalability
- Operational stability
