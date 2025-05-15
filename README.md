# 🐾 SNIFF — A Lightweight WebSocket-Enabled File Watcher Server

**SNIFF** is a simple, efficient TCP server built in C that supports **hot-reloading** through **file change notifications** using multithreading and synchronization primitives. It is designed to detect changes in a target file (e.g., `index.html`) and instantly broadcast updates to connected WebSocket clients.

---

## 🔧 Features

- 📁 **File Watcher** using inotify (Linux) to detect real-time file changes.
- 🌐 **WebSocket Server** that upgrades HTTP connections and maintains active client sockets.
- 🔄 **Live Reload**: Notify all connected WebSocket clients when the watched file changes.
- 🧵 **Multithreaded** architecture using POSIX threads for handling:
  - Client connections
  - File monitoring
  - File watcher notifications
- 📦 **Modular Design** for better maintainability and extensibility.

---

## 🧠 How It Works

1. The server listens for TCP connections on port `8080`.
2. Clients requesting a WebSocket upgrade are handled and tracked.
3. A background thread (`file_watcher`) monitors changes to a target file (e.g., `index.html`).
4. Upon detecting a change, it signals the `monitor` thread via condition variables.
5. The `monitor` thread then broadcasts the update to all connected WebSocket clients.

---

## 🚀 Getting Started

### 🔨 Build

```bash
gcc src/*.c -o server -lssl -lcrypto -lpthread
