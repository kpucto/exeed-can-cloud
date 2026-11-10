
const DEVICE_TOKEN = "DEVICE_TOKEN";
const VIEWER_TOKEN = "VIEWER_TOKEN";

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    const path = url.pathname;

    if (path === "/" || path === "/health") {
      return Response.json({
        status: "online",
        service: "EXEED CAN CLOUD",
        mode: "READ ONLY"
      });
    }

    let role;

    if (path === "/device") {
      const auth = request.headers.get("Authorization");
      if (auth !== `Bearer ${env.DEVICE_TOKEN}`) {
        return new Response("Unauthorized", { status: 401 });
      }
      role = "device";
    } else if (path === "/viewer") {
      if (url.searchParams.get("token") !== env.VIEWER_TOKEN) {
        return new Response("Unauthorized", { status: 401 });
      }
      role = "viewer";
    } else if (path === "/status") {
      role = "status";
    } else {
      return new Response("Not found", { status: 404 });
    }

    if (role !== "status" &&
        request.headers.get("Upgrade")?.toLowerCase() !== "websocket") {
      return new Response("WebSocket required", { status: 426 });
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
  }

  async fetch(request) {
    const url = new URL(request.url);

    if (url.pathname === "/status") {
      return Response.json({
        status: "online",
        mode: "READ ONLY",
        viewers: this.state.getWebSockets("viewer").length,
        devices: this.state.getWebSockets("device").length,
        totalFrames: this.totalFrames,
        lastSeen: this.lastSeen
      });
    }

    if (url.pathname !== "/connect") {
      return new Response("Not found", { status: 404 });
    }

    if (request.headers.get("Upgrade")?.toLowerCase() !== "websocket") {
      return new Response("WebSocket required", { status: 426 });
    }

    const role = url.searchParams.get("role");

    if (role !== "device" && role !== "viewer") {
      return new Response("Invalid role", { status: 400 });
    }

    const pair = new WebSocketPair();
    const client = pair[0];
    const server = pair[1];

    this.state.acceptWebSocket(server, [role]);
    server.serializeAttachment({ role });

    return new Response(null, {
      status: 101,
      webSocket: client
    });
  }

  webSocketMessage(ws, message) {
    const attachment = ws.deserializeAttachment();
    const role = attachment?.role;

    // Браузер может только получать данные.
    if (role !== "device") {
      ws.close(1008, "Read only connection");
      return;
    }

    if (typeof message !== "string") {
      ws.close(1003, "Text JSON only");
      return;
    }

    let packet;

    try {
      packet = JSON.parse(message);
    } catch {
      ws.close(1007, "Invalid JSON");
      return;
    }

    if (
      packet?.type !== "frames" ||
      !Array.isArray(packet.frames) ||
      packet.frames.length > 500
    ) {
      ws.close(1008, "Invalid CAN packet");
      return;
    }

    this.totalFrames += packet.frames.length;
    this.lastSeen = Date.now();

    const outgoing = JSON.stringify({
      type: "frames",
      frames: packet.frames
    });

    for (const viewer of this.state.getWebSockets("viewer")) {
      try {
        viewer.send(outgoing);
      } catch {
        try {
          viewer.close(1011, "Broadcast error");
        } catch {}
      }
    }
  }

  webSocketClose(ws, code, reason, wasClean) {
    try {
      ws.close(code, reason);
    } catch {}
  }

  webSocketError(ws) {
    try {
      ws.close(1011, "WebSocket error");
    } catch {}
  }
}
