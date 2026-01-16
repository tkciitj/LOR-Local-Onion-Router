# LOR - Local Onion Router (File Transfer)

A low-level, educational implementation of **onion routing** inspired by the Tor network.  
This project demonstrates how **layered encryption** and **multi-hop routing** can be used
to securely transfer files through untrusted intermediate nodes without revealing
the complete communication path to any single relay.

> ⚠️ This is **not** an implementation of the real Tor network or Tor Browser.  
> It is a **local simulation** designed for learning systems programming,
> networking, and applied cryptography.

---

## 📌 Project Objectives

- Understand and implement the **core concept of onion routing**
- Demonstrate **layered encryption** (one layer per relay)
- Ensure that **no relay has access to plaintext data**
- Simulate **multi-hop anonymous routing** over TCP
- Provide a **visualization of data flow** across relays
- Build a clean, modular, low-level C++ networking project

---

## 🧠 Core Idea (Correct Mental Model)
> Sender → Relay 1 → Relay 2 → Relay 3 → Receiver

- The sender encrypts the data in **multiple layers**
- Each relay removes **exactly one encryption layer**
- Relays only know:
  - the previous hop
  - the next hop
- The final receiver reconstructs the original file

This mirrors the **onion routing principle** used by Tor.

---

## 🔐 What This Project Demonstrates

| Concept | Demonstration |
|------|--------------|
| Onion Routing | Nested encryption layers |
| Confidentiality | Relays cannot read plaintext |
| Anonymity (conceptual) | No relay knows full route |
| Integrity | Optional end-to-end hash verification |
| Networking | TCP socket programming |
| Systems Design | Multi-process relay architecture |

---

## ❌ What This Project Does NOT Do

- No Tor Browser
- No real Tor network interaction
- No directory authorities
- No hidden services (.onion)
- No traffic obfuscation or padding
- No real anonymity guarantees

This is a **controlled, educational simulation**.

---

## ⚙️ High-Level Workflow

### 1. Configuration
- Fixed number of relays (default: 3)
- Predefined ports and symmetric keys
- Fixed chunk size

### 2. Sender
- Reads file in binary mode
- Splits file into chunks
- Encrypts each chunk in **multiple layers**
- Sends encrypted data to Relay 1

### 3. Relay Nodes
Each relay:
- Acts as a TCP server and client
- Decrypts **one encryption layer**
- Forwards remaining encrypted payload
- Never sees original plaintext

### 4. Receiver
- Receives decrypted chunks
- Reassembles the original file
- (Optional) Verifies integrity using hash

---

## 🧱 System Architecture
+--------+ +--------+ +--------+ +--------+ +----------+
| Sender | ---> | Relay1 | ---> | Relay2 | ---> | Relay3 | ---> | Receiver |
+--------+ +--------+ +--------+ +--------+ +----------+

Each arrow represents a TCP connection.

---

## 📦 Packet Flow (Conceptual)
Plaintext Chunk
↓ encrypt with K3
Encrypted Layer 3
↓ encrypt with K2
Encrypted Layer 2
↓ encrypt with K1
Final Onion Packet → Relay 1

Decryption happens in reverse order.

---

## 📊 Visualization (Planned)

This project includes a **central visualization component** that passively observes events.

### Visualization principles:
- Does NOT affect routing
- Does NOT decrypt data
- Receives only event logs

### Example events:
```json
{
  "node": "Relay2",
  "chunk_id": 4,
  "event": "layer_decrypted"
}



