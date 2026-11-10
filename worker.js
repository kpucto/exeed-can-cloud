
export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    const path = url.pathname;

    if (path === "/" || path === "/health") {
      return Response.json({
        status: "online",
        service: "EXEED CAN CLOUD",
        mode: "READ ONLY",
        authentication: "DISABLED FOR TEST"
      });
    }

    let role;

    if (path === "/device") {
      role = "device";
    } else if (path === "/viewer") {
      role = "viewer";
    } else if (path === "/status") {
      role = "status";
    } else {
      return new Response("Not found", { status: 404 });
    }

    if (
      role !== "status" &&
      request.headers.get("Upgrade")?.toLowerCase() !== "websocket"
    ) {
      return new Response("WebSocket required", { status: 426 });
    }

    if (!env.CAN_HUB) {
      return Response.json(
        { error: "CAN_HUB binding unavailable" },
        { status: 500 }
      );
    }

    const id = env.CAN_HUB.idFromName("exeed-main");
    const stub = env.CAN_HUB.get(id);

    const internalUrl = new URL(request.url);
    internalUrl.pathname = role === "status" ? "/status" : "/connect";
    internalUrl.searchParams.set("role", role);

    return stub.fetch(new Request(internalUrl, request));
  }
};

export class CanHub {
  constructor(state, env) {
    this.state = state;
    this.env = env;

    this.totalFrames = 0;
    this.lastSeen = null;
    this.lastConnected = null;
    this.lastClose = null;
    this.lastError = null;
    this.lastMessage = null;
  }

  async fetch(request) {
    const url = new URL(request.url);

    if (url.pathname === "/status") {
      return Response.json({
        status: "online",
        mode: "READ ONLY",

        devices: this.state.getWebSockets("device").length,
        viewers: this.state.getWebSockets("viewer").length,

        totalFrames: this.totalFrames,
        lastSeen: this.lastSeen,
        lastConnected: this.lastConnected,

        lastClose: this.lastClose,
        lastError: this.lastError,
        lastMessage: this.lastMessage
      });
    }

    if (url.pathname !== "/connect") {
      return new Response("Not found", { status: 404 });
    }

    if (
      request.headers.get("Upgrade")?.toLowerCase() !== "websocket"
    ) {
      return new Response("WebSocket required", { status: 426 });
    }

    const role = url.searchParams.get("role");

    if (role !== "device" && role !== "viewer") {
      return new Response("Invalid role", { status: 400 });
    }

    const pair = new WebSocketPair();
    const client = pair[0];
    const server = pair[1];

    server.serializeAttachment({
      role,
      connectedAt: Date.now()
    });

    this.state.acceptWebSocket(server, [role]);

    this.lastConnected = {
      role,
      time: Date.now()
    };

    this.lastClose = null;
    this.lastError = null;

    console.log("[WS] CONNECTED:", role);

    return new Response(null, {
      status: 101,
      webSocket: client
    });
  }

  webSocketMessage(ws, message) {
    const attachment = ws.deserializeAttachment();

    if (attachment?.role !== "device") {
      console.log("[WS] Unauthorized viewer message");
      ws.close(1008, "Read only connection");
      return;
    }

    if (typeof message !== "string") {
      this.lastError = {
        type: "NON_TEXT_MESSAGE",
        time: Date.now()
      };

      ws.close(1003, "Text JSON only");
      return;
    }

    let packet;

    try {
      packet = JSON.parse(message);
    } catch {
      this.lastError = {
        type: "INVALID_JSON",
        time: Date.now()
      };

      ws.close(1007, "Invalid JSON");
      return;
    }

    if (
      packet?.type !== "frames" ||
      !Array.isArray(packet.frames) ||
      packet.frames.length > 500
    ) {
      this.lastError = {
        type: "INVALID_CAN_PACKET",
        time: Date.now()
      };

      ws.close(1008, "Invalid CAN packet");
      return;
    }

    this.totalFrames += packet.frames.length;
    this.lastSeen = Date.now();

    this.lastMessage = {
      type: packet.type,
      frames: packet.frames.length,
      time: Date.now()
    };

    const outgoing = JSON.stringify({
      type: "frames",
      frames: packet.frames
    });

    for (const viewer of this.state.getWebSockets("viewer")) {
      try {
        viewer.send(outgoing);
      } catch (error) {
        console.log("[WS] BROADCAST ERROR:", String(error));

        try {
          viewer.close(1011, "Broadcast error");
        } catch {}
      }
    }
  }

  webSocketClose(ws, code, reason, wasClean) {
    const attachment = ws.deserializeAttachment();

    this.lastClose = {
      role: attachment?.role || "unknown",
      code,
      reason: String(reason || ""),
      wasClean: Boolean(wasClean),
      time: Date.now()
    };

    console.log(
      "[WS] CLOSED:",
      JSON.stringify(this.lastClose)
    );
  }

  webSocketError(ws, error) {
    const attachment = ws.deserializeAttachment();

    this.lastError = {
      role: attachment?.role || "unknown",
      message: String(error),
      time: Date.now()
    };

    console.log(
      "[WS] ERROR:",
      JSON.stringify(this.lastError)
    );
  }
}
