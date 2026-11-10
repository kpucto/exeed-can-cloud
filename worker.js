
export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    const path = url.pathname;

    // =========================================
    // HEALTH CHECK
    // =========================================

    if (path === "/" || path === "/health") {
      return Response.json({
        status: "online",
        service: "EXEED CAN CLOUD",
        mode: "READ ONLY"
      });
    }

    let role;

    // =========================================
    // DEVICE AUTHORIZATION
    // =========================================

    if (path === "/device") {
      const auth = request.headers.get("Authorization") || "";

      const receivedToken = auth.startsWith("Bearer ")
        ? auth.substring(7)
        : "";

      const expectedToken = env.DEVICE_TOKEN || "";

      console.log("[AUTH] DEVICE request");
      console.log("[AUTH] Header exists:", auth.length > 0);
      console.log("[AUTH] Bearer prefix:", auth.startsWith("Bearer "));
      console.log("[AUTH] Received token length:", receivedToken.length);
      console.log("[AUTH] Expected token length:", expectedToken.length);
      console.log("[AUTH] Tokens match:", receivedToken === expectedToken);

      if (!expectedToken || receivedToken !== expectedToken) {
        console.log("[AUTH] DEVICE AUTH FAILED");

        return new Response("Unauthorized", {
          status: 401
        });
      }

      console.log("[AUTH] DEVICE AUTH OK");

      role = "device";

    // =========================================
    // VIEWER AUTHORIZATION
    // =========================================

    } else if (path === "/viewer") {

      const viewerToken = url.searchParams.get("token") || "";
      const expectedViewerToken = env.VIEWER_TOKEN || "";

      if (
        !expectedViewerToken ||
        viewerToken !== expectedViewerToken
      ) {
        return new Response("Unauthorized", {
          status: 401
        });
      }

      role = "viewer";

    // =========================================
    // STATUS
    // =========================================

    } else if (path === "/status") {

      role = "status";

    } else {

      return new Response("Not found", {
        status: 404
      });
    }

    // =========================================
    // WEBSOCKET VALIDATION
    // =========================================

    if (
      role !== "status" &&
      request.headers.get("Upgrade")?.toLowerCase() !== "websocket"
    ) {
      return new Response("WebSocket required", {
        status: 426
      });
    }

    // =========================================
    // DURABLE OBJECT
    // =========================================

    const id = env.CAN_HUB.idFromName("exeed-main");
    const stub = env.CAN_HUB.get(id);

    const internalUrl = new URL(request.url);

    internalUrl.pathname =
      role === "status" ? "/status" : "/connect";

    internalUrl.searchParams.set("role", role);

    return stub.fetch(
      new Request(internalUrl, request)
    );
  }
};


// =====================================================
// CAN HUB DURABLE OBJECT
// =====================================================

export class CanHub {

  constructor(state, env) {
    this.state = state;
    this.env = env;

    this.totalFrames = 0;
    this.lastSeen = null;
  }

  // ===================================================
  // REQUEST HANDLER
  // ===================================================

  async fetch(request) {
    const url = new URL(request.url);

    // -----------------------------------------------
    // STATUS
    // -----------------------------------------------

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
      return new Response("Not found", {
        status: 404
      });
    }

    // -----------------------------------------------
    // WEBSOCKET CHECK
    // -----------------------------------------------

    if (
      request.headers.get("Upgrade")?.toLowerCase() !== "websocket"
    ) {
      return new Response("WebSocket required", {
        status: 426
      });
    }

    const role = url.searchParams.get("role");

    if (role !== "device" && role !== "viewer") {
      return new Response("Invalid role", {
        status: 400
      });
    }

    // -----------------------------------------------
    // CREATE WEBSOCKET
    // -----------------------------------------------

    const pair = new WebSocketPair();

    const client = pair[0];
    const server = pair[1];

    this.state.acceptWebSocket(server, [role]);

    server.serializeAttachment({
      role: role
    });

    console.log("[CAN HUB] WebSocket connected:", role);

    return new Response(null, {
      status: 101,
      webSocket: client
    });
  }

  // ===================================================
  // RECEIVE WEBSOCKET MESSAGE
  // ===================================================

  webSocketMessage(ws, message) {

    const attachment = ws.deserializeAttachment();
    const role = attachment?.role;

    // Browser is READ ONLY.
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

    // -----------------------------------------------
    // PACKET VALIDATION
    // -----------------------------------------------

    if (
      packet?.type !== "frames" ||
      !Array.isArray(packet.frames) ||
      packet.frames.length > 500
    ) {
      ws.close(1008, "Invalid CAN packet");
      return;
    }

    // -----------------------------------------------
    // UPDATE STATISTICS
    // -----------------------------------------------

    this.totalFrames += packet.frames.length;
    this.lastSeen = Date.now();

    // -----------------------------------------------
    // BROADCAST TO VIEWERS
    // -----------------------------------------------

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

  // ===================================================
  // CONNECTION CLOSED
  // ===================================================

  webSocketClose(ws, code, reason, wasClean) {
    console.log(
      "[CAN HUB] WebSocket closed:",
      code,
      reason
    );

    try {
      ws.close(code, reason);
    } catch {}
  }

  // ===================================================
  // CONNECTION ERROR
  // ===================================================

  webSocketError(ws) {
    console.log("[CAN HUB] WebSocket error");

    try {
      ws.close(1011, "WebSocket error");
    } catch {}
  }
}
