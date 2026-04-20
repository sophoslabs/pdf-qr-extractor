# QR Extractor IPC Versioning Policy

## 1. Purpose

This document defines how the QR Extractor IPC protocol is versioned, evolved,
and validated between the Client and the extractor service.

The goal is to ensure:

- Safe independent deployment
- Predictable compatibility behavior
- No silent protocol corruption
- Explicit failure on incompatibility

## 2. Version Field

The IPC protocol includes a 32-bit `version` field in the request header.

This version represents the **protocol contract**, not the application version.

## 3. Compatibility Rules

|                Scenario            | Behavior                   |
|------------------------------------|----------------------------|
| Same version                       | Fully compatible           |
| Daemon version > Extractor version | Extractor rejects          |
| Daemon version < Extractor version | Extractor rejects          |
| Version mismatch                   | VERSION_UNSUPPORTED status |

No automatic downgrade or fallback is attempted.

## 4. Backward Compatibility Policy

Backward compatibility is guaranteed **only within the same protocol version**.

Any incompatible change requires incrementing the protocol version.

Examples of incompatible changes:

- Changing header layout
- Changing field meaning
- Changing framing rules
- Changing status code semantics

## 5. Forward Compatibility Policy

Forward compatibility is **not guaranteed**.

Older extractors must explicitly reject newer protocol versions.

This prevents undefined behavior and silent data corruption.

## 6. Allowed Compatible Changes (Same Version)

The following changes may be done without incrementing version:

- Adding new status codes (if old ones unchanged)
- Adding optional payload fields (if old framing preserved)
- Improving extractor internal behavior
- Performance optimizations

## 7. Required Version Increment

Protocol version must be incremented when:

- Header layout changes
- Framing changes
- Field order changes
- Field sizes change
- Status meanings change
- Payload format changes

## 8. Version Upgrade Procedure

When upgrading protocol version:

1. Increment PROTOCOL_VERSION constant.
2. Update protocol.md.
3. Update versioning_policy.md.
4. Update Client to support new version.
5. Deploy extractor and Client together.

Mixed-version deployments are not supported.

## 9. Error Handling

When a version mismatch occurs:

- Extractor responds with VERSION_UNSUPPORTED.
- Client logs explicit version mismatch.
- Client must not retry.

This ensures fast failure and prevents retry storms.

## 10. Audit & Traceability

Every protocol version change must:

- Be documented
- Be reviewed
- Be tested
- Be version-controlled

## 11. Design Intent

The versioning policy enforces:

- Explicit contracts
- Safe evolution
- Predictable deployments
- No hidden compatibility assumptions

Note :- Application version and protocol version are independent. Multiple application releases may share the same protocol version as long as the IPC contract remains unchanged. Protocol version is incremented only when incompatible protocol changes are introduced.

